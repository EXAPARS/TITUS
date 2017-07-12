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


// implements the external user-accessible classes of the lib defined in TITUS_DLB.h as wrappers to the internal implementation

#include <stdio.h>
#include <TITUS_DLB.hpp>
#include <TITUS_DLB_.h>
#include "TITUS_DLB_internal.hpp"
#include "TITUS_DLB_context.hpp"



// *********************************************************************
// ***************** TITUS_DLB EXTERN C Wrappers impl ************************
// *********************************************************************

gaspi_return_t c_TITUS_DLB_gaspi_proc_init(gaspi_timeout_t timeout){
	//~ std::cout << "c_TITUS_DLB_gaspi_proc_init(timeout = " << timeout << ")" << std::endl; std::cout.flush();
	return TITUS_DLB::gaspi_proc_init(timeout);
}

void c_TITUS_DLB_parallel_work() {
	//~ std::cout << "c_TITUS_DLB_parallel_work()" << std::endl; std::cout.flush();
    TITUS_DLB::parallel_work();
}

void c_TITUS_DLB_set_context(void* arg) {
	//~ std::cout << "c_TITUS_DLB_set_context(arg=" << arg << ")" << std::endl; std::cout.flush();
    TITUS_DLB::set_context((TITUS_DLB_Context *)arg);
}

void c_TITUS_DLB_get_context(void **ret_val) {
	//~ std::cout << "c_TITUS_DLB_get_context(ret_val=" << ret_val << ")" << std::endl; std::cout.flush();
    *ret_val = TITUS_DLB::get_context();
}

void c_TITUS_DLB_new_context(void **ret_val, int shared_task_segment_size, int algorithm) {
	std::cout << "c_TITUS_DLB_new_context(ret_val=" << ret_val << ", shared_task_segment_size=" << shared_task_segment_size << ", algorithm=" << algorithm << ")" << std::endl; std::cout.flush();
	//! DODO : MEMORY LEAK where to free ?
    *ret_val = new TITUS_DLB_Context(shared_task_segment_size, algorithm);
	//~ std::cout << "new_context = " << * ret_val << std::endl;
}

void c_TITUS_DLB_delete_context(void *context ) {
	//~ std::cout << "c_TITUS_DLB_delete_context(context=" << context << ")" << std::endl; std::cout.flush();
	//! DODO : MEMORY LEAK where to free ?
    delete context;
}

void c_TITUS_DLB_set_problem(void *context, void *problem, int task_size, int nb_task, void *result, int result_size, void *ptr_task_function, void *params) {
	//~ std::cout << "c_TITUS_DLB_set_problem("
		//~ << "context=" << context 
		//~ << ", problem=" << problem << ", task_size=" << task_size << ", nb_task=" << nb_task 
		//~ << ", result=" << result << ", result_size=" << result_size
		//~ << ", ptr_task_function= " << ptr_task_function << ", params=" << params 
		//~ << ")" << std::endl; std::cout.flush();
    TITUS_DLB_Context *ctx;
    //!TODO: check functionality
    if (context == nullptr) {
        if (TITUS_DLB::get_context() == nullptr) {
            TITUS_DLB::set_context(new TITUS_DLB_Context());
        }
        ctx = TITUS_DLB::get_context();
    }
    else ctx = (TITUS_DLB_Context *)context;
    ctx->set_problem(problem, task_size, nb_task, result, result_size, (void (*)(void*, void*, void*))ptr_task_function, params);
}

void c_TITUS_DLB_get_logger(void **ret_val, void *context) {
	//~ std::cout << "c_TITUS_DLB_get_logger(ret_val=" << ret_val << ", context=" << context << ")" << std::endl; std::cout.flush();
    *ret_val = ((TITUS_DLB_Context *)context)->get_logger();
}

void c_TITUS_DLB_get_DVS_context(void **ret_val, void *context) {
	//~ std::cout << "c_TITUS_DLB_get_DVS_context(ret_val=" << ret_val << ", context=" << context << ")" << std::endl; std::cout.flush();
    *ret_val = ((TITUS_DLB_Context *)context)->get_DVS_context();
}

void c_TITUS_DLB_set_DVS_context(void *context, void *arg) {
	//~ std::cout << "c_TITUS_DLB_set_DVS_context(context=" << context << ", arg=" << arg << ")" << std::endl; std::cout.flush();
    ((TITUS_DLB_Context *)context)->set_DVS_context((DVS_Context *)arg);
}

