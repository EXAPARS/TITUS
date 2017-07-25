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


#ifndef __TITUS_DLB_HPP__
#define __TITUS_DLB_HPP__

#if __cplusplus < 201103L
#define nullptr 0
#endif

#define TITUS_DLB_DEFAULT_ALGORITHM TITUS_DLB::WORK_STEALING

#include <iostream>

typedef unsigned long int TITUS_DLB_int;

class TITUS_DLB_Context;

#include <GASPI.h>
#include "TITUS_DLB_gaspi_tools.hpp"
#include <DVS.hpp>
#include <TITUS_Logger.hpp>

// proper library use

class TITUS_DLB_Context_impl;

class TITUS_DLB{
public :
	enum TITUS_DLB_Algorithm {WORK_STEALING=0, WORK_REQUESTING=1};
	
	static gaspi_return_t gaspi_proc_init(gaspi_timeout_t timeout = GASPI_BLOCK);
	static bool get_gaspi_init_complete();
	static gaspi_return_t gaspi_barrier(gaspi_group_t group, gaspi_timeout_t timeout = GASPI_BLOCK);
	
    static void parallel_work(uint64_t timeout_ms = 0);
    
    static void set_context(TITUS_DLB_Context * arg);
    static TITUS_DLB_Context * get_context();
    
};


class TITUS_DLB_Context
{
    TITUS_DLB_Context_impl * m_impl;
    friend class TITUS_DLB;
public :
    TITUS_DLB_Context(TITUS_DLB_Context_impl * arg):m_impl(arg) {} //implicit conversion
    TITUS_DLB_Context(); //default : from default config file (if env var is set) or default config
    TITUS_DLB_Context(int shared_task_segment_size, int algorithm); //user-provided args.
    TITUS_DLB_Context(const char * config_filename); //from config file
    TITUS_DLB_Context(const TITUS_DLB_Context &arg); // clone context
    ~TITUS_DLB_Context();
    
    void set_problem(void *problem, int task_size, int nb_task, void *result, int result_size, void (*ptr_task_function)(void*, void*, void*), void * params);    

    TITUS_Logger * get_logger();
    DVS_Context * get_DVS_context();
    void set_DVS_context(DVS_Context * arg);
    TITUS_DLB_int get_rank()const;
    TITUS_DLB_int get_nb_ranks()const;
    static TITUS_DLB_int get_nb_contexts();
    TITUS_DLB_int get_context_id()const;
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

std::ostream & operator << (std::ostream & out, const TITUS_DLB_Context & arg);
#endif
