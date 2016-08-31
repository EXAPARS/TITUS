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
//  FILE	: dlb_external.cpp
//  CONTENT	:
//
//====================================================================================


// implements the external user-accessible classes of the lib defined in DLB.h as wrappers to the internal implementation

#include <stdio.h>
#include <DLB.hpp>
#include "dlb_internal.hpp"
#include "dlb_context.hpp"

//! TODO : rework fortran interfaces, ensure types validity
extern "C"
{
    // DLB :
    gaspi_return_t dlb_gaspi_proc_init_(gaspi_timeout_t timeout);
    void dlb_set_nb_queues_(int * arg);
    void dlb_parallel_work_();

    // DLB_CONTEXT : 
    void dlb_set_context_(void* arg);
    void dlb_get_context_(void **ret_val);
    void dlb_new_context_(void **ret_val, int *shared_task_segment_size, int *algorithm);
    void dlb_set_problem_(void *context, void *problem, int *task_size, int *nb_task, void *result, int *result_size, void *ptr_task_function, void *params);
    void dlb_get_logger_(void **ret_val, void *context);
    void dlb_get_DVS_context_(void **ret_val, void *context);

    // DLB_LOGGER :
    void dlb_set_DVS_context_(void *context, void *arg);
    void dlb_print_current_session_(void *logger, char *filename);
    void dlb_print_agregated_session_info_(void *logger, char *filename);
    void dlb_print_all_sessions_(void *logger, char *filename);

    void dlb_dump_buffer_(void *logger);
    void dlb_print_buffer_(void *logger, char *filename);
    void dlb_set_autodump_(void *logger, int *val, char *logdir);
}

// *********************************************************************
// ************************* DLB EXTERN C*******************************
// *********************************************************************

// sets the gaspi configuration for the number of queues
// can impact memory usage.
void dlb_set_nb_queues_(int * arg) {
    ASSERT(arg != nullptr);
    if (*arg < 6){
		TITUS_DBG << "error : dlb_set_nb_queues_ : TITUS requires at least 6 queues, arg = " << *arg << std::endl;
		ASSERT(*arg >= 6);
	}
    gaspi_config_t conf;
    ASSERT(gaspi_config_get(&conf));
    conf.queue_num = *arg;
    SUCCESS_OR_DIE(gaspi_config_set(conf));
}

gaspi_return_t dlb_gaspi_proc_init_(gaspi_timeout_t timeout){
	return DLB::gaspi_proc_init(timeout);
}

void dlb_parallel_work_() {
    DLB::parallel_work();
}

void dlb_set_context_(void* arg) {
    DLB::set_context((DLB_Context *)arg);
}

void dlb_get_context_(void **ret_val) {
    *ret_val = DLB::get_context();
}

void dlb_new_context_(void **ret_val, int *shared_task_segment_size, int *algorithm) {
    *ret_val = new DLB_Context(*shared_task_segment_size, *algorithm);
}

void dlb_set_problem_(void *context, void *problem, int *task_size, int *nb_task, void *result, int *result_size, void *ptr_task_function, void *params) {
    DLB_Context *ctx;
    if (context == nullptr) {
        if (DLB::get_context() == nullptr) {
            DLB::set_context(new DLB_Context());
        }
        ctx = DLB::get_context();
    }
    else ctx = (DLB_Context *)context;
    ctx->set_problem(problem, *task_size, *nb_task, result, *result_size, (void (*)(void*, void*, void*))ptr_task_function, params);
}

void dlb_get_logger_(void **ret_val, void *context) {
    *ret_val = ((DLB_Context *)context)->get_logger();
}

void dlb_get_DVS_context_(void **ret_val, void *context) {
    *ret_val = ((DLB_Context *)context)->get_DVS_context();
}

void dlb_set_DVS_context_(void *context, void *arg) {
    ((DLB_Context *)context)->set_DVS_context((DVS_Context *)arg);
}

void dlb_print_current_session_(void *logger, char *filename) {
    if (filename != nullptr && *filename != '\0') {
        std::ostream * out = new std::ofstream(filename);
        ((DLB_Logger *)logger)->print_current_session(*out);
    }
    else{
        ((DLB_Logger *)logger)->print_current_session(std::cout);
    }
}

void dlb_print_agregated_session_info_(void *logger, char *filename) {
    if (filename != nullptr && *filename != '\0') {
        std::ostream * out = new std::ofstream(filename);
        ((DLB_Logger *)logger)->print_agregated_session_info(*out);
    }
    else{
        ((DLB_Logger *)logger)->print_agregated_session_info(std::cout);
    }
}
void dlb_print_all_sessions_(void *logger, char *filename) {
    if (filename != nullptr && *filename != '\0') {
        std::ostream * out = new std::ofstream(filename);
        ((DLB_Logger *)logger)->print_all_sessions(*out);
    }
    else{
        ((DLB_Logger *)logger)->print_all_sessions(std::cout);
    }
}




void dlb_dump_buffer_(void *logger) {
    ((DLB_Logger *)logger)->dump_buffer();
}

