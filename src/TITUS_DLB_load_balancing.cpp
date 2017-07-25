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
#include <assert.h>

#include <TITUS_DLB_gaspi_tools.hpp>
#include "TITUS_DLB_internal.hpp"
#include "TITUS_DLB_context.hpp"
//#include <TITUS_logger.hpp>


TITUS_DLB_int TITUS_DLB_impl::load_balancing()
{
    DEBUG_PRINT("=====================TITUS_DLB::load_balancing==================\n");
    gaspi_rank_t target_rank;
    MetadataResult * r = context->get_metadata_result();
    
    static bool first_theft = false;
    if (first_theft == false){
        first_theft = true;
        TITUS_DBG << "first theft !" << std::endl;
    }
    //assert that number of result is not greater than expected number of result
    if (r->nb_result > r->nb_usr_results){
		TITUS_DBG << "ASSERT IS GOING TO FAIL : " << r->nb_result << " > " << r->nb_usr_results << std::endl; //TITUS_DBG.flush();
	}
    assert(r->nb_result <= r->nb_usr_results);
    
    // check that no results are added after global_termination_detected detection has been reached
    if (r->nb_result == r->nb_usr_results){
		if (termination_detection() == SUCCESS){
			return FINISHED;
		}
	}
    target_rank = DVS::select_target_rank();
    ASSERT(context->registrations_status[target_rank] == true);
    
    // tauto check
    if(target_rank >= context->get_nb_ranks()) {
        TITUS_DBG << "BAD TARGET RANK " << target_rank << "/" << context->get_nb_ranks() << ")" << std::endl; //TITUS_DBG.flush();
        assert(target_rank < context->get_nb_ranks());
    }
    if (target_rank == context->get_rank()) return(load_balancing()); // if target == self, retry
    
    uint64_t before_theft_attempt = rdtsc();
    if (context->ptr_algorithm_function(target_rank) == SUCCESS) // blocking steal attempt
    {
        static bool first_successful_theft = false;
	if (first_successful_theft == false){
            first_successful_theft = true;
            TITUS_DBG << "first successful theft ! (target = " << target_rank << ")" << std::endl;
        }
        return WORKER;
    }
    uint64_t after_theft_attempt = rdtsc();
    //~ uint64_t sleeptime = 2 * (after_theft_attempt - before_theft_attempt) / (TITUS_PROC_FREQ/1000000);
    //~ if (sleeptime > 10000) TITUS_DBG << "failed theft attempt. sleeping for " << sleeptime << "us" << std::endl;
    //~ usleep(sleeptime); //! TODO : is that a smart attempt to avoid network overload ? probably not ...
    return THIEF;
}

typedef struct kill_me_later_args_s{
	pthread_t killer_thread;
	int sig;
	uint64_t timeout_ms;
}kill_me_later_args_t;

void * kill_me_later(void * _arg){
	kill_me_later_args_t *arg = (kill_me_later_args_t*)_arg;
	//TITUS_DBG << "kill_me_later : sleeping for " << secs << " seconds" << std::endl;
	usleep(arg->timeout_ms * 1000);
	//TITUS_DBG << "kill_me_later : killing process" << std::endl;
	print_stacktrace_and_quit(arg->sig);
	return nullptr;
}

void TITUS_DLB_impl::parallel_work(uint64_t timeout_ms)
{
    DEBUG_PRINT("=====================TITUS_DLB::parallel_work====================\n");
	if (context == nullptr){
		TITUS_DBG << "TITUS_DLB_impl::parallel_work : ASSERT IS GOING TO FAIL : context is not set" << std::endl;
		ASSERT(context != nullptr);
	}
    TITUS_DLB_int state;
    context->parallel_work_session_time.start();
    
    if((context->ptr_dequeue->tail - context->ptr_dequeue->head) > 0)        state = WORKER;
    else                                                                    state = THIEF;

	kill_me_later_args_t kill_me_later_args;
    if (timeout_ms != 0) {
		kill_me_later_args.sig = SIGUSR2;
		kill_me_later_args.timeout_ms = timeout_ms;
		
		pthread_create(&kill_me_later_args.killer_thread, nullptr, kill_me_later, (void*)&kill_me_later_args);
	}
    while(1)
    {
        switch(state)
        {
            case WORKER:
                work_on_my_dequeue();
                state = THIEF;
                break;
            case THIEF:
                //assert when 100000 steal try failed on the same rank
				// check if segment push requested on result segment
				return_result();
                state = load_balancing();
                //display_state();
                break;
            case FINISHED:
				if (timeout_ms != 0) pthread_kill(kill_me_later_args.killer_thread, SIGKILL);
                context->parallel_work_session_time.stop();
                context->logger.end_session();
                context->reset();
                
                // tauto check
                assert(context->get_metadata_result()->local_transiting_results.empty());
                assert(context->get_metadata_task()->is_empty());
                assert(context->get_metadata_tmp()->is_empty());
                assert(context->get_metadata_result()->is_empty());
                
                gaspi_number_t qsize;
				SUCCESS_OR_DIE(gaspi_queue_size(context->segment_task,&qsize)); ASSERT(qsize == 0);
				SUCCESS_OR_DIE(gaspi_queue_size(context->segment_result,&qsize)); ASSERT(qsize == 0);
				SUCCESS_OR_DIE(gaspi_queue_size(context->segment_tmp,&qsize)); ASSERT(qsize == 0);
				std::cout << "context->segment_scratch = " << (uint) context->segment_scratch << std::endl;
				SUCCESS_OR_DIE(gaspi_queue_size(context->segment_scratch,&qsize)); ASSERT(qsize == 0);
                return;
            default:
                //assert state not known
                assert(0);
        }
    }
}

