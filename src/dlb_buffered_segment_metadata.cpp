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


#include "TITUS_DLB_buffered_segment_metadata.hpp"


// *********************************************************************
// *************** buffered SEGMENT METADATA (BASE) ******************
// *********************************************************************

// BufferElt contains information about the buffer entry
// It is located at the beginning of the entry and is followed in memory by the data attached to this buffer element.
// Copying sizeof(BufferElt) + data_size adequetely copies the buffer entry as a whole.

// *** CONSTRUCTOR ***
BufferElt::BufferElt(TITUS_DLB_int first_task_id, TITUS_DLB_int nb_result, TITUS_DLB_int task_size, gaspi_rank_t owner):
	first_task_id(first_task_id), nb_result(nb_result), data_size(task_size * nb_result), owner(owner)
{}

// *** METHODS ***
size_t BufferElt::buffer_entry_size()const {
	return data_size + sizeof(BufferElt);
}

void BufferElt::print(std::ostream & out)const{
	out << "first_task_id=" << first_task_id;
    out << ", nb_result=" << nb_result;
    out << ", data_size=" << data_size;
    out << ", owner=" << owner;   
}
void BufferElt::print(std::ostream && out)const{
	out << "first_task_id=" << first_task_id;
    out << ", nb_result=" << nb_result;
    out << ", data_size=" << data_size;
    out << ", owner=" << owner;   
}

BufferElt* BufferElt::next()const{
	return (BufferElt*)ADD_PTR(this, buffer_entry_size());
}

bool BufferElt::is_valid()const{
	//if (first_task_id < 0) return false;
	if (nb_result == 0) return false;
	if (buffer_entry_size() > ((const TITUS_DLB_Context_impl *)TITUS_DLB_impl::get_context())->get_metadata_result()->get_size()) return false;
	if (data_size > ((const TITUS_DLB_Context_impl *)TITUS_DLB_impl::get_context())->get_metadata_result()->get_size()) return false;
	if (owner > TITUS_DLB_impl::get_context()->get_nb_ranks()) return false;
	if (data_size == 0) return false;
	return true;
}

// *** CONSTRUCTOR ***
BufferedSegmentMetadata::BufferedSegmentMetadata(size_t segment_size, gaspi_segment_id_t segment_id, size_t size_of_object):
	SegmentMetadata(segment_size,segment_id),segment_wipe_id(0), buffer_elt_count(0), pending_write_lists(new std::vector<write_list_args_t>())
{
	buffer_head = buffer_tail = (gaspi_offset_t)size_of_object;
}


BufferedSegmentMetadata * BufferedSegmentMetadata::read_to_scratch(gaspi_rank_t target_rank, gaspi_offset_t offset_in_scratch)const{
	gaspi_segment_id_t target_segment_id = this->segment_id;
	return read_to_scratch(target_rank, target_segment_id, offset_in_scratch);
}

BufferedSegmentMetadata * BufferedSegmentMetadata::read_to_scratch(gaspi_rank_t target_rank, gaspi_segment_id_t target_segment_id, gaspi_offset_t offset_in_scratch){
	if (target_segment_id == (gaspi_segment_id_t)-1)
		target_segment_id = TITUS_DLB_impl::get_context()->segment_task;
	gaspi_segment_id_t scratch_segment_id = TITUS_DLB_impl::get_context()->segment_scratch;
	gaspi_queue_id_t queue_scratch_id = TITUS_DLB_impl::get_context()->queue_scratch;
	SUCCESS_OR_DIE( gaspi_read( scratch_segment_id, offset_in_scratch, target_rank, target_segment_id, 0, sizeof(BufferedSegmentMetadata), queue_scratch_id, TITUS_DLB_GASPI_TIMEOUT ) );
	SUCCESS_OR_DIE( gaspi_wait( queue_scratch_id, TITUS_DLB_GASPI_TIMEOUT ) );

	return (BufferedSegmentMetadata*) ADD_PTR(TITUS_DLB_impl::get_context()->ptr_segment_scratch , offset_in_scratch);
}

// *** helpers & printers ***

std::string BufferedSegmentMetadata::state_str(gaspi_atomic_value_t value)const{
	switch (value){
		case SEGMENT_AVAILABLE : return "SEGMENT_AVAILABLE";
		case COPY_IN_PROGRESS : return "COPY_IN_PROGRESS";
		default : std::stringstream a(""); a << "SEGMENT LOCKED BY RANK " << value; return a.str();
	}
}
std::string BufferedSegmentMetadata::state_str()const{
	return state_str(state);
}

std::ostream & operator<<(std::ostream & out, const BufferElt & arg){
	arg.print(out);
	return out;
}

BufferEltIterator BufferedSegmentMetadata::begin() const{
	return (BufferElt*) ADD_PTR(this, buffer_tail);
}
BufferEltIterator BufferedSegmentMetadata::end() const{
	return (BufferElt*) ADD_PTR(this, buffer_head);
}

//typedef std::map<gaspi_rank_t,elts_collection> buffer_elts_by_dest;
BufferedSegmentMetadata::buffer_elts_by_dest & operator <<(BufferedSegmentMetadata::buffer_elts_by_dest & arg, BufferedSegmentMetadata::buffer_elts_by_dest::value_type const & val){
	if (! val.second.second->is_valid()){
		TITUS_DBG << "Error : ASSERT IS GOING TO FAIL : invalid buffer element : " << val.second.second << std::endl;
		usleep (100000);
		ASSERT(val.second.second->is_valid());
	}
	arg.insert(val);
	return arg;
}
BufferedSegmentMetadata::buffer_elts_by_dest & operator <<(BufferedSegmentMetadata::buffer_elts_by_dest & arg, BufferedSegmentMetadata::buffer_elts_by_dest::mapped_type const & val){
	return arg << BufferedSegmentMetadata::buffer_elts_by_dest::value_type((val.second->owner != TITUS_DLB_impl::get_context()->get_rank() ? DVS::select_next_hop_to(val.second->owner) : val.second->owner) ,val);
}
template<class iterator_type>
BufferedSegmentMetadata::buffer_elts_by_dest & operator << (BufferedSegmentMetadata::buffer_elts_by_dest & arg, std::pair<iterator_type, iterator_type> const & val){
	for (auto it = val.first ; it != val.second; it ++){
		arg << *it;
	} 
	return arg;
}


