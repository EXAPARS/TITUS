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
//  FILE    :   dlb_work_requesting.cpp
//  CONTENT :
//
//====================================================================================


//! TODO : this code has not been maintained and may be broken to any degree.

#include <GASPI.h>
#include <string.h>
#include <assert.h>
#include <cstdint>

#include <dlb_gaspi_tools.hpp>
#include "dlb_internal.hpp"
#include "dlb_context.hpp"

dlb_int DLB_impl::work_requesting(gaspi_rank_t target_rank)
{
    DEBUG_PRINT("=====================DLB::work_requesting=====================\n");
    context->logger.signal_start_theft(target_rank);

    gaspi_atomic_value_t old_value;
    MetadataTask *m;
    bool success = 0;
    
    m = context->get_metadata_task();
    
    SUCCESS_OR_DIE(gaspi_atomic_compare_swap(context->segment_task, 0, target_rank, (gaspi_atomic_value_t)TASK_AVAILABLE, (gaspi_atomic_value_t)context->get_rank(), &old_value, DLB_GASPI_TIMEOUT));
    //DEBUG_PRINT("old_value = %llu target_rank=%llu\n",old_value,target_rank);
    
    dlb_int victim_state = old_value;

    if(victim_state == TASK_AVAILABLE)
    {
        success = 1;
        gaspi_notification_t        value = 1;
        gaspi_notification_id_t id, first_id;  // 2 bytes
        id = context->get_rank();
        
        SUCCESS_OR_DIE(gaspi_notify_waitsome(context->segment_task, id, 1, &first_id, DLB_GASPI_TIMEOUT));
        //DEBUG_PRINT("\t\tgaspi_notify_waitsome id[%u] first_id[%u], m->nb_task[%llu]\n",id,first_id,m->nb_task);
        
        SUCCESS_OR_DIE(gaspi_notify_reset (context->segment_task, first_id, &value) );
        SUCCESS_OR_DIE(gaspi_atomic_compare_swap(context->segment_task, 0, target_rank, COPY_IN_PROGRESS, FREE_FOR_COPY, &old_value, DLB_GASPI_TIMEOUT));
        
        //assert when gaspi_atomic_compare_swap failed
        assert(old_value == COPY_IN_PROGRESS);

        //We can't do m->state = NO_TASK thus we use compare swap function
        SUCCESS_OR_DIE(gaspi_atomic_compare_swap(context->segment_task, 0, context->get_rank(), COPY_IN_PROGRESS, NO_TASK, &old_value, DLB_GASPI_TIMEOUT));

        //assert when gaspi_atomic_compare_swap failed
        assert(old_value == COPY_IN_PROGRESS);

    }
    
    dlb_int first_id = 0;
    if (success) first_id = m->end_task_id - m->nb_task +1;
    context->logger.signal_end_theft(victim_state, m->nb_task, first_id, target_rank, m->owner_task_rank);
    
    if(success)    return SUCCESS;
    else         return FAILED;
}

