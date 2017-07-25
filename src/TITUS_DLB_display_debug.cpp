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
#include "TITUS_DLB_context.hpp"

#ifdef DEBUG

void TITUS_DLB_Context_impl::display_state()
{
    MetadataTask *m;
    m = get_metadata_task();
    switch(m->state)
    {
        case TASK_AVAILABLE:    DEBUG_PRINT("TASK_AVAILABLE\n");                   break;
        case NO_TASK:           DEBUG_PRINT("NO_TASK\n");                          break;
        case FREE_FOR_COPY:     DEBUG_PRINT("FREE_FOR_COPY\n");                    break;
        default:                DEBUG_PRINT("RANK_DETECTED(%llu)\n",m->state);
    }
    return;
}

void TITUS_DLB_Context_impl::display_metadatatask()
{
    MetadataTask *m;
    m = get_metadata_task();
    
    DEBUG_PRINT("=======================DISPLAY MetadataTask=======================\n");
    display_state();
    DEBUG_PRINT("size = %llu\n",            m->segment_size);
    DEBUG_PRINT("owner_task_rank = %llu\n", m->owner_task_rank);
    DEBUG_PRINT("nb_task = %llu\n",         m->nb_task);
    DEBUG_PRINT("first_task_id = %llu\n",    m->first_task_id);
    DEBUG_PRINT("task_size = %llu\n",       m->task_size);
    DEBUG_PRINT("result_size = %llu\n",     m->result_size);
    DEBUG_PRINT("ptr_task_function = %x\n", m->ptr_task_function);
    DEBUG_PRINT("=========================END MetadataTask=========================\n");
}


void TITUS_DLB_Context_impl::display_metadataresult()
{
    MetadataResult *r;
    r = get_metadata_result();
    
    DEBUG_PRINT("=======================DISPLAY MetadataResult=====================\n");
    DEBUG_PRINT("nb_result = %llu\n",           r->nb_result);
    DEBUG_PRINT("termination_status.left_child_subtree_termination = %llu\n",   r->termination_status.left_child_subtree_termination);
    DEBUG_PRINT("termination_status.right_child_subtree_termination = %llu\n",  r->termination_status.right_child_subtree_termination);
    DEBUG_PRINT("termination_status.global_termination_detected = %llu\n",         r->termination_status.global_termination_detected);
    DEBUG_PRINT("nb_result_initial = %llu\n",   r->nb_result_initial);
    DEBUG_PRINT("result_size = %llu\n",         r->result_size);
    DEBUG_PRINT("base_result = %x\n",           r->base_result);
    DEBUG_PRINT("=========================END MetadataResult=======================\n");
}


void TITUS_DLB_Context_impl::display_metadatatmp()
{
    MetadataTmp *t;
    t = get_metadata_tmp();
    
    DEBUG_PRINT("=======================DISPLAY MetadataTmp========================\n");
    DEBUG_PRINT("owner_result_rank = %llu\n",       t->owner_result_rank);
    DEBUG_PRINT("last_result_id = %llu\n",          t->last_result_id);
    DEBUG_PRINT("nb_result = %llu\n",               t->nb_result);
    DEBUG_PRINT("result_size = %llu\n",             t->result_size);
    DEBUG_PRINT("nb_result_max = %llu\n",           t->nb_result_max);
    DEBUG_PRINT("ptr_next_result_to_store = %x\n",  t->ptr_next_result_to_store);
    DEBUG_PRINT("=========================END MetadataTmp==========================\n");
}


void TITUS_DLB_Context_impl::display_dequeue()
{
    DEBUG_PRINT("========================DISPLAY Dequeue===========================\n");
    DEBUG_PRINT("owner_result_rank = %llu\n",       ptr_dequeue->owner_task_rank);
    DEBUG_PRINT("head = %llu\n",                    ptr_dequeue->head);
    DEBUG_PRINT("tail = %llu\n",                    ptr_dequeue->tail);
    DEBUG_PRINT("base_task = %x\n",                 ptr_dequeue->base_task);
    DEBUG_PRINT("first_task_id = %llu\n",            ptr_dequeue->first_task_id);
    DEBUG_PRINT("task_size = %llu\n",               ptr_dequeue->task_size);
    DEBUG_PRINT("result_size = %llu\n",             ptr_dequeue->result_size);
    DEBUG_PRINT("ptr_task_function = %x\n",         ptr_dequeue->ptr_task_function);
    DEBUG_PRINT("===========================END Dequeue============================\n");
}


void TITUS_DLB_Context_impl::display_Context()
{
    DEBUG_PRINT("========================DISPLAY Dequeue===========================\n");
    DEBUG_PRINT("addr = %x\n",                              this);
    DEBUG_PRINT("rank = %llu\n",                            rank);
    DEBUG_PRINT("nb_procs = %llu\n",                        nb_procs);
    DEBUG_PRINT("shared_task_segment_size = %llu\n",        shared_task_segment_size);
    DEBUG_PRINT("shared_result_segment_size = %llu\n",      shared_result_segment_size);
    DEBUG_PRINT("shared_tmp_result_segment_size = %llu\n",  shared_tmp_result_segment_size);
    DEBUG_PRINT("segment_task = %llu\n",                    segment_task);
    DEBUG_PRINT("segment_result = %llu\n",                  segment_result);
    DEBUG_PRINT("segment_tmp = %llu\n",                     segment_tmp);
    DEBUG_PRINT("queue_task = %llu\n",                      queue_task);
    DEBUG_PRINT("queue_result = %llu\n",                    queue_result);
    DEBUG_PRINT("ptr_segment_task = %x\n",                  ptr_segment_task);
    DEBUG_PRINT("ptr_segment_result = %x\n",                ptr_segment_result);
    DEBUG_PRINT("ptr_segment_tmp = %x\n",                   ptr_segment_tmp);
    DEBUG_PRINT("===========================END Dequeue============================\n");
}
#endif
