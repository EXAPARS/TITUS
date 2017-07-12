/* 
* This file is part of the TITUS software.
* https://github.com/exapars/TITUS
* Copyright (c) 2015-2017 University of Versailles UVSQ
* Copyright (c) 2017 Bull SAS
*
* TITUS  is a free software: you can redistribute it and/or modify  
* it under the terms of the GNU Lesser General Public License as   
* published by the Free Software Foundation, version 3.
*
* TITUS is distributed in the hope that it will be useful, but 
* WITHOUT ANY WARRANTY; without even the implied warranty of 
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*/


#include "TITUS_DLB_internal.hpp"
#include "TITUS_DLB_context.hpp"


TITUS_DLB_Context_impl* 			TITUS_DLB_impl::					context 					= nullptr;

bool 						TITUS_DLB_impl::					barrier_segment_is_init 	= false;
gaspi_segment_id_t 			TITUS_DLB_impl::					segment_barrier 			= 10; // needed for notifications (???)
gaspi_segment_id_t 			TITUS_DLB_impl::barrier_status::	segment_barrier 			= 10;
gaspi_pointer_t 			TITUS_DLB_impl::					ptr_segment_barrier 		= nullptr;
size_t 						TITUS_DLB_impl::					shared_barrier_segment_size = 1000; // unused.
TITUS_DLB_impl::barrier_status * 	TITUS_DLB_impl::					barrier_statuses 			= nullptr;

gaspi_queue_id_t 			TITUS_DLB_impl::					queue_barrier = 100; //! TODO : use max queue -1 ?
gaspi_queue_id_t 			TITUS_DLB_impl::barrier_status::	queue_barrier = 100;

bool                        TITUS_DLB_impl::                  gaspi_init_complete = false;

void TITUS_DLB_impl::set_context(TITUS_DLB_Context_impl * arg) {
    context = arg;
    DVS::set_context(arg->get_DVS_context());
}
TITUS_DLB_Context_impl * TITUS_DLB_impl::get_context() {
    return context;
}


TITUS_DLB_impl::barrier_status::barrier_status(gaspi_group_t group):
	handshake_ok(false),
	group(group),
	barrier_is_in_progress(false),
	rank(((gaspi_rank_t)-1)),
	me_in_group(-1),
	group_size(0),
	ranks_in_group(nullptr),
	left_child_ok(false), right_child_ok(false), parent_ok(false)
{
	SUCCESS_OR_DIE( gaspi_proc_rank(&rank) );
}

// if barrier not in progress, initialises fields and sets barrier in progress.
// else returns immediately and do nothing.
void TITUS_DLB_impl::barrier_status::init_barrier(){
	if (barrier_is_in_progress) return;	
	
	if (group != GASPI_GROUP_ALL){
		gaspi_rank_t old_group_size = group_size;
		SUCCESS_OR_DIE( gaspi_group_size(group, &group_size) );
		
		if (group_size != old_group_size){
			handshake_ok = false;
			if (ranks_in_group != nullptr) delete ranks_in_group;
			ranks_in_group = new gaspi_rank_t[group_size];
		}
		SUCCESS_OR_DIE( gaspi_group_ranks(group, ranks_in_group) );
	
		for (gaspi_rank_t i=0 ; i != ((uint16_t)-1) ; i++){
			if (ranks_in_group[i] == rank){
				me_in_group = i;
				break;
			}
		}
	}
	else{
		// because build_infrastructure was set to false, GASPI_GROUP_ALL is disabled
		// mock initialization using gaspi global info getters instead of group info getters
		gaspi_rank_t old_group_size = group_size;
		gaspi_rank_t proc_num;
		SUCCESS_OR_DIE( gaspi_proc_num(& proc_num) );
		group_size = proc_num;
		me_in_group = rank;
		
		// all ranks are in the group
		if (group_size != old_group_size){
			handshake_ok = false;
			if (ranks_in_group != nullptr) delete ranks_in_group;
			ranks_in_group = new gaspi_rank_t[group_size];
			
			for (gaspi_rank_t i=0;i<group_size;i++) ranks_in_group[i] = i;
		}
		
		neighbors = infixed_b_tree_neighbors::get_tree_neighbors(me_in_group, group_size);
	}
	
	// ### connect ###
	neighbors = infixed_b_tree_neighbors::get_tree_neighbors(me_in_group, group_size);
	if (neighbors.parent != ((uint16_t)-1)){
		while(gaspi_connect(neighbors.parent, GASPI_BLOCK) != GASPI_SUCCESS) usleep(10);
		SUCCESS_OR_DIE( gaspi_segment_register(segment_barrier, neighbors.parent, GASPI_BLOCK) );
	}
	if (neighbors.left_child != ((uint16_t)-1)){
		while(gaspi_connect(neighbors.left_child, GASPI_BLOCK) != GASPI_SUCCESS) usleep(10);
		SUCCESS_OR_DIE( gaspi_segment_register(segment_barrier, neighbors.left_child, GASPI_BLOCK) );
	}
	if (neighbors.right_child != ((uint16_t)-1)){
		while(gaspi_connect(neighbors.right_child, GASPI_BLOCK) != GASPI_SUCCESS) usleep(10);
		SUCCESS_OR_DIE( gaspi_segment_register(segment_barrier, neighbors.right_child, GASPI_BLOCK) );
	}
	
	left_child_ok = right_child_ok = parent_ok = false;
	barrier_is_in_progress = true;


	// if first barrier : handshake protocol with neighbors to ensure segments are available.
	if (!handshake_ok)first_barrier_handshake_protocol();
	
}