void c_TITUS_DLB_print_current_session(void *logger, char *filename) {
	//~ std::cout << "c_TITUS_DLB_print_current_session(logger=" << logger << ", filename=" << filename << ")" << std::endl; std::cout.flush();
    if (filename != nullptr && *filename != '\0') {
        std::ostream * out = new std::ofstream(filename);
        ((TITUS_DLB_Logger *)logger)->print_current_session(*out);
    }
    else{
        ((TITUS_DLB_Logger *)logger)->print_current_session(std::cout);
    }
}

void c_TITUS_DLB_print_agregated_session_info(void *logger, char *filename) {
	//~ std::cout << "c_TITUS_DLB_print_agregated_session_info(logger=" << logger << ", filename=" << filename << ")" << std::endl; std::cout.flush();
    if (filename != nullptr && *filename != '\0') {
        std::ostream * out = new std::ofstream(filename);
        ((TITUS_DLB_Logger *)logger)->print_agregated_session_info(*out);
    }
    else{
        ((TITUS_DLB_Logger *)logger)->print_agregated_session_info(std::cout);
    }
}
void c_TITUS_DLB_print_all_sessions(void *logger, char *filename) {
	//~ std::cout << "c_TITUS_DLB_print_all_sessions(logger=" << logger << ", filename=" << filename << ")" << std::endl; std::cout.flush();
    if (filename != nullptr && *filename != '\0') {
        std::ostream * out = new std::ofstream(filename);
        ((TITUS_DLB_Logger *)logger)->print_all_sessions(*out);
    }
    else{
        ((TITUS_DLB_Logger *)logger)->print_all_sessions(std::cout);
    }
}




void c_TITUS_DLB_dump_buffer(void *logger) {
	//~ std::cout << "c_TITUS_DLB_dump_buffer(logger=" << logger << ")" << std::endl; std::cout.flush();
    ((TITUS_DLB_Logger *)logger)->dump_buffer();
}

void c_TITUS_DLB_print_buffer(void *logger, char *filename) {
	//~ std::cout << "c_TITUS_DLB_print_buffer(logger=" << logger << ", filename=" << filename << ")" << std::endl; std::cout.flush();
    if (filename != nullptr && *filename != '\0') {
        std::ostream * out = new std::ofstream(filename);
        ((TITUS_DLB_Logger *)logger)->print_buffer(*out);
    }
    else{
        ((TITUS_DLB_Logger *)logger)->print_buffer(std::cout);
    }
}

void c_TITUS_DLB_set_autodump(void *logger, int *val, char *logdir) {
	//~ std::cout << "c_TITUS_DLB_set_autodump(logger=" << logger << ", logdir=" << logdir << ")" << std::endl; std::cout.flush();
    ((TITUS_DLB_Logger *)logger)->set_autodump(*val!=0, logdir);
}

// *********************************************************************
// ***************************** TITUS_DLB ***********************************
// *********************************************************************
gaspi_return_t TITUS_DLB::gaspi_proc_init(gaspi_timeout_t timeout){
	return TITUS_DLB_impl::gaspi_proc_init_impl(timeout);
}
bool TITUS_DLB::get_gaspi_init_complete(){
	return TITUS_DLB_impl::get_gaspi_init_complete();
}

gaspi_return_t TITUS_DLB::gaspi_barrier(gaspi_group_t group, gaspi_timeout_t timeout){
	return TITUS_DLB_impl::barrier(group,timeout);
}

void TITUS_DLB::parallel_work(uint64_t timeout_ms) {
    TITUS_DLB_impl::parallel_work(timeout_ms);
}
    
void TITUS_DLB::set_context(TITUS_DLB_Context * arg) {
    TITUS_DLB_impl::set_context(arg->m_impl);
}
TITUS_DLB_Context * TITUS_DLB::get_context() {
	if (TITUS_DLB_impl::get_context() == nullptr) return nullptr;
    return new TITUS_DLB_Context(TITUS_DLB_impl::get_context());
}

std::ostream & operator <<(std::ostream & out, TITUS_DLB::TITUS_DLB_Algorithm const & arg) {
    switch (arg) {
        case TITUS_DLB::WORK_REQUESTING : out << "WORK_REQUESTING"; break;
        case TITUS_DLB::WORK_STEALING : out << "WORK_STEALING"; break;
        default : out << "INVALID ALGORITHM VALUE";
    }
    return out;
}

