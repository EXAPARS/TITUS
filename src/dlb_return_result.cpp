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

#include <GASPI.h>
#include <string.h>

#include <dlb_gaspi_tools.hpp>
#include "dlb_internal.hpp"
#include "dlb_context.hpp"
#include <sstream>


void DLB_impl::return_result() {
    //return_result_simple();
    return_results_buffered();
}

// returns results in the tmp segment into the results segment of the destination selected by DVS as the next in the route to the owner of the tasks that generated those results.
void DLB_impl::return_results_buffered() {
	get_context()->get_logger()->signal_start_return_results();

	//get_context()->get_metadata_result()->try_push_local_transiting_results();
	
	if (!get_context()->get_metadata_result()->is_empty()){
		//TITUS_DBG << "try_flush_buffer_elts_packed (1st pass)" << std::endl ; //TITUS_DBG.flush();
		get_context()->get_metadata_result()->try_flush_buffer_elts_packed();
	}
    
    if (get_context()->get_metadata_tmp()->nb_result != 0){
		get_context()->get_metadata_tmp()->push_tmp_results();
		
		if (!get_context()->get_metadata_result()->is_empty()){
			//TITUS_DBG << "try_flush_buffer_elts_packed (2nd pass)" << std::endl ; //TITUS_DBG.flush();
			get_context()->get_metadata_result()->try_flush_buffer_elts_packed();
		}
	}
	get_context()->get_logger()->signal_end_return_results();
}

// fully connected mode : requires to be able to communicate to any rank.
// returns results in the tmp segment to the owner of the tasks that generated them.
// results are written remotely at their final place in the result segment of their owner.
// adress is calculated with last_result_id and nb_result fields of the MetadataTmp of the current context.
// the remote result segment must be allocated large enough to contain the whole results of current work
// requires same results size and known number of initial tasks per rank
// does not support task spawning.
// does not support results routing through small world neighboring.
void DLB_impl::return_result_simple()
{
    dlb_int t1, t2;
	get_context()->get_logger()->signal_start_return_results();
    //DEBUG_PRINT("=====================DLB::return_result====================\n");
    MetadataTmp *t;
    t = context->get_metadata_tmp();
    gaspi_atomic_value_t value_old;
    
    if(t->nb_result == 0) return;

    if (t->owner_result_rank == context->get_rank()
    )
    {
        memcpy(ADD_PTR(context->ptr_segment_result,(t->last_result_id-t->nb_result)*t->result_size+sizeof(MetadataResult)), ADD_PTR(t->ptr_next_result_to_store, t->result_size), (size_t)t->nb_result*t->result_size);
    }
    else
    {
        SUCCESS_OR_DIE(gaspi_write(context->segment_tmp, context->shared_tmp_result_segment_size-(t->result_size*t->nb_result), t->owner_result_rank, context->segment_result, ((t->last_result_id-t->nb_result)*t->result_size+sizeof(MetadataResult)), (gaspi_size_t)(t->nb_result*t->result_size), context->queue_result, DLB_GASPI_TIMEOUT ));
        SUCCESS_OR_DIE(gaspi_wait(context->queue_result, DLB_GASPI_TIMEOUT)); // How can we optimize out this waiting time with overlaping ? (flip-flop buffer ...)
    }
    SUCCESS_OR_DIE( gaspi_atomic_fetch_add (context->segment_result, 0, t->owner_result_rank, (gaspi_atomic_value_t)t->nb_result, &value_old, DLB_GASPI_TIMEOUT));
    DEBUG_PRINT("gaspi_atomic_fetch_add (seg_src[%llu], off[0], rank[%llu], nb_result[%llu], value_old[%llu]))\n",context->segment_result, t->owner_result_rank, t->nb_result, value_old);

	get_context()->get_logger()->signal_end_return_results();
}