gaspi_return_t gaspi_notify_no_overflow(
	gaspi_segment_id_t segment, 
	gaspi_rank_t rank, 
	gaspi_notification_id_t id, 
	gaspi_notification_t value, 
	gaspi_queue_id_t queue, 
	gaspi_timeout_t timeout
){
	static const uint min_time = 100; // usecs
	static uint64_t last_sent = 0;
	uint64_t now = rdtsc();
	uint usecs_to_sleep = min_time > ((now - last_sent) / (double)(TITUS_PROC_FREQ / 1e6)) ? min_time - ((now - last_sent) / (double)(TITUS_PROC_FREQ / 1e6)) : 0; // @ 2.4 Ghz
	if (timeout == GASPI_BLOCK || usecs_to_sleep / 1000 < timeout){
		if (usecs_to_sleep > 0){
			//TITUS_DBG << "sleeping for " << usecs_to_sleep << " usecs" << std::endl; //TITUS_DBG.flush();
			usleep(usecs_to_sleep);
		}
		last_sent = rdtsc();
		return gaspi_notify(segment, rank, id, value, queue, timeout);
	}
	else {
		return GASPI_TIMEOUT;
	}
}


void TITUS_DLB_impl::barrier_status::first_barrier_handshake_protocol(){
	bool left_child_msg_sent = neighbors.left_child == ((uint16_t)-1);
	bool left_child_msg_recieved = neighbors.left_child == ((uint16_t)-1);
	bool right_child_msg_sent = neighbors.right_child == ((uint16_t)-1);
	bool right_child_msg_recieved = neighbors.right_child == ((uint16_t)-1);
	bool parent_msg_sent = neighbors.parent == ((uint16_t)-1);
	bool parent_msg_recieved = neighbors.parent == ((uint16_t)-1);

	
	void * barrier_segment_ptr; SUCCESS_OR_DIE(gaspi_segment_ptr(segment_barrier, &barrier_segment_ptr));
	// used by parent for notifying children of the end of the barrier
	gaspi_offset_t recieve_offset = 0;
	bool * recieve_ptr = (bool*) ADD_PTR(barrier_segment_ptr, recieve_offset);
	*recieve_ptr = false;
	
	// used by parent for notifying children of the end of the barrier
	gaspi_offset_t msg_offset = recieve_offset + sizeof(bool);
	bool * msg_ptr =  (bool*) ADD_PTR(barrier_segment_ptr, msg_offset);
	*msg_ptr = true;
	
	// print neighbors
	//TITUS_DBG << neighbors << std::endl; //TITUS_DBG.flush();
	
	// send
	while (!(left_child_msg_sent && right_child_msg_sent && parent_msg_sent)
			||
		   !(left_child_msg_recieved && right_child_msg_recieved && parent_msg_recieved)){
		// send
		if (!left_child_msg_sent){
			left_child_msg_sent  = ( gaspi_passive_send(segment_barrier, msg_offset, neighbors.left_child,  sizeof(bool), GASPI_TEST) == GASPI_SUCCESS );
			if (left_child_msg_sent){
				//~ std::cout << "rank " << rank << " : passive send completed with left child (" << neighbors.left_child << ")." << std::endl;
			}
		}
		if (!right_child_msg_sent){
			right_child_msg_sent = ( gaspi_passive_send(segment_barrier, msg_offset, neighbors.right_child, sizeof(bool), GASPI_TEST) == GASPI_SUCCESS );
			if (right_child_msg_sent) {
				//~ std::cout << "rank " << rank << " : passive send completed with right child (" << neighbors.right_child << ")." << std::endl;
			}
		}
		if (!parent_msg_sent){
			parent_msg_sent      = ( gaspi_passive_send(segment_barrier, msg_offset, neighbors.parent,      sizeof(bool), GASPI_TEST) == GASPI_SUCCESS );
			if (parent_msg_sent) {
				//~ std::cout << "rank " << rank << " : passive send completed with parent (" << neighbors.parent << ")." << std::endl;
			}
		}
		// try recieve
		usleep(100000);
		gaspi_rank_t rcv_rank = 0;
		if (gaspi_passive_receive(segment_barrier, recieve_offset, &rcv_rank, sizeof(bool), GASPI_TEST) == GASPI_SUCCESS){

			//assert(*recieve_ptr == true);
			//*recieve_ptr = false;

			if (rcv_rank == neighbors.left_child){
				left_child_msg_recieved = true;
				//~ std::cout << "rank " << rank << " : handshaked with left child (" << neighbors.left_child << ") : passive recieve completed." << std::endl;
				continue;
			}
			if (rcv_rank == neighbors.right_child){
				right_child_msg_recieved = true;
				//~ std::cout << "rank " << rank << " : handshaked with right child (" << neighbors.right_child << ") : passive recieve completed." << std::endl;
				continue;
			}
			if (rcv_rank == neighbors.parent){
				parent_msg_recieved = true;
				//~ std::cout << "rank " << rank << " : handshaked with parent (" << neighbors.parent << ") : passive recieve completed." << std::endl;
				continue;
			}

		}
		usleep(100);
	}
	//~ std::cout << "rank " << rank << " : ### handshake prtocol complete." << std::endl;
	handshake_ok = true;
}