std::ostream & operator << (std::ostream & out , BufferedSegmentMetadata::selected_elts_to_push const & arg){
	return out << "elt_count=" << arg.elt_count << ", to_reserve=" << arg.to_reserve << ", size=" << arg.selected.size();
}

// *** methods & algorithms ***

// waits for the notification corresponding to the elt_index-th element of the buffered segment
// returns the corresponding notification value. 
gaspi_notification_t BufferedSegmentMetadata::wait_notification_for_elt(int elt_index)const{
	ASSERT(elt_index <= MAX_BUFFER_ELTS);
	gaspi_notification_id_t dummy = -1;
	uint64_t wait_start = rdtsc();
	while (true){
		gaspi_return_t retval = gaspi_notify_waitsome(segment_id, elt_index, 1, & dummy, 100);
		if (retval == GASPI_SUCCESS) break;
		if (retval == GASPI_ERROR) SUCCESS_OR_DIE(retval);
		ASSERT(retval == GASPI_TIMEOUT);
		//print_stacktrace_and_quit(-1); //uncomment to generate error, stacktrace and library state
		TITUS_DBG << "BufferedSegmentMetadata::wait_notification_for_elt : [WARNING] waiting for notification for elt " << elt_index << " for " << (rdtsc() - wait_start) / (TITUS_PROC_FREQ/1000) << "ms, seg_id = " << (uint)segment_id << std::endl;
		assert ((rdtsc() - wait_start) / (TITUS_PROC_FREQ/1000) < 10000); // abort after 10secs
	}
	ASSERT(dummy == elt_index);
	gaspi_notification_t val;
	gaspi_notify_reset(segment_id, elt_index, &val);
	//~ TITUS_DBG << "BufferedSegmentMetadata::wait_notification_for_elt : recieved notification for " << val << " elements, starting at index " << elt_index << std::endl;

	return val;
}

void BufferedSegmentMetadata::wait_notifications_for_all_elts()const{
	for (int i = 0; i < buffer_elt_count; i+=wait_notification_for_elt(i));
}

