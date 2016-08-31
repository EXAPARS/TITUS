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
//  FILE    :   dlb_work_on_my_dequeue.c
//  CONTENT :
//
//====================================================================================

#include <GASPI.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <cstdint>
#include <dlb_gaspi_tools.hpp>
#include "dlb_internal.hpp"
#include "dlb_context.hpp"


void DLB_impl::write_task_to_remote_segment()
{
	//! TODO : move to work_requesting.
	//! TODO : this code has not been maintained and may be broken to any degree.
    dlb_int t1, t2;
    t1 = rdtsc(); //! TODO : add signal
    
    DEBUG_PRINT("==============DLB::write_task_to_remote_segment============\n");
    MetadataTask *m;
    m = context->get_metadata_task();
    gaspi_notification_t value = 1;
    gaspi_notification_id_t id = m->state;
    gaspi_rank_t thief_rank = m->state;
    //assert if size of data src is greater than the size of data dest
    assert(context->shared_task_segment_size >= m->segment_size);

    m->compare_and_swap_status(context->get_rank(),context->segment_task, thief_rank, COPY_IN_PROGRESS);
    SUCCESS_OR_DIE( gaspi_write_notify(context->segment_task, 0, thief_rank, context->segment_task, 0, m->segment_size, id, value, context->queue_task, DLB_GASPI_TIMEOUT ));
    
    t2 = rdtsc(); //! TODO : add signal
    context->comm_send_counter += (t2-t1);
}