// *********************************************************************
// ************************* TITUS_DLB_Context *******************************
// *********************************************************************

TITUS_DLB_Context::TITUS_DLB_Context() {
    m_impl = new TITUS_DLB_Context_impl();
}
TITUS_DLB_Context::TITUS_DLB_Context(int shared_task_segment_size, int algorithm) {
    m_impl = new TITUS_DLB_Context_impl(shared_task_segment_size,algorithm);
}
TITUS_DLB_Context::TITUS_DLB_Context(const char * config_filename) {
    m_impl = new TITUS_DLB_Context_impl(config_filename);
}
TITUS_DLB_Context::TITUS_DLB_Context(const TITUS_DLB_Context &arg) {
    m_impl = new TITUS_DLB_Context_impl(*(arg.m_impl));
}
TITUS_DLB_Context::~TITUS_DLB_Context() {
    delete m_impl;
}
void TITUS_DLB_Context::set_problem(void *problem, int task_size, int nb_task, void *result, int result_size, void (*ptr_task_function)(void*, void*, void*), void * params) {
    m_impl->set_problem(problem, task_size, nb_task, result, result_size, ptr_task_function, params);
}

TITUS_DLB_Logger * TITUS_DLB_Context::get_logger() {
    return new TITUS_DLB_Logger(m_impl->get_logger());
}

DVS_Context * TITUS_DLB_Context::get_DVS_context() {
    return m_impl->get_DVS_context();
}

void TITUS_DLB_Context::set_DVS_context(DVS_Context * arg) {
    m_impl->set_DVS_context(arg);
}

TITUS_DLB_int TITUS_DLB_Context::get_rank()const {
    return m_impl->get_rank();
}
TITUS_DLB_int TITUS_DLB_Context::get_nb_ranks()const {
    return m_impl->get_nb_ranks();
}
TITUS_DLB_int TITUS_DLB_Context::get_nb_contexts(){
	return TITUS_DLB_Context_impl::get_context_count();
}
TITUS_DLB_int TITUS_DLB_Context::get_context_id()const{
	return m_impl->get_id();
}
void TITUS_DLB_Context::print(std::ostream & out)const { m_impl->print_state(out); }


std::ostream & operator << (std::ostream & out, const TITUS_DLB_Context & arg){
	arg.print(out);
	return out;
}

// DEBUG PRINTERS
#ifdef DEBUG
void TITUS_DLB_Context::display_state()            { m_impl->display_state(); }
void TITUS_DLB_Context::display_metadatatask()    { m_impl->display_metadatatask(); }
void TITUS_DLB_Context::display_metadataresult()    { m_impl->display_metadataresult(); }
void TITUS_DLB_Context::display_metadatatmp()        { m_impl->display_metadatatmp(); }
void TITUS_DLB_Context::display_dequeue()            { m_impl->display_dequeue(); }
void TITUS_DLB_Context::display_Context()            { m_impl->display_Context(); }
#endif
// *********************************************************************
// *************************** TITUS_DLB_Logger ******************************
// *********************************************************************

int TITUS_DLB_Logger::get_some_metric() {
    return m_impl->get_some_metric();
}
void TITUS_DLB_Logger::activate_some_metric() {
    m_impl->activate_some_metric();
}

void TITUS_DLB_Logger::print_current_session(std::ostream & out)const{
    m_impl->print_current_session(out);
}
void TITUS_DLB_Logger::print_agregated_session_info(std::ostream & out)const{
    m_impl->print_agregated_session_info(out);
}
void TITUS_DLB_Logger::print_all_sessions(std::ostream & out)const{
    m_impl->print_all_sessions(out);
}

void TITUS_DLB_Logger::dump_buffer() {
    m_impl->dump_buffer();
}
void TITUS_DLB_Logger::print_buffer(std::ostream & out)const{
    m_impl->print_buffer(out);
}
void TITUS_DLB_Logger::set_autodump(bool val, std::ostream & out) {
    m_impl->set_autodump(val,out);
}
void TITUS_DLB_Logger::set_autodump(bool val, const char *logdir){
    m_impl->set_autodump(val,logdir);
}