// notifications based barrier using a b-tree with infixed numerotation (as in sorted b-tree)
gaspi_return_t TITUS_DLB_impl::barrier_status::barrier(gaspi_timeout_t timeout){
	// init instance fields and handshake if first barrier for this group.
	init_barrier();
	
	if (rank==0) 
	
	//~ TITUS_DBG << "init_barrier done, entering barrier_status::barrier" << std::endl; //TITUS_DBG.flush();
		
	if (group_size == 1) return GASPI_SUCCESS;
	if (group_size == 0){
			TITUS_DBG << "WARNING : barrier_status::barrier() : EMPTY GROUP ID=" << ((uint)group) << ". returning GASPI_ERROR(" << GASPI_ERROR << ")" << std::endl; //TITUS_DBG.flush();
			return GASPI_ERROR;
	}
	if (me_in_group == -1){
		TITUS_DBG << "WARNING : barrier_status::barrier() : RANK "<< rank << "NOT IN GROUP ID=" << ((uint)group) << ". returning GASPI_ERROR(" << GASPI_ERROR << ")" << std::endl; //TITUS_DBG.flush();
		return GASPI_ERROR;
	}
	
	// used by parent for notifying children of the end of the barrier
	gaspi_notification_id_t parent_notification_id = MIN_BARRIER_NOTIFICATION_ID + NOTIFICATION_ID_PER_BARRIER_PER_GROUP * group;
	// used by children to notify parent of the advancement of the barrier
	gaspi_notification_id_t left_child_notification_id = parent_notification_id + 1;
	gaspi_notification_id_t right_child_notification_id = left_child_notification_id + 1;
	
	// ### notify from leafs to root ###
	if (left_child_ok == false || right_child_ok == false){
		if (neighbors.left_child != ((uint16_t)-1) || left_child_ok){
			// wait for left child notification
			gaspi_notification_id_t dummy;
			gaspi_return_t r = gaspi_notify_waitsome(segment_barrier, left_child_notification_id, 1, &dummy, timeout);
			if (r != GASPI_SUCCESS) return r; // may leave due to timeout
			gaspi_notification_t val;
			SUCCESS_OR_DIE(gaspi_notify_reset(segment_barrier, left_child_notification_id, &val));
			//~ if (val != ((uint)ranks_in_group[neighbors.left_child]+1))
				//~ TITUS_DBG << "ASSERT IS GOING TO FAIL : " << val << " != " << ranks_in_group[neighbors.left_child]+1 << ")" << std::endl; //TITUS_DBG.flush();
			ASSERT( val == ((uint)ranks_in_group[neighbors.left_child]+1) );
		}
		left_child_ok = true;
		if (neighbors.right_child != ((uint16_t)-1) || right_child_ok){
			// wait for left child notification
			gaspi_notification_id_t dummy;
			gaspi_return_t r = gaspi_notify_waitsome(segment_barrier, right_child_notification_id, 1, & dummy, timeout);
			if (r != GASPI_SUCCESS) return r; // may leave due to timeout
			gaspi_notification_t val;
			SUCCESS_OR_DIE(gaspi_notify_reset(segment_barrier, right_child_notification_id, &val));
			//~ if (val != ((uint)ranks_in_group[neighbors.right_child]+1))
				//~ TITUS_DBG << "ASSERT IS GOING TO FAIL : " << val << " != " << ranks_in_group[neighbors.right_child]+1 << ")" << std::endl; //TITUS_DBG.flush();
			ASSERT( val == ((uint)ranks_in_group[neighbors.right_child]+1) );
		}
		right_child_ok = true;
		// else : i am leaf. nowait
		// notify up once per barrier session
		if (neighbors.parent != ((uint16_t)-1)){
			gaspi_notification_id_t upward_notification_id = neighbors.is_left_child() ? left_child_notification_id : right_child_notification_id;
			SUCCESS_OR_DIE(gaspi_notify(segment_barrier, ranks_in_group[neighbors.parent], upward_notification_id, rank+1, 0, TITUS_DLB_GASPI_TIMEOUT));
			//~ TITUS_DBG << "sent upward notification to parent rank " << neighbors.parent << " : target = " << ranks_in_group[neighbors.parent] << ", notification ID = " << upward_notification_id << " notification value = " << rank +1 << std::endl; //TITUS_DBG.flush();
		}
	}

	// ### notify from root to leafs ###
	if (neighbors.parent != ((uint16_t)-1)){
		gaspi_notification_id_t dummy;
		gaspi_return_t r = gaspi_notify_waitsome(segment_barrier, parent_notification_id, 1, & dummy, timeout);
		if (r != GASPI_SUCCESS) return r; // may leave due to timeout
		gaspi_notification_t val;
		SUCCESS_OR_DIE(gaspi_notify_reset(segment_barrier, parent_notification_id, &val));
		if (val != ((uint)ranks_in_group[neighbors.parent]+1))
			TITUS_DBG << "ASSERT IS GOING TO FAIL : " << val << " != " << ranks_in_group[neighbors.parent]+1 << ")" << std::endl; //TITUS_DBG.flush();
		ASSERT( val == ((uint)ranks_in_group[neighbors.parent]+1) );
	}
	// else : i am root. nowait
	// notify down
	if (neighbors.left_child != ((uint16_t)-1)){
		SUCCESS_OR_DIE(gaspi_notify(segment_barrier, ranks_in_group[neighbors.left_child], parent_notification_id, rank+1, 0, TITUS_DLB_GASPI_TIMEOUT));
		//~ TITUS_DBG << "sent downward notifications to left child : target = " << ranks_in_group[neighbors.left_child] << ", notification ID = " << parent_notification_id << " notification value = " << rank +1 << std::endl; //TITUS_DBG.flush();
	}
	if (neighbors.right_child != ((uint16_t)-1)){
		SUCCESS_OR_DIE(gaspi_notify(segment_barrier, ranks_in_group[neighbors.right_child], parent_notification_id, rank+1, 0, TITUS_DLB_GASPI_TIMEOUT));
		//~ TITUS_DBG << "sent downward notifications to right child : target = " << ranks_in_group[neighbors.right_child] << ", notification ID = " << parent_notification_id << " notification value = " << rank +1 << std::endl; //TITUS_DBG.flush();
	}
	barrier_is_in_progress = false;
	return GASPI_SUCCESS;
}