void DLB_impl::move_task_from_my_dequeue_to_my_segment()
{
    DEBUG_PRINT("========DLB::move_task_from_my_dequeue_to_my_segment=======\n");
    context->logger.signal_start_put_on_segment();
    gaspi_atomic_value_t old_value;
    MetadataTask *m;
    m = context->get_metadata_task();
    
    dlb_int nb_task, nb_task_max;
    nb_task     = context->ptr_dequeue->tail - context->ptr_dequeue->head;
    nb_task_max = ((context->shared_task_segment_size - sizeof(MetadataTask)) / context->ptr_dequeue->task_size);

    if (nb_task < NB_TASK_MIN)               // not enough task on dequeue
    {
        m->segment_size         = sizeof(MetadataTask);
        m->owner_task_rank      = context->get_rank();
        m->nb_task              = 0;
        m->end_task_id          = 0;
        m->task_size            = 0;
        m->result_size          = 0;
        m->ptr_task_function    = nullptr;
    }
    else if (nb_task >=  (2*nb_task_max))    //move nb_task_max on task segment from dequeue
    {
        m->segment_size         = sizeof(MetadataTask) + context->ptr_dequeue->task_size * nb_task_max;
        m->owner_task_rank      = context->ptr_dequeue->owner_task_rank;
        m->nb_task              = nb_task_max;
        m->end_task_id          = context->ptr_dequeue->end_task_id - (nb_task - nb_task_max);
        m->task_size            = context->ptr_dequeue->task_size;
        m->result_size          = context->ptr_dequeue->result_size;
        m->ptr_task_function    = context->ptr_dequeue->ptr_task_function;
        memcpy(ADD_PTR(context->ptr_segment_task,sizeof(MetadataTask)), ADD_PTR(context->ptr_dequeue->base_task, context->ptr_dequeue->head * context->ptr_dequeue->task_size), (size_t)m->nb_task*m->task_size);
        
        context->ptr_dequeue->head       = context->ptr_dequeue->head + nb_task_max;
    }
    else                                     // move nb_task/2 on task segment from dequeue
    {
        dlb_int nb_task_set_on_segment, nb_task_remainder;
        
        nb_task_set_on_segment  = nb_task/2;
        nb_task_remainder       = nb_task - nb_task_set_on_segment;

        m->segment_size                 = sizeof(MetadataTask) + context->ptr_dequeue->task_size * nb_task_set_on_segment;
        m->owner_task_rank      = context->ptr_dequeue->owner_task_rank;
        m->nb_task              = nb_task_set_on_segment;
        m->end_task_id         = context->ptr_dequeue->end_task_id - nb_task_remainder;
        m->task_size            = context->ptr_dequeue->task_size;
        m->result_size          = context->ptr_dequeue->result_size;
        m->ptr_task_function    = context->ptr_dequeue->ptr_task_function;

        memcpy(ADD_PTR(context->ptr_segment_task,sizeof(MetadataTask)), ADD_PTR(context->ptr_dequeue->base_task, context->ptr_dequeue->head * context->ptr_dequeue->task_size), (size_t)m->nb_task*m->task_size);
        context->ptr_dequeue->head       = context->ptr_dequeue->head + nb_task_set_on_segment;
    }
    
    if(m->nb_task == 0)     old_value = m->compare_and_swap_status(context->get_rank(),context->segment_task, FREE_FOR_COPY, NO_TASK);
    else                    old_value = m->compare_and_swap_status(context->get_rank(),context->segment_task, FREE_FOR_COPY, TASK_AVAILABLE);

    assert(old_value == FREE_FOR_COPY);

    context->logger.signal_end_put_on_segment(m->nb_task);
    return;
}
// so called "self-theft"
void DLB_impl::move_task_from_my_segment_to_my_dequeue()
{
    DEBUG_PRINT("========DLB::move_task_from_my_segment_to_my_dequeue=======\n");
    context->logger.signal_start_get_from_segment();
    MetadataTask *m;
    m = context->get_metadata_task();
    
    dlb_int nb_task, nb_task_steal;
    nb_task             = m->nb_task;
    nb_task_steal       = 0;
    
    if (nb_task < NB_TASK_MIN)          //move all tasks to dequeue
    {
        context->ptr_dequeue->head               = 0;
        context->ptr_dequeue->tail               = m->nb_task;
        context->ptr_dequeue->owner_task_rank    = m->owner_task_rank;
        context->ptr_dequeue->task_size          = m->task_size;
        context->ptr_dequeue->end_task_id       = m->end_task_id;
        context->ptr_dequeue->result_size        = m->result_size;
        context->ptr_dequeue->ptr_task_function  = m->ptr_task_function;
        
        m->nb_task          = 0;
        m->segment_size             = sizeof(MetadataTask);
        m->end_task_id     = 0;
        
        memcpy(context->ptr_dequeue->base_task, ADD_PTR(context->ptr_segment_task,m->segment_size), (size_t)context->ptr_dequeue->tail*context->ptr_dequeue->task_size);
    }
    else                                // move half the tasks to dequeue
    {
        nb_task_steal                               = nb_task/2;

        context->ptr_dequeue->head                    = 0;
        context->ptr_dequeue->tail                   = nb_task_steal;
        context->ptr_dequeue->owner_task_rank         = m->owner_task_rank;
        context->ptr_dequeue->task_size              = m->task_size;
        context->ptr_dequeue->end_task_id           = m->end_task_id;
        context->ptr_dequeue->result_size            = m->result_size;
        context->ptr_dequeue->ptr_task_function      = m->ptr_task_function;
        
        m->nb_task          = nb_task - nb_task_steal;
        m->segment_size             = sizeof(MetadataTask) + m->task_size * m->nb_task;
        m->end_task_id     -= nb_task_steal;
        
        memcpy(context->ptr_dequeue->base_task, ADD_PTR(context->ptr_segment_task,m->segment_size), (size_t)context->ptr_dequeue->tail*context->ptr_dequeue->task_size);
    }
    if(m->nb_task == 0)     m->compare_and_swap_status(context->get_rank(), context->segment_task, m->state, NO_TASK);
    else                    m->compare_and_swap_status(context->get_rank(), context->segment_task, m->state, TASK_AVAILABLE);
    context->logger.signal_end_get_from_segment(m->nb_task);
    return;
}
    
void DLB_impl::do_next_task()
{

    context->logger.signal_start_task();
    void *task_data,*results_ptr,*generic_task_params;
    MetadataTmp *t;
    t = context->get_metadata_tmp();
    
    task_data = ADD_PTR(context->ptr_dequeue->base_task,(context->ptr_dequeue->tail-1)*context->ptr_dequeue->task_size);      //get pointer on task to do
    results_ptr = t->ptr_next_result_to_store;                                                      						  //get pointer on place to write result
    generic_task_params = context->ptr_dequeue->ptr_task_params;
    context->ptr_dequeue->ptr_task_function(task_data, results_ptr, generic_task_params);

    t->ptr_next_result_to_store = ADD_PTR(t->ptr_next_result_to_store,-(t->result_size));  //set pointer on next result
    ++t->nb_result;
    --context->ptr_dequeue->end_task_id;
    --context->ptr_dequeue->tail;

    context->logger.signal_end_task();
    return;
}

void DLB_impl::detection_request()
{
    DEBUG_PRINT("======================DLB::detection_request=====================\n");
    MetadataTask *m;
    m = context->get_metadata_task();
    
    if(m->state == context->get_rank())                            move_task_from_my_segment_to_my_dequeue();
    else if(context->algorithm == DLB::WORK_REQUESTING)        write_task_to_remote_segment();
    else if(context->algorithm == DLB::WORK_STEALING)        {    }
}

