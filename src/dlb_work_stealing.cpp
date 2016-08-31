/* 
* This file is part of the TITUS software.
* https://github.com/exapars/
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


//====================================================================================
//
//  AUTHOR  :   UEMURA Seijilo, FONTNAILLE Clément, PETIT Eric 
//  FILE    :   dlb_work_stealing.c
//  CONTENT :
//
//====================================================================================

#include <GASPI.h>
#include <string.h>
#include <assert.h>
#include <cstdint>

#include <dlb_gaspi_tools.hpp>
#include "dlb_internal.hpp"
#include "dlb_context.hpp"

#define SIZE_FIRST_READ 32768

dlb_int DLB_impl::work_stealing(gaspi_rank_t target_rank)
{
    DEBUG_PRINT("=====================DLB::work_stealing=====================\n");
    context->logger.signal_start_theft(target_rank);
    
    MetadataTask * m = context->get_metadata_task();
    
    gaspi_atomic_value_t old_value;
    bool success = false;
    
    //fprintf(stderr, "rank %lu : DLB_impl::work_stealing : locking distant segment for theft\n", context->get_rank());
    //fprintf(stderr, "  > gaspi_atomic_compare_swap(%u, %x, %d, %lu, %lu, &old_value, DLB_GASPI_TIMEOUT)\n"
	//				, context->segment_task, offsetof(std::remove_reference(decltype(*m)), m->state), target_rank, (gaspi_atomic_value_t)TASK_AVAILABLE, context->get_rank());
    old_value = context->get_metadata_task()->compare_and_swap_status(target_rank,context->segment_task, (gaspi_atomic_value_t)TASK_AVAILABLE, context->get_rank());
    DEBUG_PRINT("old_value = %llu target_rank=%llu\n",old_value,target_rank);
    
    dlb_int victim_state = old_value;
    
    if(victim_state == TASK_AVAILABLE)
    {
        success = 1;
        if(context->shared_task_segment_size < SIZE_FIRST_READ)
        {
            SUCCESS_OR_DIE(gaspi_read( context->segment_task, 0, target_rank, context->segment_task, 0, context->shared_task_segment_size, context->queue_task, DLB_GASPI_TIMEOUT ));
            SUCCESS_OR_DIE(gaspi_wait( context->queue_task, DLB_GASPI_TIMEOUT));
        }
        else
        {
            SUCCESS_OR_DIE(gaspi_read( context->segment_task, 0, target_rank, context->segment_task, 0, SIZE_FIRST_READ, context->queue_task, DLB_GASPI_TIMEOUT ));
            SUCCESS_OR_DIE(gaspi_wait( context->queue_task, DLB_GASPI_TIMEOUT));
            if(m->segment_size > SIZE_FIRST_READ)
            {
                SUCCESS_OR_DIE(gaspi_read( context->segment_task, SIZE_FIRST_READ, target_rank, context->segment_task, SIZE_FIRST_READ, (m->segment_size-SIZE_FIRST_READ), context->queue_task, DLB_GASPI_TIMEOUT ));
                SUCCESS_OR_DIE(gaspi_wait( context->queue_task, DLB_GASPI_TIMEOUT));
            }
        }
        
        // release distant segment lock
        gaspi_atomic_value_t unlock_distant_old_value = 
			m->compare_and_swap_status(target_rank,context->segment_task, (gaspi_atomic_value_t)context->get_rank(), FREE_FOR_COPY);

        // comp&swap on my own segment segment : set it to NO_TASK for a moment
        gaspi_atomic_value_t unlock_local_old_value = 
			m->compare_and_swap_status(context->get_rank(),context->segment_task, m->state, NO_TASK);

        //assert that locks have been released
        if(unlock_distant_old_value != context->get_rank()) {
			TITUS_DBG << "ASSERT IS GOING TO FAIL : " << unlock_distant_old_value << " != " << context->get_rank(); //TITUS_DBG.flush();
			ASSERT(unlock_distant_old_value == context->get_rank());
		}
		
        ASSERT(unlock_local_old_value == context->get_rank());
        
        //TITUS_DBG << "DLB_impl::work_stealing > " << m->nb_task << " task stolen from rank " << target_rank << " end_task_id=" << m->end_task_id << ", owner = " << m->owner_task_rank << std::endl; //TITUS_DBG.flush();
    }
    
    dlb_int first_id = 0;
    if (success) first_id = m->end_task_id - m->nb_task +1;
    context->logger.signal_end_theft(victim_state, m->nb_task, first_id, target_rank, m->owner_task_rank);
    
    if(success)    return SUCCESS;
    else         return FAILED;
}