// try pushing all elts of buffer and local_transiting_results to next hop or submit results if dest = local rank
// by default results that need to be sent to another process are sent to the same segment id.
void BufferedSegmentMetadata::try_flush_buffer_elts_packed(){
	//~ TITUS_DBG << "in BufferedSegmentMetadata::try_flush_buffer_elts_packed" << std::endl;

	if (is_empty()) return; //! todo : take local_transiting_results.size() into acount
	if (! (buffer_head < segment_size)){
		TITUS_DBG << "ASSERT IS GOING TO FAIL : BufferedSegmentMetadata::try_flush_buffer_elts_packed > invalid buffer head : " << buffer_head << ">= " << segment_size << std::endl; //TITUS_DBG.flush();
		ASSERT(buffer_head < segment_size);
	}
	TITUS_DLB_Context_impl * ctx = TITUS_DLB_impl::get_context();
    // lock local segment
    uint64_t start_try_lock = rdtsc();
    gaspi_atomic_value_t old_value = 0;
	int count = 0; // for debugging purpose
    while (1){
		count ++;
        old_value = compare_and_swap_status(ctx->get_rank(), this->segment_id, SEGMENT_AVAILABLE, COPY_IN_PROGRESS);
        if (old_value == SEGMENT_AVAILABLE) break;
    }
    uint64_t lock_acquired = rdtsc();
	//~ TITUS_DBG << "BufferedSegmentMetadata::try_flush_buffer_elts_packed : local segment locked for flush after " << (lock_acquired - start_try_lock) / (TITUS_PROC_FREQ / 1e6) << " µs, segment occupation = " << buffer_head << "/" << segment_size << " (" << buffer_elt_count << " elements in buffer)" << std::endl;
	wait_notifications_for_all_elts();
	
	// agregate all locally available results into local_transiting_results.
	for (auto seg_it = begin(); seg_it != end() ; seg_it ++){
		local_transiting_results << buffer_elts_by_dest::mapped_type(segment_id, &(*seg_it));
	}
	
	uint failed_push = 0; // increments each time the first element of local_transiting_results is the same than at last call else resets
	if (local_transiting_results.size() > 0){
		static TITUS_DLB_int last_first_task_id = local_transiting_results.begin()->second.second->first_task_id;
		TITUS_DLB_int first_task_id = local_transiting_results.begin()->second.second->first_task_id;
		if (first_task_id == last_first_task_id) failed_push ++;
		else failed_push = 0;
	}
	else failed_push = 0;
	if (failed_push > 1000){
		gaspi_rank_t target = DVS::select_next_hop_to(local_transiting_results.begin()->second.second->owner);
		TITUS_DBG << "ASSERT IS GOING TO FAIL : failed_push > 1000."
				  << "lets have a look at the remote segment @rank " << (uint)target
				  << " on route to " << local_transiting_results.begin()->second.second->owner << std::endl; //TITUS_DBG.flush();
		TITUS_DBG << *read_to_scratch(target);
		ASSERT(failed_push <= 1000);
	}
	
	//! TODO : cleanup
	//TITUS_DBG << "BufferedSegmentMetadata::try_flush_buffer_elts_packed() : agregated local_transiting_results : " << std::endl;
	//print_local_transiting_results(TITUS_DBG);
	
	auto begin_range = local_transiting_results.begin() ;
	auto end_range = local_transiting_results.upper_bound(begin_range->first);
	while (begin_range != local_transiting_results.end()){
		gaspi_rank_t target = begin_range->first;
		end_range = local_transiting_results.upper_bound(target);
		
		if (target == ctx->get_rank()){ // if destination is me
			for (auto it = begin_range ; it != end_range ; ){
				ASSERT(it->second.first == segment_id); // results are on local segment (do not free them, segment will be wiped clean)
				ASSERT(it->second.second->is_valid()); // results are valid
				ctx->submit_results(it->second.second);
				auto tmp = it; tmp++;
				local_transiting_results.erase(it); 
				it = tmp;
			}
			begin_range = end_range;
			continue;
		}
		
		// tauto check
		if (TITUS_DLB_impl::get_context()->registrations_status[target] != true){
			TITUS_DBG << "ERROR RANK " << TITUS_DLB_impl::get_context()->get_rank() << " : ASSERT IS GOING TO FAIL : " <<
			"registrations_status[" << target << "] != true" << std::endl; //TITUS_DBG.flush();
			ASSERT(TITUS_DLB_impl::get_context()->registrations_status[target] == true);
		}

		// push elements
		selected_elts_to_push res = try_push_vector_elt_to(begin_range, end_range, target, ctx->segment_result);
		// put aside elements that are on the segment and were not sent, remove the rest from the map
		int i = 0;
		for (auto it = begin_range; it != end_range ; ){
			if (res.size() == 0 || res.selected[i] == false){ // item has not been sent
				if (it->second.first == (gaspi_segment_id_t)-1){ // item was in local memory
					// keep the item there : do nothing
				}
				else{ // item was on segment
					BufferElt * elt_on_segment = it->second.second;
					ASSERT(elt_on_segment->is_valid());
					//TITUS_DBG << "try_flush_buffer_elts_packed : could not reserve memory at rank " << target << ", putting aside :" << *elt_on_segment << std::endl;

					BufferElt* new_elt = (BufferElt*) malloc(elt_on_segment->buffer_entry_size());
					ASSERT(new_elt != nullptr);

					memcpy(new_elt,elt_on_segment,elt_on_segment->buffer_entry_size());
					it->second.second = new_elt; // make map element point to the newly allocated BufferElt
					it->second.first = (gaspi_segment_id_t)-1; // new BufferElt is not on segment
				}
				it ++;
			}
			else { // item has been sent
				// remove item from map
				//TITUS_DBG << "try_flush_buffer_elts_packed : pushed elt : " << *it->second.second << " @target=" << target << std::endl;
				if (it->second.first == (gaspi_segment_id_t)-1){ // item was in local memory
					// delete item from mallocated memory
					free(it->second.second);
				}
				else{ // item was on segment
					// segment will be wiped clean : do nothing
				}
				auto tmp = it; tmp++;
				local_transiting_results.erase(it);
				it = tmp;
			}
			i++;
		}
		begin_range = end_range;
	}
	gaspi_wait(ctx->queue_result,TITUS_DLB_GASPI_TIMEOUT);
	
	// gaspi_wait
	uint64_t wait_start = rdtsc();
	while (true){
		gaspi_return_t retval = gaspi_wait(ctx->queue_result, 1000); 
		if (retval == GASPI_SUCCESS) break;
		if (retval != GASPI_TIMEOUT) SUCCESS_OR_DIE(retval);
		TITUS_DBG << "BufferedSegmentMetadata::try_flush_buffer_elts : WARNING : waiting for queue to flush for " << (rdtsc() - wait_start) / (TITUS_PROC_FREQ/1000) << "ms" << std::endl;
	}


    purge();
    // unlock local buffer
    ASSERT( compare_and_swap_status(ctx->get_rank(), segment_id, COPY_IN_PROGRESS, SEGMENT_AVAILABLE, true) == COPY_IN_PROGRESS );
	//~ TITUS_DBG << "BufferedSegmentMetadata::try_flush_buffer_elts_packed : done in " << (rdtsc() - start_try_lock) / (TITUS_PROC_FREQ / 1e3) << " ms." << std::endl;
}


// try reserving enough memory to send all results iterated within [begin:end[. Distant segment must be either locked or inaccessible by other processes or threads.
// the returned vector of boolean value, which length is equal to the length of the elt_vec argument, represents whether or not each element of the elt_vec vector has been successfully sent to the remote segment.
//! does not lock nor unlock the remote segment
//! does call gaspi_wait. local memory for corresponding buffer elements shall not be modified until a successfull call to gaspi_wait on the results queue.
template <class InputIterator> // some class of iterator "pointing" to std::pair<segment_id_t, BufferElt*>
BufferedSegmentMetadata::selected_elts_to_push BufferedSegmentMetadata::select_elts_to_push(InputIterator begin, InputIterator end, size_t available_size, size_t max_elts_count){
	TITUS_DLB_Context_impl * ctx = TITUS_DLB_impl::get_context();

	selected_elts_to_push r;
	for (auto it = begin ; it != end ; it++){
		BufferElt * elt_ptr = it->second.second;
		ASSERT(elt_ptr->is_valid());
		if (available_size - r.to_reserve > elt_ptr->buffer_entry_size() && r.elt_count < max_elts_count){
			r.to_reserve += elt_ptr->buffer_entry_size();
			r.elt_count ++;
			r.selected.push_back(true);
			continue;
		}
		else {
			r.selected.push_back(false);
		}
	}
	if (! (r.to_reserve < available_size)){
		TITUS_DBG << "ASSERT IS GOING TO FAIL : BufferedSegmentMetadata::try_push_vector_elt_to > invalid memory reservation value : " << r.to_reserve << ">= " << available_size << std::endl; //TITUS_DBG.flush();
		ASSERT(r.to_reserve < available_size);
	}
	//! TODO : check which is actually faster
	//return r;
	return std::move(r);
}