gaspi_return_t TITUS_DLB_impl::barrier(gaspi_group_t group, gaspi_timeout_t timeout){
	// trivia checks
	gaspi_config_t config;
    SUCCESS_OR_DIE(gaspi_config_get(&config));
    if (config.build_infrastructure != GASPI_TOPOLOGY_NONE) return gaspi_barrier(group, timeout);
	
	TITUS_DBG << "ERROR : TITUS_DLB_impl::barrier() : TITUS_DLB::barrier is buggy and should not be used with gaspi static topology. It is a placeholder for future development. Use GASPI_TOPOLOGY_DYNAMIC" << std::endl; //TITUS_DBG.flush();
	ASSERT(false);
	
	gaspi_number_t gaspi_is_init; SUCCESS_OR_DIE(gaspi_initialized(& gaspi_is_init));
	if (gaspi_is_init == false){
		TITUS_DBG << "ERROR : TITUS_DLB_impl::barrier() : gaspi_proc_init has not been called before." << std::endl; //TITUS_DBG.flush();
		ASSERT(gaspi_is_init != false);
		return GASPI_ERROR;
	}
	gaspi_number_t group_max; gaspi_group_max(&group_max);
	if (group >= group_max){
		TITUS_DBG << "ERROR : TITUS_DLB_impl::barrier() : INVALID GROUP ID=" << ((uint)group) << std::endl; //TITUS_DBG.flush();
		ASSERT(group < group_max);
		return GASPI_ERROR;
	}
	
	
	// where is gaspi_build_infrastructure ??
	//~ gaspi_topology_t topo;
	//~ SUCCESS_OR_DIE( gaspi_build_infrastructure(& topo) );
	//~ if (topo != GASPI_TOPOLOGY_NONE) return gaspi_barrier(group,timeout);    
	
	
	// static fields init
	// those are supposed to be static variables in the TITUS_DLB class
	static bool barrier_segment_is_init = false;
	if (! barrier_segment_is_init){
		SUCCESS_OR_DIE( gaspi_segment_alloc(segment_barrier, 10000, GASPI_ALLOC_DEFAULT) );
		barrier_segment_is_init = true;
	}
	
	static barrier_status * statuses = nullptr;
	if (statuses == nullptr){
		statuses = (barrier_status*)malloc(sizeof(barrier_status) * group_max);ASSERT(statuses != nullptr);
		for (int i=0 ; i< group_max ; i++){
			new (& statuses[i] ) barrier_status(i);
		}
	}
	
	// barrier
	return statuses[group].barrier(timeout);
}