void DLB_impl::active_wait_state()
{
    DEBUG_PRINT("======================DLB::active_wait_state==============\n");
    gaspi_atomic_value_t old_value;
    int i;
    MetadataTask *m;
    m = context->get_metadata_task();

    if(m->state != NO_TASK)
    {
        gaspi_printf("WARNING : active wait on rank %u\n", context->get_rank());
#ifdef DEBUG
        display_state();
#endif
        i = 0;
        while(1)
        {
            ++i;
            //assert AFTER 1 s
            if (!(i < 100000)){
				TITUS_DBG << "ASSERT IS GOING TO FAIL : " << __FILE__ << ":" << __LINE__ << std::endl;
				TITUS_DBG.flush();
				assert(i < 100000);
			}
            usleep(20);
            if(m->state == FREE_FOR_COPY)
            {
                old_value = m->compare_and_swap_status(context->get_rank(),context->segment_task, FREE_FOR_COPY, NO_TASK);
                ASSERT(old_value == FREE_FOR_COPY)
                break;
            }
        }
    }
}

void DLB_impl::work_on_my_dequeue()
{
    DEBUG_PRINT("======================DLB::work_on_my_dequeue==============\n");
    context->logger.signal_start_work();
    gaspi_atomic_value_t old_value;
    MetadataTask *m;
    MetadataTmp *t;
    
    m = context->get_metadata_task();
    t = context->get_metadata_tmp();
    
    //start with NO_TASK state 
    //gaspi_printf("%llu\n",m->state);
    assert(m->state == NO_TASK);
    
    //The rank can put task on shared segment
    if((context->ptr_dequeue->tail - context->ptr_dequeue->head) > NB_TASK_MIN){
		old_value = m->compare_and_swap_status(context->get_rank(),context->segment_task, NO_TASK, FREE_FOR_COPY);
		ASSERT(old_value == NO_TASK);
	}
    
    //Initialization dequeue after successful work request
    if(m->nb_task > 0)  move_task_from_my_segment_to_my_dequeue();

    //Initialization MetadataResult
    t->owner_result_rank        = context->ptr_dequeue->owner_task_rank;
    t->last_result_id           = context->ptr_dequeue->end_task_id;
    t->nb_result                = 0;
    t->result_size              = context->ptr_dequeue->result_size;
    t->ptr_next_result_to_store = ADD_PTR(context->ptr_segment_tmp, context->shared_tmp_result_segment_size-t->result_size);

    while(context->ptr_dequeue->tail != context->ptr_dequeue->head)
    {
		// PRELIMINARY CHECKS & SETUP
		
        //assert that segment tmp is not overflown
        assert(t->nb_result <= t->nb_result_max);
        //copy result if tmp segment is full
        if(t->nb_result == t->nb_result_max)
        {
            return_result();
			t->owner_result_rank        = context->ptr_dequeue->owner_task_rank;
			t->last_result_id           = context->ptr_dequeue->end_task_id;
			t->nb_result                = 0;
			t->result_size              = context->ptr_dequeue->result_size;
			t->ptr_next_result_to_store = ADD_PTR(context->ptr_segment_tmp, context->shared_tmp_result_segment_size-t->result_size);
        }
		// check if segment push requested on result segment
		
		if (context->get_metadata_result()->occupancy() >= PUSH_REQUEST_MIN_OCCUPANCY){
			//TITUS_DBG << "WARNING : DLB_impl::work_on_my_dequeue : detected SEGMENT_PUSH_REQUESTED while working. Calling push_results_buffer" << std::endl; //TITUS_DBG.flush();
			context->logger.signal_end_work();
			context->push_results_buffer();
		    context->logger.signal_start_work();

		}
		
        do_next_task();
		
        //request on local shared segment
        if((context->ptr_dequeue->tail - context->ptr_dequeue->head) == 0)
			old_value = m->compare_and_swap_status(context->get_rank(),context->segment_task, TASK_AVAILABLE, (gaspi_atomic_value_t)context->get_rank());

        switch(m->state)
        {
            case TASK_AVAILABLE:                                                    break;
            case NO_TASK:                                                           break;
            case COPY_IN_PROGRESS:                                                  break;
            case FREE_FOR_COPY:     move_task_from_my_dequeue_to_my_segment();  	break;
            default :               detection_request();// work request detection & handling
        }
    }
    //assert task segment state is not TASK_AVAILABLE
    assert(!(m->state == TASK_AVAILABLE));

    //return result
    return_result();
    active_wait_state();
    
    context->logger.signal_end_work();
}