// try pushing all results iterated within [begin:end[ to the remote segment dst_seg_id on target rank
// all BufferElt for which memory is been reserved are then sent to the remote buffered segment using a sigle gaspi_write_list
// only one notification is issued to the remote rank : 
//  - notification id is the index of the first remotely allocated BufferElt on the destination buffered segment
//  - notification value is the the number of BufferElt remotely allocated and sent.
template <class InputIterator> // some class of iterator "pointing" to std::pair<segment_id_t, BufferElt*>
BufferedSegmentMetadata::selected_elts_to_push BufferedSegmentMetadata::try_push_vector_elt_to(InputIterator begin, InputIterator end, gaspi_rank_t target, gaspi_segment_id_t dst_seg_id) {
	if (begin == end) return selected_elts_to_push();
	
	//TITUS_DBG << "entered try_push_vector_elt_to" << std::endl; TITUS_DBG.flush();
	TITUS_DLB_Context_impl * ctx = TITUS_DLB_impl::get_context();

	// #################### try lock remote segment ####################
	gaspi_atomic_value_t old_status_value = compare_and_swap_status(target, ctx->segment_result, SEGMENT_AVAILABLE, ctx->get_rank());
	if (old_status_value != SEGMENT_AVAILABLE){
		// failed to lock remote segment : return empty results
		TITUS_DBG << "BufferedSegmentMetadata::try_push_vector_elt_to : failed to lock segment " << (uint) dst_seg_id << " on rank " << (uint)target << std::endl;
		return selected_elts_to_push();
	}
	uint64_t lock_start = rdtsc();
	
	BufferedSegmentMetadata * tmp = read_to_scratch(target); // read remote state to scratch
	// print remote buffer state
	//! TODO : cleanup
	// TITUS_DBG << "try_push_vector_elt_to : locked remote segment : rank=" << target << ", current_head=" << tmp->buffer_head << ", wipe_id=" << tmp->segment_wipe_id << std::endl; TITUS_DBG.flush();
	BufferedSegmentMetadata * remote_bsm = (BufferedSegmentMetadata *) malloc(sizeof(BufferedSegmentMetadata)); ASSERT(remote_bsm != nullptr); memcpy((void*)remote_bsm,tmp,sizeof(BufferedSegmentMetadata)); 
	// create a local mem copy to free up scratch space. consider tmp gone.
	size_t available = remote_bsm->segment_size > remote_bsm->buffer_head ? remote_bsm->segment_size - remote_bsm->buffer_head : 0;
	
	selected_elts_to_push r = select_elts_to_push(begin, end, available, MAX_BUFFER_ELTS - remote_bsm->buffer_elt_count);
	
	if (r.elt_count != 0) {
		// perform remote memory reservation
		
		gaspi_atomic_value_t value_old;
		SUCCESS_OR_DIE( gaspi_atomic_fetch_add(dst_seg_id, offsetof(typeofthis, buffer_head), target, r.to_reserve, &value_old, TITUS_DLB_GASPI_TIMEOUT) );
		ASSERT(value_old == remote_bsm->buffer_head);

		SUCCESS_OR_DIE( gaspi_atomic_fetch_add(dst_seg_id, offsetof(typeofthis, buffer_elt_count), target, r.elt_count, &value_old, TITUS_DLB_GASPI_TIMEOUT) );
		ASSERT(value_old == remote_bsm->buffer_elt_count);
	}
	
	
	// ##################### unlock remote segment #####################
	old_status_value = compare_and_swap_status(target, ctx->segment_result, ctx->get_rank(), SEGMENT_AVAILABLE, true);
	if (old_status_value != ctx->get_rank()){
		TITUS_DBG << "BufferedSegmentMetadata::try_push_vector_elt_to : ASSERT IS GOING TO FAIL : BufferedSegmentMetadata::try_remote_buffer_alloc > unlock failed : " << state_str(old_status_value) << "!= " << state_str(ctx->get_rank()) << std::endl; //TITUS_DBG.flush();
		ASSERT(old_status_value == ctx->get_rank());
	}
	uint64_t lock_end = rdtsc();

	TITUS_DBG << "BufferedSegmentMetadata::try_push_vector_elt_to : reserved=" << r.to_reserve << " , target=" << target << " elt_count=" << r.elt_count << ", vec_size=" << r.selected.size() << " elements, starting at index " << remote_bsm->buffer_elt_count << ", offset=" << (uint)remote_bsm->buffer_head << std::endl;

	if (r.elt_count == 0) return r;
    // else construct gaspi_write_list arguments
    write_list_args_t args(r.elt_count, target);
	gaspi_offset_t offset_remote_i = remote_bsm->buffer_head;
	int i = 0, elt_i = 0;
	gaspi_offset_t scratch_segment_head = sizeof(BufferedSegmentMetadata);
	size_t results_pushed_count = 0;
	const void * scratch_segment_ptr = ctx->ptr_segment_scratch;
	for (auto it = begin; it != end ; it ++){
		if (r.selected[i]){
			auto elt_ptr = it->second.second;
			ASSERT(elt_ptr->is_valid());
			// local data location information : depends on wether the data is on segment or in local memory
			if (it->second.first == (gaspi_segment_id_t)-1){
				// copy element to scratch segment for remote write
				BufferElt * new_elt_on_scratch = (BufferElt*)ADD_PTR(scratch_segment_ptr,scratch_segment_head);
				gaspi_offset_t new_elt_offset = scratch_segment_head;
				scratch_segment_head += elt_ptr->buffer_entry_size();
				memcpy(new_elt_on_scratch, elt_ptr, elt_ptr->buffer_entry_size());
				
				args.segment_id_local[elt_i] = ctx->segment_scratch;
				args.offset_local[elt_i] = new_elt_offset;
			}
			else{
				args.segment_id_local[elt_i] = it->second.first;
				args.offset_local[elt_i] = DIFF_PTR(elt_ptr, this);
			}
			// remote data location information
			args.segment_id_remote[elt_i] = dst_seg_id;
			args.offset_remote[elt_i] = offset_remote_i;
			args.size[elt_i] = elt_ptr->buffer_entry_size();
			offset_remote_i += args.size[elt_i];
			elt_i ++;
			// increment results count
			results_pushed_count += elt_ptr->nb_result;
		}
		i++;
	}
	// offset_remote_i should now match the previous reservation evaluation
	if (! (r.elt_count == elt_i)){
		TITUS_DBG << "ASSERT IS GOING TO FAIL : BufferedSegmentMetadata::try_push_vector_elt_to > invalid mem reservation value : r.elt_count = " << r.elt_count  << ", " << r.elt_count  << "!= " << elt_i << std::endl; //TITUS_DBG.flush();
		ASSERT(r.elt_count == elt_i);
	}
	if (! (remote_bsm->buffer_head + r.to_reserve == offset_remote_i)){
		TITUS_DBG << "ASSERT IS GOING TO FAIL : BufferedSegmentMetadata::try_push_vector_elt_to > invalid mem reservation value : r.to_reserve = " <<  r.to_reserve << ", " << remote_bsm->buffer_head + r.to_reserve << "!= " << offset_remote_i << std::endl; //TITUS_DBG.flush();
		ASSERT(remote_bsm->buffer_head + r.to_reserve == offset_remote_i);
	}
	
	
	// enqueue remote writes
	for (size_t i=0 ; i<args.elt_count;i+=254){
		wait_if_queue_full(ctx->queue_result, args.elt_count +1);
		gaspi_number_t 			elt_count_iter 			= std::min(args.elt_count - i, (size_t)254);
		gaspi_segment_id_t * 	segment_id_local_iter 	= args.segment_id_local + i;
		gaspi_offset_t * 		offset_local_iter 		= args.offset_local + i;
		gaspi_segment_id_t * 	segment_id_remote_iter 	= args.segment_id_remote + i;
		gaspi_offset_t * 		offset_remote_iter 		= args.offset_remote + i;
		gaspi_size_t * 			size_iter 				= args.size + i;
		
		SUCCESS_OR_DIE( gaspi_write_list(
			elt_count_iter,
			segment_id_local_iter, offset_local_iter,
			args.target, segment_id_remote_iter, offset_remote_iter, size_iter,
			ctx->queue_result, TITUS_DLB_GASPI_TIMEOUT) );
	}
	gaspi_notification_id_t notify_id = remote_bsm->buffer_elt_count; // index of first element sent
	gaspi_notification_t notify_val = r.elt_count; // # of elts sent
	SUCCESS_OR_DIE( gaspi_notify(
		dst_seg_id, args.target, notify_id, notify_val,
		ctx->queue_result, TITUS_DLB_GASPI_TIMEOUT) );
	//~ TITUS_DBG << "BufferedSegmentMetadata::try_push_vector_elt_to : issued notification for " << notify_val << " elements, starting at index " << notify_id << std::endl;

	pending_write_lists->push_back(std::move(args));
	
	free (remote_bsm);
	
	ctx->get_logger()->signal_results_pushed(results_pushed_count);
	//! TODO : move gaspi_wait outside of the lock (NOT TRIVIAL)
	SUCCESS_OR_DIE( gaspi_wait(ctx->queue_result, TITUS_DLB_GASPI_TIMEOUT) );
	return std::move(r);
}


