/* 
* This file is part of the TITUS software.
* https://github.com/exapars/TITUS
* Copyright (c) 2015-2017 University of Versailles UVSQ - Exascale Computing Research
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


#include <GASPI.h>
#include <string.h>

#include <TITUS_DLB_gaspi_tools.hpp>
#include "TITUS_DLB_internal.hpp"
#include "TITUS_DLB_context.hpp"
#include <sstream>


void TITUS_DLB_impl::return_result() {
    //return_result_simple();
    return_results_buffered();
}

// returns results in the tmp segment into the results segment of the destination selected by DVS as the next in the route to the owner of the tasks that generated those results.
void TITUS_DLB_impl::return_results_buffered() {
	get_context()->time_spent_returning_results.start();
	//~ TITUS_DBG << "in TITUS_DLB_impl::return_results_buffered" << std::endl;

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
	get_context()->time_spent_returning_results.stop();
}


// returns results in the tmp segment to the owner of the tasks that generated them.
// results are written remotely at their final place in the result segment of their owner.
// adress is calculated with last_result_id and nb_result fields of the MetadataTmp of the current context.
// the remote result segment must be allocated large enough to contain the whole results of current work
// requires same results size and known number of initial tasks per rank
// does not support task spawning.
// does not support results routing through small world neighboring.
void TITUS_DLB_impl::return_result_simple()
{
	get_context()->time_spent_returning_results.start();

    TITUS_DLB_int t1, t2;
    //DEBUG_PRINT("=====================TITUS_DLB::return_result====================\n");
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
        SUCCESS_OR_DIE(gaspi_write(context->segment_tmp, context->shared_tmp_result_segment_size-(t->result_size*t->nb_result), t->owner_result_rank, context->segment_result, ((t->last_result_id-t->nb_result)*t->result_size+sizeof(MetadataResult)), (gaspi_size_t)(t->nb_result*t->result_size), context->queue_result, TITUS_DLB_GASPI_TIMEOUT ));
        //DEBUG_PRINT("gaspi_write (seg_src[%llu], off[%llu], rank[%llu], seg_dest[%llu], off[%llu], size[%llu]...))\n",context->segment_tmp, context->shared_tmp_result_segment_size-(t->result_size*t->nb_result), t->owner_result_rank, context->segment_result, ((t->last_result_id-t->nb_result)*t->result_size+sizeof(MetadataResult)),(TITUS_DLB_int)t->nb_result*t->result_size);
        SUCCESS_OR_DIE(gaspi_wait(context->queue_result, TITUS_DLB_GASPI_TIMEOUT)); //We can cover this wait by compute with a doubled tmp buffer
        //DEBUG_PRINT("t->result_size=%d t->nb_result=%d MetadataTmp %d MetadataResult %d\n",t->result_size ,t->nb_result, sizeof(MetadataTmp), sizeof(MetadataResult));
    }
    // 
    SUCCESS_OR_DIE( gaspi_atomic_fetch_add (context->segment_result, 0, t->owner_result_rank, (gaspi_atomic_value_t)t->nb_result, &value_old, TITUS_DLB_GASPI_TIMEOUT));
    DEBUG_PRINT("gaspi_atomic_fetch_add (seg_src[%llu], off[0], rank[%llu], nb_result[%llu], value_old[%llu]))\n",context->segment_result, t->owner_result_rank, t->nb_result, value_old);

	get_context()->time_spent_returning_results.stop();
}