void dlb_print_buffer_(void *logger, char *filename) {
    if (filename != nullptr && *filename != '\0') {
        std::ostream * out = new std::ofstream(filename);
        ((DLB_Logger *)logger)->print_buffer(*out);
    }
    else{
        ((DLB_Logger *)logger)->print_buffer(std::cout);
    }
}

void dlb_set_autodump_(void *logger, int *val, char *logdir) {
    ((DLB_Logger *)logger)->set_autodump(*val!=0, logdir);
}

// *********************************************************************
// ***************************** DLB ***********************************
// *********************************************************************
gaspi_return_t DLB::gaspi_proc_init(gaspi_timeout_t timeout){
	return DLB_impl::gaspi_proc_init_impl(timeout);
}

gaspi_return_t DLB::gaspi_barrier(gaspi_group_t group, gaspi_timeout_t timeout){
	return DLB_impl::barrier(group,timeout);
}

void DLB::parallel_work(uint64_t timeout_ms) {
    DLB_impl::parallel_work(timeout_ms);
}
    
void DLB::set_context(DLB_Context * arg) {
    DLB_impl::set_context(arg->m_impl);
}
DLB_Context * DLB::get_context() {
	if (DLB_impl::get_context() == nullptr) return nullptr;
    return new DLB_Context(DLB_impl::get_context());
}

std::ostream & operator <<(std::ostream & out, DLB::DLB_Algorithm const & arg) {
    switch (arg) {
        case DLB::WORK_REQUESTING : out << "WORK_REQUESTING"; break;
        case DLB::WORK_STEALING : out << "WORK_STEALING"; break;
        default : out << "INVALID ALGORITHM VALUE";
    }
    return out;
}

// *********************************************************************
// ************************* DLB_Context *******************************
// *********************************************************************

DLB_Context::DLB_Context() {
    m_impl = new DLB_Context_impl();
}
DLB_Context::DLB_Context(int shared_task_segment_size, int algorithm) {
    m_impl = new DLB_Context_impl(shared_task_segment_size,algorithm);
}
DLB_Context::DLB_Context(const char * config_filename) {
    m_impl = new DLB_Context_impl(config_filename);
}
DLB_Context::DLB_Context(const DLB_Context &arg) {
    m_impl = new DLB_Context_impl(*(arg.m_impl));
}
DLB_Context::~DLB_Context() {
    delete m_impl;
}
void DLB_Context::set_problem(void *problem, size_t task_size, size_t nb_task, void *result, size_t result_size, void (*ptr_task_function)(void*, void*, void*), void * params) {
    m_impl->set_problem(problem, task_size, nb_task, result, result_size, ptr_task_function, params);
}

DLB_Logger * DLB_Context::get_logger() {
    return new DLB_Logger(m_impl->get_logger());
}

DVS_Context * DLB_Context::get_DVS_context() {
    return m_impl->get_DVS_context();
}

void DLB_Context::set_DVS_context(DVS_Context * arg) {
    m_impl->set_DVS_context(arg);
}

dlb_int DLB_Context::get_rank()const {
    return m_impl->get_rank();
}
dlb_int DLB_Context::get_nb_ranks()const {
    return m_impl->get_nb_ranks();
}
dlb_int DLB_Context::get_nb_contexts(){
	return DLB_Context_impl::get_context_count();
}
dlb_int DLB_Context::get_context_id()const{
	return m_impl->get_id();
}
void DLB_Context::print(std::ostream & out)const { m_impl->print_state(out); }


std::ostream & operator << (std::ostream & out, const DLB_Context & arg){
	arg.print(out);
	return out;
}

// DEBUG PRINTERS
#ifdef DEBUG
void DLB_Context::display_state()            { m_impl->display_state(); }
void DLB_Context::display_metadatatask()    { m_impl->display_metadatatask(); }
void DLB_Context::display_metadataresult()    { m_impl->display_metadataresult(); }
void DLB_Context::display_metadatatmp()        { m_impl->display_metadatatmp(); }
void DLB_Context::display_dequeue()            { m_impl->display_dequeue(); }
void DLB_Context::display_Context()            { m_impl->display_Context(); }
#endif
// *********************************************************************
// *************************** DLB_Logger ******************************
// *********************************************************************

void DLB_Logger::print_current_session(std::ostream & out)const{
    m_impl->print_current_session(out);
}
void DLB_Logger::print_agregated_session_info(std::ostream & out)const{
    m_impl->print_agregated_session_info(out);
}
void DLB_Logger::print_all_sessions(std::ostream & out)const{
    m_impl->print_all_sessions(out);
}

void DLB_Logger::dump_buffer() {
    m_impl->dump_buffer();
}
void DLB_Logger::print_buffer(std::ostream & out)const{
    m_impl->print_buffer(out);
}
void DLB_Logger::set_autodump(bool val, std::ostream & out) {
    m_impl->set_autodump(val,out);
}
void DLB_Logger::set_autodump(bool val, const char *logdir){
    m_impl->set_autodump(val,logdir);
}