//! TODO : OLD AND SHOULD BE DEPRECATED.
//! piecewise remote memory reservation is inneficient. should use try_push_vector_elt_to
// allocate remote memory in rank target, in buffered segment seg_id
// returns the offset at witch memory has been allocated or nullptr if no memory was available as the first field of the pair
// and the element id of the reserved memory area in the distant buffered segment.
// segment_id must be created as such
// try_remote_buffer_alloc does not modify any local value beside scratch segment.
std::pair<gaspi_offset_t,gaspi_atomic_value_t> BufferedSegmentMetadata::try_remote_buffer_alloc(size_t bytes, gaspi_segment_id_t seg_id, gaspi_rank_t target)const {
    TITUS_DLB_Context_impl * ctx = TITUS_DLB_impl::get_context();
    
    uint64_t try_remote_buffer_alloc_start = rdtsc(); // TODO : cleanup
    
    gaspi_atomic_value_t old_status_value = 0;
	
    // try lock until locked or nb_try
	std::pair<gaspi_offset_t,gaspi_atomic_value_t> ret(-1,0);
	
	// lock segment using rank as lock value if remote rank is available
	old_status_value = compare_and_swap_status(target, seg_id, SEGMENT_AVAILABLE, ctx->get_rank());
	if (old_status_value != SEGMENT_AVAILABLE){
		TITUS_DBG << "BufferedSegmentMetadata::try_remote_buffer_alloc : failed to lock segment " << (uint) seg_id << " on rank " << (uint)target << std::endl;
		// failed to lock segment
		return ret; //(-1,0)
	}
	//else : remote segment is locked
    gaspi_atomic_value_t reserved_memory = 0;
    
	// read remote buffer state
	BufferedSegmentMetadata * tmp = read_to_scratch(target); // read remote state to scratch
	//! TODO : cleanup
	// TITUS_DBG << "try_push_vector_elt_to : locked remote segment : rank=" << target << ", current_head=" << tmp->buffer_head << ", wipe_id=" << tmp->segment_wipe_id << std::endl; TITUS_DBG.flush();
	BufferedSegmentMetadata * remote_bsm = (BufferedSegmentMetadata *) malloc(sizeof(BufferedSegmentMetadata)); ASSERT(remote_bsm != nullptr); memcpy((void*)remote_bsm,tmp,sizeof(BufferedSegmentMetadata)); 

	// reserve memory
	gaspi_atomic_value_t old_head_value, distant_buffer_elt_count;
	if (remote_bsm->buffer_head + bytes < remote_bsm->segment_size && remote_bsm->buffer_elt_count < MAX_BUFFER_ELTS){
		// using fetch & add
		SUCCESS_OR_DIE( gaspi_atomic_fetch_add(seg_id, offsetof(typeofthis, buffer_head), target, bytes, &old_head_value, TITUS_DLB_GASPI_TIMEOUT) );
		SUCCESS_OR_DIE( gaspi_atomic_fetch_add(seg_id, offsetof(typeofthis, buffer_elt_count), target, 1, &distant_buffer_elt_count, TITUS_DLB_GASPI_TIMEOUT) );
		reserved_memory = bytes;
		//fprintf(stderr, " > successfully reserved %lx bytes on rank %d\n", bytes, target);
	}
	else {
		//fprintf(stderr, " > failed to reserve %lx bytes on rank %d\n", bytes, target);
	}
	// else : not enough space , reserved_memory = 0
	
	// unlock remote segment
	old_status_value = compare_and_swap_status(target, seg_id, ctx->get_rank(), SEGMENT_AVAILABLE, true);
	if (old_status_value != ctx->get_rank()){
		TITUS_DBG << "ASSERT IS GOING TO FAIL : BufferedSegmentMetadata::try_remote_buffer_alloc > unlock failed : " << state_str(old_status_value) << "!= " << state_str(ctx->get_rank()) << std::endl; //TITUS_DBG.flush();
		ASSERT(old_status_value == ctx->get_rank());
	}
	
	// make response
	if (reserved_memory != 0) ret.first = old_head_value;
	else ret.first = (gaspi_offset_t) -1;
	ret.second = distant_buffer_elt_count;

    uint64_t try_remote_buffer_alloc_end = rdtsc(); // TODO : cleanup
	//~ TITUS_DBG << "BufferedSegmentMetadata::try_remote_buffer_alloc : done (reserved " << bytes << " B on segment " << (uint) seg_id << " on rank " << (uint)target << ") in " << ((double)(try_remote_buffer_alloc_end-try_remote_buffer_alloc_start)) / (double)(TITUS_PROC_FREQ / 1e3) << "ms" << std::endl;
	
	return ret;
}


