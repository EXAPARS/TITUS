/* 
* This file is part of the TITUS software.
* https://github.com/exapars/TITUS
* Copyright (c) 2015-2017 University of Versailles UVSQ
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


#ifndef __c_TITUS_DLB__H__
#define __c_TITUS_DLB__H__

extern "C"
{
    // TITUS_DLB :
    gaspi_return_t c_TITUS_DLB_gaspi_proc_init(gaspi_timeout_t timeout);// utility wrapper to gaspi_proc_init()
    void c_TITUS_DLB_parallel_work();// cannot be called before c_TITUS_DLB_new_context()

    // c_TITUS_DLB_CONTEXT : 
    void c_TITUS_DLB_set_context(void* arg);// cannot be called before c_TITUS_DLB_new_context()
    void c_TITUS_DLB_get_context(void **ret_val);// cannot be called before c_TITUS_DLB_set_context()
    void c_TITUS_DLB_new_context(void **ret_val, int shared_task_segment_size, int algorithm);// cannot be called before gaspi_proc_init()
    void c_TITUS_DLB_delete_context(void *context );
    void c_TITUS_DLB_set_problem(void *context, void *problem, int task_size, int nb_task, void *result, int result_size, void *ptr_task_function, void *params);// cannot be called before c_TITUS_DLB_set_context()
    void c_TITUS_DLB_get_logger(void **ret_val, void *context);// cannot be called before gaspi_proc_init()
	
	// c_TITUS_DLB_LOGGER :
    void c_TITUS_DLB_print_current_session(void *logger, char *filename);// cannot be called before c_TITUS_DLB_set_problem()
    void c_TITUS_DLB_print_agregated_session_info(void *logger, char *filename);// cannot be called before c_TITUS_DLB_set_problem()
    void c_TITUS_DLB_print_all_sessions(void *logger, char *filename);// cannot be called before c_TITUS_DLB_set_problem()

    void c_TITUS_DLB_dump_buffer(void *logger);// cannot be called before c_TITUS_DLB_set_problem()
    void c_TITUS_DLB_print_buffer(void *logger, char *filename);// cannot be called before c_TITUS_DLB_set_problem()
    void c_TITUS_DLB_set_autodump(void *logger, int *val, char *logdir);// cannot be called before c_TITUS_DLB_set_context()

	// DVS
	
    void c_TITUS_DLB_get_DVS_context(void **ret_val, void *context);// cannot be called before c_TITUS_DLB_set_context()
    void c_TITUS_DLB_set_DVS_context(void *context, void *arg);// cannot be called before c_TITUS_DLB_set_context()
}

#endif
