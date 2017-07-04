/* 
* This file is part of the TITUS software.
* https://github.com/exapars/TITUS
* Copyright (c) 2015-2016 University of Versailles UVSQ
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


#include "TITUS_DLB_segment_metadata.hpp"
#include <cstring> //memcpy
#include <utility>

// *********************************************************************
// *********************** METADATA (BASE) *****************************
// *********************************************************************

// *** CONSTRUCTOR ***
SegmentMetadata::SegmentMetadata(size_t size, gaspi_segment_id_t segment_id):
	state(TASK_AVAILABLE), segment_size(size),segment_id(segment_id)
{}


SegmentMetadata * SegmentMetadata::read_to_scratch(gaspi_rank_t target_rank, gaspi_offset_t offset_in_scratch)const{
	gaspi_segment_id_t target_segment_id = this->segment_id;
	return read_to_scratch(target_rank, target_segment_id, offset_in_scratch);
}

SegmentMetadata * SegmentMetadata::read_to_scratch(gaspi_rank_t target_rank, gaspi_segment_id_t target_segment_id, gaspi_offset_t offset_in_scratch){
	if (target_segment_id == (gaspi_segment_id_t)-1)
		target_segment_id = TITUS_DLB_impl::get_context()->segment_task;
	gaspi_segment_id_t scratch_segment_id = TITUS_DLB_impl::get_context()->segment_scratch;
	gaspi_queue_id_t queue_scratch_id = TITUS_DLB_impl::get_context()->queue_scratch;
	SUCCESS_OR_DIE( gaspi_read( scratch_segment_id, offset_in_scratch, target_rank, target_segment_id, 0, sizeof(SegmentMetadata), queue_scratch_id, TITUS_DLB_GASPI_TIMEOUT ) );
	SUCCESS_OR_DIE( gaspi_wait( queue_scratch_id, TITUS_DLB_GASPI_TIMEOUT ) );

	return (SegmentMetadata*) ADD_PTR(TITUS_DLB_impl::get_context()->ptr_segment_scratch , offset_in_scratch);
}

gaspi_atomic_value_t SegmentMetadata::compare_and_swap_status(gaspi_rank_t target, gaspi_segment_id_t target_segment, gaspi_atomic_value_t comp, gaspi_atomic_value_t new_val, bool nowait){
	TITUS_DLB_Context_impl * ctx = TITUS_DLB_impl::get_context();
	if (target >= ctx->get_nb_ranks()){
		TITUS_DBG << "ERROR RANK " << ctx->get_rank() << " : ASSERT IS GOING TO FAIL : " <<
			target << " >= " << ctx->get_nb_ranks() << std::endl; //TITUS_DBG.flush();
		ASSERT(target < ctx->get_nb_ranks());
	}
	
	if (ctx->registrations_status[target] != true){
		TITUS_DBG << "ERROR RANK " << ctx->get_rank() << " : ASSERT IS GOING TO FAIL : " <<
			"registrations_status[" << target << "] != true" << std::endl; //TITUS_DBG.flush();
		ASSERT(ctx->registrations_status[target] == true);
	}

	gaspi_number_t nb_segs; SUCCESS_OR_DIE(gaspi_segment_num(&nb_segs));
	ASSERT(target_segment < nb_segs);
	
	
    gaspi_atomic_value_t old_value;
    gaspi_offset_t segment_status_offset = offsetof(SegmentMetadata, state);
	
	//TITUS_DBG << "SegmentMetadata::compare_and_swap_status(" <<
	//	"target=" << target << ", target_segment = " << (uint)target_segment << ", comp=" << comp << ", new_val=" << new_val << ")" << std::endl; //TITUS_DBG.flush();
	static uint64_t date_next_c_s; //! TODO : thread safety !
	int us_before_next_atempt = (date_next_c_s - rdtsc()) / (double) (TITUS_PROC_FREQ / 1e6);
	if (!nowait && us_before_next_atempt > 10){
		TITUS_DBG << "sleeping for " << us_before_next_atempt << "µs." << std::endl;
		usleep(us_before_next_atempt);
	}
	
	
    uint64_t f_called = rdtsc();
	if (gaspi_atomic_compare_swap(target_segment, segment_status_offset, target, comp, new_val, &old_value, TITUS_DLB_GASPI_TIMEOUT) != GASPI_SUCCESS){
		TITUS_DBG << "error rank " << ctx->get_rank() << " : " <<
			"SegmentMetadata::compare_and_swap_status : gaspi_atomic_compare_swap failed.\n" <<
			" > gaspi_atomic_compare_swap(target_segment=" << (uint)target_segment << ", segment_status_offset=" << segment_status_offset << ", target=" << target << ", comp=" << comp << ", new_val=" << new_val << ", &old_value, timeout=" << TITUS_DLB_GASPI_TIMEOUT << "ms)" << std::endl; //TITUS_DBG.flush();
		exit (EXIT_FAILURE);
	}
    uint64_t f_returned = rdtsc();
    double time_ms = (double)(f_returned - f_called) / (((double)TITUS_PROC_FREQ) / 1e3);
    if (time_ms > 10.0){
		char str[512];
		sprintf (str, "Warning : %f ms in gaspi_atomic_compare_swap [%s:%i]\n", time_ms, __FILE__, __LINE__);
		TITUS_DBG << str;
	}
	
	date_next_c_s = f_returned + 4 * (f_returned-f_called); //! TODO : find a portable adaptive approach to avoid network flood.
	
    return old_value;
}

std::ostream & operator<<(std::ostream & out, const SegmentMetadata & arg) {arg.print(out); return out;}


// *********************************************************************
// ****************** METADATA FOR TASK SEGMENT ************************
// *********************************************************************


// *** CONSTRUCTOR ***
MetadataTask::MetadataTask(TITUS_DLB_int shared_task_segment_size, gaspi_segment_id_t segment_id):
	SegmentMetadata(shared_task_segment_size, segment_id)
{
	reset();
}
void MetadataTask::reset() {
	private_tasks_count = 0;
	state = NO_TASK;
	nb_task = 0;
}

std::string MetadataTask::state_str(gaspi_atomic_value_t value)const{
	switch (value){
		case TASK_AVAILABLE : return "TASK_AVAILABLE";
		case FREE_FOR_COPY : return "FREE_FOR_COPY";
		case NO_TASK : return "NO_TASK";
		case COPY_IN_PROGRESS : return "COPY_IN_PROGRESS";
		default : std::stringstream a(""); a << "SEGMENT LOCKED BY RANK " << value; return a.str();
	}
}
std::string MetadataTask::state_str()const{
	return state_str(state);
}


MetadataTask * MetadataTask::read_to_scratch(gaspi_rank_t target_rank, gaspi_offset_t offset_in_scratch)const{
	gaspi_segment_id_t target_segment_id = this->segment_id;
	return read_to_scratch(target_rank, target_segment_id, offset_in_scratch);
}

MetadataTask * MetadataTask::read_to_scratch(gaspi_rank_t target_rank, gaspi_segment_id_t target_segment_id, gaspi_offset_t offset_in_scratch){
	if (target_segment_id == (gaspi_segment_id_t)-1)
		target_segment_id = TITUS_DLB_impl::get_context()->segment_task;
	gaspi_segment_id_t scratch_segment_id = TITUS_DLB_impl::get_context()->segment_scratch;
	gaspi_queue_id_t queue_scratch_id = TITUS_DLB_impl::get_context()->queue_scratch;
	SUCCESS_OR_DIE( gaspi_read( scratch_segment_id, offset_in_scratch, target_rank, target_segment_id, 0, sizeof(MetadataTask), queue_scratch_id, TITUS_DLB_GASPI_TIMEOUT ) );
	SUCCESS_OR_DIE( gaspi_wait( queue_scratch_id, TITUS_DLB_GASPI_TIMEOUT ) );

	return (MetadataTask*) ADD_PTR(TITUS_DLB_impl::get_context()->ptr_segment_scratch , offset_in_scratch);
}

std::ostream & operator<<(std::ostream & out, const MetadataTask & arg) {arg.print(out); return out;}

// *********************************************************************
// ***************** METADATA FOR RESULTS SEGMENT **********************
// *********************************************************************

// *** CONSTRUCTOR ***
MetadataResult::MetadataResult(size_t segment_size,  gaspi_segment_id_t segment_id, TITUS_DLB_int rank, TITUS_DLB_int nb_procs, TITUS_DLB_int nb_usr_results, TITUS_DLB_int result_size, void *usr_results):
	BufferedSegmentMetadata(segment_size, segment_id, sizeof(MetadataResult)),
	nb_usr_results(nb_usr_results),
	result_size(result_size),
	usr_results(usr_results)
{
	reset(rank, nb_procs);
}

std::string MetadataResult::state_str(gaspi_atomic_value_t value)const{
	return BufferedSegmentMetadata::state_str(value);
}
std::string MetadataResult::state_str()const{
	return state_str(state);
}

void MetadataResult::reset(TITUS_DLB_int rank, TITUS_DLB_int nb_procs) {
	nb_result = 0;
//	global_termination_detected = 0;
	state = SEGMENT_AVAILABLE;
	termination_status.global_termination_detected = 0;
	if((2*rank+1) < nb_procs)   termination_status.left_child_subtree_termination       = 0;    //I have a child and I wait for his results
	else                        termination_status.left_child_subtree_termination       = 1;    //I dont have a child, result=1
	
	if((2*rank+2) < nb_procs)   termination_status.right_child_subtree_termination      = 0;
	else                        termination_status.right_child_subtree_termination      = 1;
}

MetadataResult * MetadataResult::read_to_scratch(gaspi_rank_t target_rank, gaspi_offset_t offset_in_scratch)const{
	gaspi_segment_id_t target_segment_id = this->segment_id;
	return read_to_scratch(target_rank, target_segment_id, offset_in_scratch);
}

MetadataResult * MetadataResult::read_to_scratch(gaspi_rank_t target_rank, gaspi_segment_id_t target_segment_id, gaspi_offset_t offset_in_scratch){
	if (target_segment_id == (gaspi_segment_id_t)-1)
		target_segment_id = TITUS_DLB_impl::get_context()->segment_task;
	gaspi_segment_id_t scratch_segment_id = TITUS_DLB_impl::get_context()->segment_scratch;
	gaspi_queue_id_t queue_scratch_id = TITUS_DLB_impl::get_context()->queue_scratch;
	SUCCESS_OR_DIE( gaspi_read( scratch_segment_id, offset_in_scratch, target_rank, target_segment_id, 0, sizeof(MetadataResult), queue_scratch_id, TITUS_DLB_GASPI_TIMEOUT ) );
	SUCCESS_OR_DIE( gaspi_wait( queue_scratch_id, TITUS_DLB_GASPI_TIMEOUT ) );

	return (MetadataResult*) ADD_PTR(TITUS_DLB_impl::get_context()->ptr_segment_scratch , offset_in_scratch);
}


float MetadataResult::mem_occupancy() const			{return ((float)(buffer_head - buffer_tail)) / ((float)segment_size - sizeof(MetadataResult));}// return occupancy ration (0 = empty, 1 = full)
float MetadataResult::elt_count_occupancy()const	{return ((float)(buffer_elt_count)) / ((float)MAX_BUFFER_ELTS);}// return occupancy ration (0 = empty, 1 = full)
float MetadataResult::occupancy()const	 			{return std::max(mem_occupancy(), elt_count_occupancy());}// return occupancy ration (0 = empty, 1 = full)

std::ostream & operator<<(std::ostream & out, const MetadataResult & arg) {arg.print(out); return out;}
std::ostream & operator<<(std::ostream & out, const TerminationDetectionData & arg) {arg.print(out); return out;}

// *********************************************************************
// *************** METADATA FOR TMP RESULTS SEGMENT ********************
// *********************************************************************

// *** CONSTRUCTOR ***
MetadataTmp::MetadataTmp(size_t size, gaspi_segment_id_t segment_id, size_t result_size) : 
	SegmentMetadata(size, segment_id),
	owner_result_rank(TITUS_DLB_impl::get_context()->get_rank()),
	last_result_id(0),
	nb_result(0),
	nb_result_max((size - sizeof(MetadataTmp))/result_size -1),
	result_size(result_size),
	ptr_next_result_to_store(ADD_PTR(this , size - result_size)),
	dummy(0,0,0,0)
	{}

std::string MetadataTmp::state_str(gaspi_atomic_value_t value)const{
	switch (value){
		case TASK_AVAILABLE : return "SEGMENT CANNOT BE LOCKED"; // default initialization value, shall not be changed
		default : std::stringstream a(""); a << "BAD SEGMENT LOCK STATE : " << value; return a.str();
	}
}
std::string MetadataTmp::state_str()const{
	return state_str(state);
}

void MetadataTmp::reset(){
	owner_result_rank = -1;
	nb_result = 0;
	last_result_id = 0;
	result_size = 0;
	ptr_next_result_to_store = ADD_PTR(this , segment_size - result_size);
}

void MetadataTmp::push_tmp_results(){
	//~ TITUS_DBG << "in TITUS_DLB_impl::push_tmp_results" << std::endl;
	TITUS_DLB_Context_impl * ctx = TITUS_DLB_impl::get_context();
	if (nb_result == 0) return;
	
	if (owner_result_rank >= ctx->get_nb_ranks()){
		TITUS_DBG << "ERROR ASSERT IS GOING TO FAIL : " <<
			owner_result_rank << " >= " << ctx->get_nb_ranks() << std::endl; //TITUS_DBG.flush();
	}
	ASSERT(owner_result_rank < ctx->get_nb_ranks());
	
	// build a BufferElt right before results
	void * result_buffer_elt_loc = ADD_PTR(ADD_PTR(ptr_next_result_to_store, result_size),-sizeof(BufferElt));
	BufferElt * result_buffer_elt = new (result_buffer_elt_loc)BufferElt(last_result_id - nb_result, nb_result, result_size, owner_result_rank);
	
	
	//fprintf(stderr, "MetadataTmp::push_tmp_results : ");
    if (owner_result_rank == ctx->get_rank()){
		// those are my results : submit them localy.
        //TITUS_DBG << "push_tmp_results : computed " << nb_result << " of my own results to submit (first id = " << last_result_id - nb_result << ")" << std::endl; //TITUS_DBG.flush();
		//TITUS_DBG << "MetadataTmp::push_tmp_results : pushing results " << *result_buffer_elt << ", owner is local rank, submitting results." << std::endl; //TITUS_DBG.flush();
		ctx->submit_results(ADD_PTR(ptr_next_result_to_store, result_size), nb_result, last_result_id - nb_result);
		reset();
		return;
	}
	
	// else : push into result buffer for routing
	
	
	std::pair<gaspi_offset_t,TITUS_DLB_int> new_buffer_elt_offset;
	
	MetadataResult * r = ctx->get_metadata_result();
	// try remote alloc on self, if fail, push results buffer then restart
	do{
		new_buffer_elt_offset = r->try_remote_buffer_alloc(result_buffer_elt->buffer_entry_size(), ctx->segment_result, ctx->get_rank());
		//TITUS_DBG << "push_tmp_results reserved elt " << new_buffer_elt_offset.second << " on rank " << ctx->get_rank() << "(owner is rank " << owner_result_rank << ")" << std::endl; //TITUS_DBG.flush();
		if (new_buffer_elt_offset.first == (gaspi_offset_t)-1){
			TITUS_DBG << "WARNING : push_tmp_results : not enough place on result segment. Calling push_results_buffer" << std::endl; //TITUS_DBG.flush();
			ctx->push_results_buffer();
		}
	} while (new_buffer_elt_offset.first == (gaspi_offset_t)-1);
	
	// copy
	memcpy(ADD_PTR(r,new_buffer_elt_offset.first),result_buffer_elt_loc, result_buffer_elt->buffer_entry_size());

	//TITUS_DBG << "MetadataTmp::push_tmp_results : pushing results " << *result_buffer_elt << " to result segment as elt " << new_buffer_elt_offset.second << " @offset=" << new_buffer_elt_offset.first << std::endl; //TITUS_DBG.flush();
	gaspi_notification_id_t notification_id = new_buffer_elt_offset.second;

	//TITUS_DBG << "push_tmp_results : notified : segment id = " << (uint)r->segment_id << ", rank = " << ctx->get_rank() << ", notification_id = " << notification_id << ", value = " << 1 << ", queue = " << (uint)ctx->queue_result << ", timeout = " << TITUS_DLB_GASPI_TIMEOUT << std::endl;
	SUCCESS_OR_DIE( gaspi_notify(r->segment_id, ctx->get_rank(), notification_id, 1, ctx->queue_result, TITUS_DLB_GASPI_TIMEOUT) );
	reset();
}


MetadataTmp * MetadataTmp::read_to_scratch(gaspi_rank_t target_rank, gaspi_offset_t offset_in_scratch)const{
	gaspi_segment_id_t target_segment_id = this->segment_id;
	return read_to_scratch(target_rank, target_segment_id, offset_in_scratch);
}

MetadataTmp * MetadataTmp::read_to_scratch(gaspi_rank_t target_rank, gaspi_segment_id_t target_segment_id, gaspi_offset_t offset_in_scratch){
	if (target_segment_id == (gaspi_segment_id_t)-1)
		target_segment_id = TITUS_DLB_impl::get_context()->segment_task;
	gaspi_segment_id_t scratch_segment_id = TITUS_DLB_impl::get_context()->segment_scratch;
	gaspi_queue_id_t queue_scratch_id = TITUS_DLB_impl::get_context()->queue_scratch;
	SUCCESS_OR_DIE( gaspi_read( scratch_segment_id, offset_in_scratch, target_rank, target_segment_id, 0, sizeof(MetadataTmp), queue_scratch_id, TITUS_DLB_GASPI_TIMEOUT ) );
	SUCCESS_OR_DIE( gaspi_wait( queue_scratch_id, TITUS_DLB_GASPI_TIMEOUT ) );

	return (MetadataTmp*) ADD_PTR(TITUS_DLB_impl::get_context()->ptr_segment_scratch , offset_in_scratch);
}

std::ostream & operator<<(std::ostream & out, const MetadataTmp & arg) {arg.print(out); return out;}