//! OLD & DEPRECATED
// push all elts of buffer to next hop or submit results if dest = local rank
// by default results that need to be sent to another process are sent to the same segment id.
void BufferedSegmentMetadata::try_flush_buffer_elts() {
	if (is_empty()) return;

	TITUS_DLB_Context_impl * ctx = TITUS_DLB_impl::get_context();
	//fprintf(stderr,"rank %ld : BufferedSegmentMetadata::try_flush_buffer_elts(%d) > head = %lx, tail = %lx\n", ctx->get_rank(), segment_id, buffer_head, buffer_tail);
    
    // lock local segment
    gaspi_atomic_value_t old_value = 0;
	int count = 0; // for debugging purpose
    while (1){
		count ++;
        old_value = compare_and_swap_status(ctx->get_rank(), this->segment_id, SEGMENT_AVAILABLE, COPY_IN_PROGRESS);
        if (old_value == SEGMENT_AVAILABLE) break;
    }


	bool gaspi_wait_required = false; // set to true if at least 1 gaspi_write has been sent
	int i = 0;
    // copy all buffer entries, either to a distant segment using remote write, or to a local buffer.
    for (auto it = begin(); it < end(); it = it->next()) {
		ASSERT (i < buffer_elt_count);
		//TITUS_DBG << "gaspi_notify_waitsome(segment_id=" << (uint)segment_id << ", i=" << (uint)i << ", 1, & dummy, TITUS_DLB_GASPI_TIMEOUT)" << std::endl; //TITUS_DBG.flush();

		//TITUS_DBG << "gaspi_notify_reset(segment_id=" << (uint)segment_id << ", i=" << i << ", &val);" << std::endl; //TITUS_DBG.flush();
		
		//// wait for notification, ensuring communication has arrived ////
		ASSERT(wait_notification_for_elt(i) == 1);
		
		// tauto check
		if ( ! it -> is_valid() ){
			TITUS_DBG << "ASSERT IS GOING TO FAIL : try_flush_buffer_elts > error wrong bufferelt (" << i << ") : " << *it << std::endl; //TITUS_DBG.flush();
			ASSERT( it -> is_valid() );
		}

        if (it->owner != ctx->get_rank()){
			// push to next remote results segment in route
			gaspi_rank_t dest_rank =  DVS::select_next_hop_to(it->owner);
			
			if (TITUS_DLB_impl::get_context()->registrations_status[dest_rank] != true){
				TITUS_DBG << "ERROR RANK " << TITUS_DLB_impl::get_context()->get_rank() << " : ASSERT IS GOING TO FAIL : " <<
				"registrations_status[" << dest_rank << "] != true while trying to push element " << *it << std::endl; //TITUS_DBG.flush();
			}
			ASSERT(TITUS_DLB_impl::get_context()->registrations_status[dest_rank] == true);

			// try_push_buffer_elt_to
			if (try_push_buffer_elt_to(&(*it), dest_rank, ctx->segment_result) == false){
				// if fail, store results in local memory to try sending them again later
				//TITUS_DBG << "could not return [" << *it << "] to rank " << dest_rank << ". putting results aside" << std::endl; //TITUS_DBG.flush();
				BufferElt* new_elt = (BufferElt*) malloc(it->buffer_entry_size());
				ASSERT(new_elt != nullptr);
				memcpy(new_elt,&(*it),it->buffer_entry_size());
				local_transiting_results << buffer_elts_by_dest::value_type(dest_rank, buffer_elts_by_dest::mapped_type(-1, new_elt));
			}
			else {
				gaspi_wait_required = true;
			}
		}
        else {
            // push to local results
            //TITUS_DBG << "try_flush_buffer_elts : found results to submit in elt " << i << " : " << *it << std::endl; //TITUS_DBG.flush();
            ctx->submit_results(&(*it));
        }
        i++;
    }
	if (i != buffer_elt_count){
		TITUS_DBG << "ERROR : ASSERT IS GOING TO FAIL : " << i << " != " << buffer_elt_count << std::endl; //TITUS_DBG.flush();
	}
	ASSERT (i == buffer_elt_count);
	
	//TODO : optimize : hide this wait by using an other segment or flip-flop buffer (it may be a bottleneck to both work and result flow)
	if (gaspi_wait_required){
		uint64_t wait_start = rdtsc();
		while (true){
			gaspi_return_t retval = gaspi_wait(ctx->queue_result, 1000); 
			if (retval == GASPI_SUCCESS) break;
			if (retval != GASPI_TIMEOUT) SUCCESS_OR_DIE(retval);
			TITUS_DBG << "BufferedSegmentMetadata::try_flush_buffer_elts : WARNING : waiting for queue to flush for " << (rdtsc() - wait_start) / (TITUS_PROC_FREQ/1000) << "ms" << std::endl;
		}
		//TITUS_DBG << "BufferedSegmentMetadata::try_flush_buffer_elts gaspi_wait returned SUCCESS" << std::endl;
	}

    purge();
    // unlock local buffer
    bool unlock_success = compare_and_swap_status(ctx->get_rank(), segment_id, COPY_IN_PROGRESS, SEGMENT_AVAILABLE, true) == COPY_IN_PROGRESS;
    ASSERT(unlock_success);
}