gaspi_return_t TITUS_DLB_impl::gaspi_proc_init_impl(gaspi_timeout_t timeout){
	gaspi_number_t gaspi_is_init; gaspi_initialized(& gaspi_is_init);
	if (gaspi_is_init) return GASPI_SUCCESS;
	
	static bool first_call = true;
	if (first_call){
		first_call = false;
		
	    gaspi_config_t config;
		ASSERT( gaspi_config_get(&config) == GASPI_SUCCESS);
		
		config.queue_num = 3;
		config.build_infrastructure = GASPI_TOPOLOGY_DYNAMIC;
		
		ASSERT( gaspi_config_set(config) == GASPI_SUCCESS);
	}
	
	// ############ GASPI PROC INIT ################
	gaspi_return_t r = gaspi_proc_init(timeout);	
	// ############################# ################
	
	if (r == GASPI_SUCCESS){
		gaspi_config_t config;
		SUCCESS_OR_DIE( gaspi_config_get(&config));
		if (config.queue_num < 3){
			TITUS_DBG << "TITUS_DLB_impl::gaspi_proc_init_impl : ASSERT IS GOING TO FAIL : number of queues must be at least 3. maybe gaspi_proc_init enforced it. check gaspi sources at GPI2.c:pgaspi_init_core()" << std::endl;
		}
		ASSERT(config.queue_num >= 3);
		gaspi_rank_t rank; gaspi_proc_rank(& rank);
		if (rank == 0){
			TITUS_DBG << "TITUS_DLB_gaspi_proc_init() done using config :" << std::endl ;
			print_gaspi_config(TITUS_DBG);
			//TITUS_DBG.flush();
		}
		// TITUS_DBG_CONFIG
		std::stringstream dbg_out_name("");
		dbg_out_name << "dbg_out_" << std::setfill('0') << std::setw(4) << rank << ".out";
		std::ofstream * dbg_out = new std::ofstream(dbg_out_name.str().c_str());
		TITUS_DBG.set_output(dbg_out);
		TITUS_DBG.reset_timeref();
		TITUS_DBG << "time reset" << std::endl; TITUS_DBG.flush();
	}
	gaspi_init_complete = true;

	return r;
}

#ifdef DEBUG
void TITUS_DLB_impl::display_state() { context->display_state();}
void TITUS_DLB_impl::display_metadatatask() { context->display_metadatatask();}
void TITUS_DLB_impl::display_metadataresult() { context->display_metadataresult();}
void TITUS_DLB_impl::display_metadatatmp() { context->display_metadatatmp();}
void TITUS_DLB_impl::display_dequeue() { context->display_dequeue();}
void TITUS_DLB_impl::display_Context() { context->display_Context();}
#endif
