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


//TODO is still up to date in current implementation?

#include <GASPI.h>
#include <string.h>
#include <assert.h>
#include <cstdint>

#include <TITUS_DLB_gaspi_tools.hpp>
#include "TITUS_DLB_internal.hpp"
#include "TITUS_DLB_context.hpp"

TITUS_DLB_int TITUS_DLB_impl::work_requesting(gaspi_rank_t target_rank)
{
    DEBUG_PRINT("=====================TITUS_DLB::work_requesting=====================\n");
    context->time_spent_stealing.start();

    gaspi_atomic_value_t old_value;
    MetadataTask *m;
    bool success = 0;
    
    m = context->get_metadata_task();
    
    SUCCESS_OR_DIE(gaspi_atomic_compare_swap(context->segment_task, 0, target_rank, (gaspi_atomic_value_t)TASK_AVAILABLE, (gaspi_atomic_value_t)context->get_rank(), &old_value, TITUS_DLB_GASPI_TIMEOUT));
    //DEBUG_PRINT("old_value = %llu target_rank=%llu\n",old_value,target_rank);
    
    TITUS_DLB_int victim_state = old_value;

    if(victim_state == TASK_AVAILABLE)
    {
        success = 1;
        gaspi_notification_t        value = 1;
        gaspi_notification_id_t id, first_id;  // 2 bytes
        id = context->get_rank();
        
        SUCCESS_OR_DIE(gaspi_notify_waitsome(context->segment_task, id, 1, &first_id, TITUS_DLB_GASPI_TIMEOUT));
        //DEBUG_PRINT("\t\tgaspi_notify_waitsome id[%u] first_id[%u], m->nb_task[%llu]\n",id,first_id,m->nb_task);
        
        SUCCESS_OR_DIE(gaspi_notify_reset (context->segment_task, first_id, &value) );
        SUCCESS_OR_DIE(gaspi_atomic_compare_swap(context->segment_task, 0, target_rank, COPY_IN_PROGRESS, FREE_FOR_COPY, &old_value, TITUS_DLB_GASPI_TIMEOUT));
        
        //assert when gaspi_atomic_compare_swap failed
        assert(old_value == COPY_IN_PROGRESS);

        //We can't do m->state = NO_TASK thus we use compare swap function
        SUCCESS_OR_DIE(gaspi_atomic_compare_swap(context->segment_task, 0, context->get_rank(), COPY_IN_PROGRESS, NO_TASK, &old_value, TITUS_DLB_GASPI_TIMEOUT));

        //assert when gaspi_atomic_compare_swap failed
        assert(old_value == COPY_IN_PROGRESS);

    }
    
    TITUS_DLB_int first_id = 0;
    if (success) first_id = m->end_task_id - m->nb_task +1;
    context->time_spent_stealing.stop(victim_state == TASK_AVAILABLE);
    context->stolen_tasks_count += m->nb_task;
    if (victim_state == FREE_FOR_COPY) context->miss_free_for_copy++;
    else if (victim_state == NO_TASK) context->miss_notask_count++;
    else context->miss_target_locked_count++;
    
    if(success)    return SUCCESS;
    else         return FAILED;
}