//! OLD & DEPRECATED
// push the buffer elt and its associated subsequent data into a distant buffered segment.
// does not wait for the communication to complete. One must wait for queue context->queue_result for completion before releasing the memory used by the segment.
//! TODO : optimise by chunking buffer elts that go to the same remote rank || send_multiple_buffer_elt_to (using gaspi_write_list)
bool BufferedSegmentMetadata::try_push_buffer_elt_to(gaspi_pointer_t buffer_elt, gaspi_rank_t target, gaspi_segment_id_t dst_seg_id) const{

    BufferElt *e = (BufferElt*)buffer_elt;

    // check BufferElt validity
    //! TODO : cleanup
    TITUS_DLB_Context_impl * ctx = TITUS_DLB_impl::get_context();
    if (e->owner >= ctx->get_nb_ranks()){
		TITUS_DBG << e;
	}

    gaspi_offset_t src_offset = DIFF_PTR(buffer_elt, this);

    std::pair<gaspi_offset_t,gaspi_atomic_value_t> dest_offset = try_remote_buffer_alloc(e->buffer_entry_size(), dst_seg_id, target);
    
    if (dest_offset.first != (gaspi_offset_t) -1) {
		//TITUS_DBG << "BufferedSegmentMetadata::try_push_buffer_elt_to : reserved elt " << dest_offset.second << " on rank " << target << " : " << *e << std::endl; //TITUS_DBG.flush();
        //TITUS_DBG
		//	<< " : gaspi_write_notify( segment_id_local=" << (uint)segment_id << ", offset_local="<< src_offset << ", target="<< target << ", seg_id_remote=" << (uint)dst_seg_id << ", offset_remote=" << dest_offset.first
		//	<< ", size()=" << e->buffer_entry_size() << ", notification_id=" << dest_offset.second << ", 1, ctx->queue_result, TITUS_DLB_GASPI_TIMEOUT );" << std::endl; //TITUS_DBG.flush();
        gaspi_notification_id_t notification_id = dest_offset.second;
        wait_if_queue_full(ctx->queue_result,0);
        SUCCESS_OR_DIE(gaspi_write_notify( segment_id, src_offset, target, dst_seg_id, dest_offset.first, (gaspi_size_t)e->buffer_entry_size(), notification_id, 1, ctx->queue_result, TITUS_DLB_GASPI_TIMEOUT ));
        ctx->get_logger()->signal_results_pushed(e->nb_result);
        //TITUS_DBG << "BufferedSegmentMetadata::try_push_buffer_elt_to > "
		//	<< "successfuly sent elt[" << *e << "] to segment " << (int)dst_seg_id << " on rank " << target << " as elt " << dest_offset.second << " @offset=" << dest_offset.first << std::endl; //TITUS_DBG.flush();
        return true;
        //~ SUCCESS_OR_DIE( gaspi_atomic_fetch_add (ctx->segment_result, 0, t->owner_result_rank, nb_tasks, &value_old, TITUS_DLB_GASPI_TIMEOUT));
        //~ DEBUG_PRINT("gaspi_atomic_fetch_add (seg_src[%llu], off[0], rank[%llu], nb_result[%llu], value_old[%llu]))\n",ctx->segment_result, t->owner_result_rank, t->nb_result, value_old);
    }
    else {
        //! todo : how to trigger remote buffer purge ?
        return false;
    }
}


