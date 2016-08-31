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

#ifndef __DLB_HPP__
#define __DLB_HPP__

#if __cplusplus < 201103L
#define nullptr 0
#endif

#define DLB_DEFAULT_ALGORITHM DLB::WORK_STEALING

#include <iostream>

typedef unsigned long int dlb_int;

class DLB_Context;
class DLB_Logger;

#include <GASPI.h>
#include "dlb_gaspi_tools.hpp"
#include <DVS.hpp>

// proper library use

class DLB_Context_impl;
class DLB_Logger_impl;

class DLB_Logger{
    DLB_Logger_impl * m_impl;
public :
    DLB_Logger(DLB_Logger_impl * arg):m_impl(arg) {} //implicit conversion
    DLB_Logger(const DLB_Logger & arg); // TODO : (NYI) clone logger

    void print_current_session(std::ostream & out = std::cout)const;
    void print_agregated_session_info(std::ostream & out = std::cout)const;
    void print_all_sessions(std::ostream & out = std::cout)const;

    
    void dump_buffer();
    void print_buffer(std::ostream & out)const;
    void set_autodump(bool val, std::ostream & out = TITUS_DBG); // cannot be called before gaspi_proc_init()
    void set_autodump(bool val, const char *logdir);

    friend void print_stacktrace_and_quit(int);
};

class DLB{
public :
	enum DLB_Algorithm {WORK_STEALING, WORK_REQUESTING};
	
	static gaspi_return_t gaspi_proc_init(gaspi_timeout_t timeout = GASPI_BLOCK);
	//! TODO : investigate and fix : DLB_Barrier is broken ! (only a problem using GASPI_TOPOLOGY_NONE)
	static gaspi_return_t gaspi_barrier(gaspi_group_t group, gaspi_timeout_t timeout = GASPI_BLOCK);
	
    static void parallel_work(uint64_t timeout_ms = 0);
    
    static void set_context(DLB_Context * arg);
    static DLB_Context * get_context();
    
};


class DLB_Context
{
    DLB_Context_impl * m_impl;
    friend class DLB;
public :
    DLB_Context(DLB_Context_impl * arg):m_impl(arg) {} //implicit conversion
    DLB_Context(); //default : from default config file (if env var is set) or default config
    DLB_Context(int shared_task_segment_size, int algorithm); //user-provided args.
    DLB_Context(const char * config_filename); //from config file
    DLB_Context(const DLB_Context &arg); // clone context
    ~DLB_Context();//! TODO : context deletion has not been checked for memory leaks.
    
    void set_problem(void *problem, size_t task_size, size_t nb_task, void *result, size_t result_size, void (*ptr_task_function)(void*, void*, void*), void * params);    

    DLB_Logger * get_logger();
    DVS_Context * get_DVS_context();
    void set_DVS_context(DVS_Context * arg);
    dlb_int get_rank()const;
    dlb_int get_nb_ranks()const;
    static dlb_int get_nb_contexts();
    dlb_int get_context_id()const;
	void print(std::ostream & out)const;
    
    // DEBUG PRINTERS
#ifdef DEBUG
    void display_state();
    void display_metadatatask();
    void display_metadataresult();
    void display_metadatatmp();
    void display_dequeue();
    void display_Context();
#endif
    friend class DVS_Context;
};

std::ostream & operator << (std::ostream & out, const DLB_Context & arg);
#endif
