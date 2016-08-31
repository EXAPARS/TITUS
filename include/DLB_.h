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
//  FILE    :   DLB_.h
//  CONTENT :
//
//====================================================================================


#ifndef __DLB__H__
#define __DLB__H__

extern "C"
{
    // DLB :
    void gaspi_return_t dlb_gaspi_proc_init_(gaspi_timeout_t timeout);// utility wrapper to gaspi_proc_init()
    void dlb_set_nb_queues_(int * arg);// has no effect if called after dlb_gaspi_proc_init_()
    void dlb_parallel_work_();// cannot be called before dlb_new_context_()

    // DLB_CONTEXT : 
    void dlb_set_context_(void* arg);// cannot be called before dlb_new_context_()
    void dlb_get_context_(void **ret_val);// cannot be called before dlb_set_context_()
    void dlb_new_context_(void **ret_val, int *shared_task_segment_size, int *algorithm);// cannot be called before gaspi_proc_init()
    void dlb_set_problem_(void *context, void *problem, int *task_size, int *nb_task, void *result, int *result_size, void *ptr_task_function, void *params);// cannot be called before dlb_set_context_()
    void dlb_get_logger_(void **ret_val, void *context);// cannot be called before gaspi_proc_init()
	
	// DLB_LOGGER :
    void dlb_print_current_session_(void *logger, char *filename, int *verbose);// cannot be called before dlb_set_problem_()
    void dlb_print_agregated_session_info_(void *logger, char *filename, int *verbose);// cannot be called before dlb_set_problem_()
    void dlb_print_all_sessions_(void *logger, char *filename, int *verbose);// cannot be called before dlb_set_problem_()

    void dlb_dump_buffer_(void *logger);// cannot be called before dlb_set_problem_()
    void dlb_print_buffer_(void *logger, char *filename);// cannot be called before dlb_set_problem_()
    void dlb_set_autodump_(void *logger, int *val, char *logdir);// cannot be called before dlb_set_context_()

	// DVS
	
    void dlb_get_DVS_context_(void **ret_val, void *context);// cannot be called before dlb_set_context_()
    void dlb_set_DVS_context_(void *context, void *arg);// cannot be called before dlb_set_context_()
}

#endif