//! OLD & DEPRECATED
void BufferedSegmentMetadata::try_push_local_transiting_results(){
	if (local_transiting_results.size() == 0) return;

    TITUS_DLB_Context_impl * ctx = TITUS_DLB_impl::get_context();
	
	BufferElt * loc_on_scratch = (BufferElt*)ctx->ptr_segment_scratch;
	bool wait_required = false;
	//print_local_transiting_results(TITUS_DBG); //TITUS_DBG.flush();
	//std::cerr << "rank " << get_rank() << " : TITUS_DBG = " << &TITUS_DBG << std::endl;
	int count = 0;
	for (auto it = local_transiting_results.begin(); it != local_transiting_results.end(); it++){
		BufferElt * e = it->second.second;
		ASSERT(e->is_valid())
		
		
		gaspi_rank_t target_rank = it->first;
		
		// if not enough place for elt on scratch buffer, jump to next.
		if ( (DIFF_PTR(loc_on_scratch, ctx->ptr_segment_scratch))+ e->buffer_entry_size() >= ctx->shared_scratch_segment_size ){
			//TITUS_DBG << "BufferedSegmentMetadata::try_push_local_transiting_results : " << *e << " : not nough place on scratch "
			//			<< "(" << DIFF_PTR(loc_on_scratch, ptr_segment_scratch) << "/" << shared_scratch_segment_size << ")" << std::endl; //TITUS_DBG.flush();
			count ++;
			continue;
		}
		
		
		// try lock
		//std::pair<gaspi_offset_t,gaspi_atomic_value_t> alloc_res = r->try_remote_buffer_alloc(e->buffer_entry_size(), segment_result, target_rank);
		auto alloc_res = try_remote_buffer_alloc(e->buffer_entry_size(), ctx->segment_result, target_rank);
		if (alloc_res.first == -1){
			//TITUS_DBG << " BufferedSegmentMetadata::try_push_local_transiting_results [" << count << "]" << *e << " failed" << std::endl; //TITUS_DBG.flush();
			// alloc failed, jump to next.
		}
		else { // alloc success
			wait_required = true;
			// move data to scratch segment then write + notify into reserved distant segment memory
			BufferElt * new_e = loc_on_scratch;
			loc_on_scratch = (BufferElt *)ADD_PTR(loc_on_scratch,e->buffer_entry_size());
			memcpy(new_e, e, e->buffer_entry_size());
			
	        gaspi_notification_id_t notification_id = alloc_res.second;
	        wait_if_queue_full(ctx->queue_result,0);
			SUCCESS_OR_DIE(gaspi_write_notify(
				ctx->segment_scratch, DIFF_PTR(new_e,ctx->ptr_segment_scratch), 
				target_rank, ctx->segment_result, alloc_res.first, (gaspi_size_t)new_e->buffer_entry_size(), 
				notification_id, 1, 
				ctx->queue_result, TITUS_DLB_GASPI_TIMEOUT 
			));
			ctx->get_logger()->signal_results_pushed(e->nb_result);

			//TITUS_DBG << "BufferedSegmentMetadata::try_push_local_transiting_results > "
			//	<< "successfuly reserved memory for elt[" << *new_e << "] to segment " << (int)ctx->segment_result << " on rank " << target_rank << " as elt " << alloc_res.second << " @offset=" << alloc_res.first << std::endl;
			//TITUS_DBG.flush();
        	
			free(e); // results were copied to scratch segment. free the non-segment local memory copy and erase entry
			auto tmp = it; tmp --;
			local_transiting_results.erase(it);
			it = tmp; // because the for loop is going to ++ back again while erase already returned next element.
			//TITUS_DBG << "BufferedSegmentMetadata::try_push_local_transiting_results [" << count << "]" << *new_e << " success (elt " << alloc_res.second << ")" << std::endl; //TITUS_DBG.flush();
		}
		count ++;
	}
	if (wait_required) {
		uint64_t wait_start = rdtsc();
		while (true){
			gaspi_return_t retval = gaspi_wait(ctx->queue_result,TITUS_DLB_GASPI_TIMEOUT);
			if (retval == GASPI_SUCCESS) break;
			if (retval != GASPI_TIMEOUT) SUCCESS_OR_DIE(retval);
			TITUS_DBG << "BufferedSegmentMetadata::try_push_local_transiting_results : WARNING : waiting for queue to fhush for " << (rdtsc() - wait_start) / (TITUS_PROC_FREQ/1000) << "ms" << std::endl;
		}
	//TITUS_DBG << "BufferedSegmentMetadata::try_push_local_transiting_results gaspi_wait returned SUCCESS" << std::endl;
	}

}

void BufferedSegmentMetadata::print_local_transiting_results(std::ostream & out, std::string line_prefix)const{
	out << "print_local_transiting_results : (" << local_transiting_results.size() << " elts)" << std::endl;
	int count = 0;
	for (auto it = local_transiting_results.begin(); it != local_transiting_results.end(); it++){
		BufferElt * e = it->second.second;
		out << line_prefix << "  [" << count << "]->" << it->first << " : " << *e << std::endl;
		count ++;
	}
}



std::ostream & operator<<(std::ostream & out, const BufferedSegmentMetadata & arg) {arg.print(out); return out;}
