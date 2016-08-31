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
//  FILE	: dlb_context.cpp
//  CONTENT	:
//
//====================================================================================


#include <GASPI.h>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <algorithm>

#include <dlb_gaspi_tools.hpp>
#include "dlb_internal.hpp"
#include "dlb_context.hpp"


/**
 * Function.\n
 * Defines and initializes variables used inside library.\n
 * The size of shared segment to communicate is set by user.\n
 * 
 * @param problem : Number of particles
 * @param task_size : File containing the coordinates
 * @param nb_task : File containing the coordinates
 * @param result : File containing the coordinates
 * @param result_size : File containing the coordinates
 * @param ptr_task_function : File containing the coordinates
 * @param shared_task_segment_size : File containing the coordinates
 * @param algorithm : load balancing algorithm used DLB::WORK_REQUESTING or DLB::WORK_STEALING
 * 
 */


// *********************************************************************
// ************************** CONSTRUCTORS *****************************
// *********************************************************************




// DEFAULT CONSTRUCTOR
DLB_Context_impl::DLB_Context_impl():
    registrations_status(nullptr), id(context_count++), logger(this), DVS_context(nullptr), ptr_dequeue(nullptr),
    algorithm(DLB_DEFAULT_ALGORITHM),
    shared_task_segment_size(10000),
    shared_result_segment_size(0),shared_tmp_result_segment_size(0),shared_scratch_segment_size(0),
    segment_task(-1), segment_result(-1), segment_tmp(-1),segment_scratch(-1),// implicit cast from signed to unsigned
    ptr_segment_result(nullptr), ptr_segment_tmp(nullptr), ptr_segment_scratch(nullptr)
{
    dlb_init_context();
    assert(check_status());
    //default : from default config file (if env var is set) or default config
    //! TODO : get config filename from env
    //SUCCESS_OR_DIE( gaspi_barrier (GASPI_GROUP_ALL, GASPI_BLOCK) );
    DVS_context = new DVS_Context(new DLB_Context(this));
}

// SIMPLE CONSTRUCTOR
DLB_Context_impl::DLB_Context_impl(int shared_task_segment_size, int algorithm):
    registrations_status(nullptr), id(context_count++), logger(this), DVS_context(nullptr), ptr_dequeue(nullptr),
    algorithm(algorithm), shared_task_segment_size(shared_task_segment_size),
    shared_result_segment_size(0),shared_tmp_result_segment_size(0),shared_scratch_segment_size(0),
    segment_task(-1), segment_result(-1), segment_tmp(-1),segment_scratch(-1),// implicit cast from signed to unsigned
    ptr_segment_result(nullptr), ptr_segment_tmp(nullptr), ptr_segment_scratch(nullptr)
{
    dlb_init_context();
    assert(check_status());
    DVS_context = new DVS_Context(new DLB_Context(this));
}

// FROM CONFIG FILE CONSTRUCTOR
DLB_Context_impl::DLB_Context_impl(const char * config_file_name):
    registrations_status(nullptr), id(context_count++), logger(this), DVS_context(nullptr), ptr_dequeue(nullptr),
    shared_result_segment_size(0),shared_tmp_result_segment_size(0),shared_scratch_segment_size(0),
    segment_task(-1), segment_result(-1), segment_tmp(-1),segment_scratch(-1),// implicit cast from signed to unsigned
    ptr_segment_result(nullptr), ptr_segment_tmp(nullptr), ptr_segment_scratch(nullptr)
{
    dlb_init_context();
    assert(check_status());
    DVS_context = new DVS_Context(new DLB_Context(this));
}

// CLONE CONTEXT CONSTRUCTOR
DLB_Context_impl::DLB_Context_impl(const DLB_Context_impl & arg):
    registrations_status(nullptr), id(context_count++), logger(this), DVS_context(nullptr), ptr_dequeue(nullptr),
	algorithm(arg.algorithm), shared_task_segment_size(arg.shared_task_segment_size),
    shared_result_segment_size(0),shared_tmp_result_segment_size(0),shared_scratch_segment_size(0),
    segment_task(-1), segment_result(-1), segment_tmp(-1),segment_scratch(-1),// implicit cast from signed to unsigned
    ptr_segment_result(nullptr), ptr_segment_tmp(nullptr), ptr_segment_scratch(nullptr)
{
    dlb_init_context();
    assert(check_status());
    DVS_context = new DVS_Context(new DLB_Context(this));
}

DLB_Context_impl::~DLB_Context_impl() {
	if ( registrations_status != nullptr )  delete(registrations_status);
	// explicitely call destructor of segment metadata (because they were contructed using placement new)
    if ( get_metadata_task()   != nullptr ) get_metadata_task()   -> ~MetadataTask();
    if ( get_metadata_result() != nullptr ) get_metadata_result() -> ~MetadataResult();
    if ( get_metadata_tmp()    != nullptr ) get_metadata_tmp()    -> ~MetadataTmp();
	
	if ( ptr_dequeue != nullptr ) free(ptr_dequeue);
	
	if ( segment_task    != -1 ) gaspi_segment_delete(segment_task);
	if ( segment_result  != -1 ) gaspi_segment_delete(segment_result);
	if ( segment_tmp     != -1 ) gaspi_segment_delete(segment_tmp);
	if ( segment_scratch != -1 ) gaspi_segment_delete(segment_scratch);
	
	delete(DVS_context); //! TODO : allow DVS contexts to be std::moved : one may want to keep the DVS_Context while deleting the DLB_Context
}

size_t DLB_Context_impl::context_count = 0;


// *********************************************************************
// ****************** UTILS & CONNECTION MANAGEMENT ********************
// *********************************************************************


void DLB_Context_impl::reset() { // resets all states and forgets about any task
    get_metadata_task()->reset();
    get_metadata_result()->reset(get_rank(),get_nb_ranks());
}

//! TODO : move to communication layer manager
void DLB_Context_impl::register_to_rank(gaspi_rank_t arg) {
    //std::cout << "rank " << get_rank() << " : register_to_rank(" << arg << ")" << std::endl;
    
    
    assert(arg < get_nb_ranks());
    assert(segment_task != (decltype(segment_task))-1);
    assert(segment_result != (decltype(segment_result))-1);
    assert(segment_tmp != (decltype(segment_tmp))-1);
    
    if (registrations_status[arg] == true) return;

    if (gaspi_config_build_infrastructure != GASPI_TOPOLOGY_DYNAMIC){
		gaspi_connect(arg,GASPI_BLOCK);
		SUCCESS_OR_DIE( gaspi_segment_register( segment_task,   arg, DLB_GASPI_TIMEOUT));
		SUCCESS_OR_DIE( gaspi_segment_register( segment_result, arg, DLB_GASPI_TIMEOUT));
		SUCCESS_OR_DIE( gaspi_segment_register( segment_tmp,    arg, DLB_GASPI_TIMEOUT));
    }
 
    registrations_status[arg] = true;
}
void DLB_Context_impl::open_segment_to_thieves(gaspi_segment_id_t arg) {
    // check that segment exists
    if (gaspi_config_build_infrastructure != GASPI_TOPOLOGY_NONE) return;
    gaspi_pointer_t tmp;
    assert(gaspi_segment_ptr(arg,&tmp) == GASPI_SUCCESS);

    //SUCCESS_OR_DIE( gaspi_connect( arg, GASPI_BLOCK));
    //~ std::stringstream msg("");
    //~ msg << " : registered segment " << ((uint)arg) << " to";
    //~ for (size_t i=0;i<get_nb_ranks();i++) {
        //~ if (registrations_status[i]==true){
            //~ SUCCESS_OR_DIE( gaspi_segment_register( arg, i, GASPI_BLOCK));
            //~ msg << " [" << i << "]";
        //~ }
    //~ }
    //~ TITUS_DBG << msg.str() << std::endl; //TITUS_DBG.flush();
}

void DLB_Context_impl::print_registration_status(std::ostream & out) {
    assert(registrations_status != nullptr);
    
    out << "rank " << get_rank() << " :";
    for (size_t i=0;i<get_nb_ranks();i++) {
        if (registrations_status[i]==true){
			out << " [" << i << "]";
		}
    }
    out << std::endl;
}

bool DLB_Context_impl::check_status() {
    bool flag = true;
    gaspi_state_vector_t vec = (gaspi_state_vector_t) malloc(get_nb_ranks() * sizeof(char)); ASSERT(vec != nullptr);
    gaspi_state_vec_get(vec);
        
    for (size_t i=0 ; i < get_nb_ranks() ; i++) {
        if (vec[i] == GASPI_STATE_CORRUPT) {
            TITUS_DBG << "DETECTED CORRUPTION ON RANK " << i << std::endl; //TITUS_DBG.flush();
            flag = false;
        }
        else {
            //TITUS_DBG << "status of rank " << i << "ok" << std::endl; //TITUS_DBG.flush();
        }
    }
    free(vec);
    return flag;
}

// *********************************************************************
// ************************** CORE FEATURES ****************************
// *********************************************************************

void DLB_Context_impl::dlb_init_segment(gaspi_segment_id_t segment_id, void ** local_mem_ptr, gaspi_size_t segment_size, bool use_simple_alloc){
    logger.signal_start_segment_init();

    if (use_simple_alloc)
     { SUCCESS_OR_DIE( gaspi_segment_alloc(    segment_id,   segment_size,   GASPI_BLOCK)); }
    else
     { SUCCESS_OR_DIE( gaspi_segment_create(   segment_id,   segment_size,   GASPI_GROUP_ALL, GASPI_BLOCK, GASPI_ALLOC_DEFAULT)); }
    SUCCESS_OR_DIE( gaspi_segment_ptr ( segment_id, local_mem_ptr ));
    ASSERT(*local_mem_ptr != nullptr);
    logger.signal_end_segment_init();
}

static dlb_sig_handler dummy_sig_handler;

void DLB_Context_impl::dlb_init_context() { //shared_task_segment_size
    //dlb_sig_handler dummy;
    //ASSERT(dummy_sig_handler.is_init());
	//std::cout << "DLB_Context_impl::dlb_init_context : self tid = " << pthread_self() << std::endl;
	TITUS_DBG << "TITUS_PROC_FREQ = " << TITUS_PROC_FREQ << std::endl; TITUS_DBG.flush();
    logger.signal_start_DLB_session(); // creating a session for logging context init time
    
    DEBUG_PRINT("=====================DLB::init_library=====================\n");
    
    gaspi_config_t config;
    SUCCESS_OR_DIE(gaspi_config_get(&config));
    gaspi_config_build_infrastructure = config.build_infrastructure;
    
    
    if  (get_rank() == 0)
    {    
        gaspi_number_t notification_num;
        gaspi_number_t queue_size_max;
        gaspi_size_t transfer_size_max;
        DEBUG_PRINT("gaspi_notification_id_t %d\n", sizeof(gaspi_notification_id_t));   //2
        DEBUG_PRINT("gaspi_notification_t %d\n", sizeof(gaspi_notification_t));         //4
        DEBUG_PRINT("gaspi_atomic_value_t %d\n", sizeof(gaspi_atomic_value_t));         //8
        DEBUG_PRINT("gaspi_number_t %d\n", sizeof(gaspi_number_t));                     //4
        DEBUG_PRINT("gaspi_size_t %d\n", sizeof(gaspi_size_t));                         //8
        DEBUG_PRINT("gaspi_rank_t %d\n", sizeof(gaspi_rank_t));                         //2
        
        SUCCESS_OR_DIE( gaspi_notification_num(&notification_num));
        DEBUG_PRINT("Max number of available notifications : gaspi_notification_num=%llu\n", notification_num);
        SUCCESS_OR_DIE( gaspi_passive_transfer_size_max (&transfer_size_max));
        DEBUG_PRINT("Max size of data for one passive gaspi communication : transfer_size_max=%llu\n", transfer_size_max);
        SUCCESS_OR_DIE( gaspi_transfer_size_max (&transfer_size_max));
        DEBUG_PRINT("Max size of data for one gaspi communication : transfer_size_max=%llu\n", transfer_size_max);
        SUCCESS_OR_DIE( gaspi_queue_size_max (&queue_size_max));
        DEBUG_PRINT("Max number of queue : queue_size_max=%llu\n", queue_size_max);
    }
    
    
    //----------------------------------------------------------------------------------------------------------
    // Alloc Dequeue
    //----------------------------------------------------------------------------------------------------------
    this->ptr_dequeue                           = (Dequeue *) malloc(sizeof(Dequeue)); ASSERT(this->ptr_dequeue != nullptr);
    this->ptr_dequeue->problem                  = (void *) malloc(this->shared_task_segment_size*sizeof(char)); ASSERT(this->ptr_dequeue->problem != nullptr);
    this->ptr_dequeue->problem_size_allocated   = this->shared_task_segment_size;
    
    //----------------------------------------------------------------------------------------------------------
    // Initialization GASPI structure for communication
    //----------------------------------------------------------------------------------------------------------
    
    if(algorithm == DLB::WORK_STEALING)          this->ptr_algorithm_function = DLB_impl::work_stealing;
    else if(algorithm == DLB::WORK_REQUESTING)   this->ptr_algorithm_function = DLB_impl::work_requesting;
    
    this->shared_task_segment_size       = (gaspi_size_t) shared_task_segment_size;
    
    this->segment_task                   = 4* id + 0;
    this->segment_result                 = 4* id + 1;
    this->segment_tmp                    = 4* id + 2;
    this->segment_scratch                = 4* id + 3;
    
    this->queue_task                     = 3* id + 0;
    this->queue_result                   = 3* id + 1;
    this->queue_scratch                  = 3* id + 2;
    
    
    this->ptr_segment_task               = NULL;
    this->ptr_segment_result             = NULL;
    this->ptr_segment_tmp                = NULL;
    
    
    dlb_init_segment(segment_task, &ptr_segment_task, shared_task_segment_size, gaspi_config_build_infrastructure == GASPI_TOPOLOGY_NONE);
    new (ptr_segment_task) MetadataTask(this->shared_task_segment_size, segment_task);
    
    
    // init registration status
    registrations_status = new bool[get_nb_ranks()];//! TODO : move to communication layer manager
    for (gaspi_rank_t i = 0 ; i < get_nb_ranks() ; i++) registrations_status[i] = false;//! TODO : move to communication layer manager
    registrations_status[get_rank()] = true;
    
    SUCCESS_OR_DIE( DLB::gaspi_barrier (GASPI_GROUP_ALL, GASPI_BLOCK) );
    if (get_rank() == 0) std::cout << " ---------------- CONTEXT INIT DONE ON ALL RANKS  ---------------- " << std::endl;
    
    logger.signal_end_DLB_session();
    
}


void DLB_Context_impl::set_problem(void *problem, size_t task_size, size_t nb_task, void *result, size_t result_size, void (*ptr_task_function)(void*, void*, void*), void * ptr_task_params)
{
    TITUS_DBG << "DLB_Context_impl::set_problem(void *problem=" << problem << ", int task_size=" << task_size << ", int nb_task=" << nb_task << ", void *result=" << result << ", int result_size=" << result_size << ", void (*ptr_task_function)(void*, void*, void*)=" << ptr_task_function << ", void * ptr_task_params=" << ptr_task_params << ");" << std::endl; TITUS_DBG.flush();
	if (nb_task > 0){
		ASSERT(problem != nullptr);
		ASSERT(result != nullptr);
	}
	ASSERT(task_size > 0);
	ASSERT(result_size > 0);
	
    logger.signal_start_DLB_session();

    int first = 1;
    if(this->shared_result_segment_size == 0)    first = 0;

    //reallocation si la taille allouer est trop petite
    if(task_size*nb_task > this->ptr_dequeue->problem_size_allocated)
    {
        TITUS_DBG << "WARNING : DLB_Context_impl::set_problem : dequeue reallocation : " << this->ptr_dequeue->problem_size_allocated << " -> " << task_size*nb_task << " bytes" << std::endl; TITUS_DBG.flush();
        this->ptr_dequeue->problem_size_allocated = task_size*nb_task;
        this->ptr_dequeue->problem   = (void *) realloc(this->ptr_dequeue->problem, task_size*nb_task);
        ASSERT(this->ptr_dequeue->problem != nullptr);
    }

    //----------------------------------------------------------------------------------------------------------
    //assert if shared segment are too small
    //----------------------------------------------------------------------------------------------------------
    if(first == 1)
    {
        int nb_max_task_steal       = (this->shared_task_segment_size-sizeof(MetadataTask))/task_size;
        int nb_max_result_returned  = (this->shared_tmp_result_segment_size-sizeof(MetadataTmp))/result_size;
        assert((this->shared_task_segment_size - sizeof(MetadataTask)) >= task_size);
        assert(nb_max_task_steal <= nb_max_result_returned);
    }

    //copie du probleme dans notre librarie
    TITUS_DBG << "DLB_Context_impl::set_problem : copying problem to dequeue : memcpy(" << this->ptr_dequeue->problem << ", " << problem << ", " << (size_t)task_size*nb_task << ");" << std::endl; TITUS_DBG.flush();
    memcpy(this->ptr_dequeue->problem, problem, (size_t)task_size*nb_task);
    TITUS_DBG << "copy completed"; TITUS_DBG.flush();

    //! TODO : GASPI segments might not have a valid size.
    //if (this->ptr_segment_result != nullptr)  SUCCESS_OR_DIE( gaspi_segment_delete(this->segment_result));
    //if (this->ptr_segment_tmp != nullptr)     SUCCESS_OR_DIE( gaspi_segment_delete(this->segment_tmp));

    // first call : allocate required gaspi segments
    if(first == 0){
        //calcul de la taille necessaire de segment
        this->shared_tmp_result_segment_size   = (gaspi_size_t)((this->shared_task_segment_size/task_size)*result_size + sizeof(MetadataTmp));         //calcul of this->shared_tmp_result_segment_size for save memory
        this->shared_result_segment_size       = (gaspi_size_t)((this->shared_task_segment_size/task_size)*result_size + sizeof(MetadataResult))*2;  //calcul of this->shared_result_segment_size for save memory
        this->shared_scratch_segment_size      = std::max(shared_result_segment_size,shared_tmp_result_segment_size);

        uint64_t start = rdtsc();
        dlb_init_segment(segment_result,      &ptr_segment_result,      shared_result_segment_size,        !gaspi_config_build_infrastructure);
        dlb_init_segment(segment_tmp,         &ptr_segment_tmp,         shared_tmp_result_segment_size,    !gaspi_config_build_infrastructure);
		dlb_init_segment(segment_scratch, 	  &ptr_segment_scratch,     shared_scratch_segment_size,       !gaspi_config_build_infrastructure);
        TITUS_DBG << "DLB_Context_impl::set_problem : segments init done (" << (start - rdtsc()) / (TITUS_PROC_FREQ/1000) << " ms)" << std::endl; TITUS_DBG.flush();
    }
    
    logger.signal_start_problem_init();

    this->task_result_counter            = 0; //! TODO : move to logger
    this->comm_send_counter              = 0; //! TODO : move to logger
    
    //----------------------------------------------------------------------------------------------------------
    // Initialization Dequeue
    //----------------------------------------------------------------------------------------------------------
    this->ptr_dequeue->owner_task_rank    = get_rank();
    this->ptr_dequeue->head               = 0;
    this->ptr_dequeue->tail               = nb_task;
    this->ptr_dequeue->base_task          = this->ptr_dequeue->problem;
    
    this->ptr_dequeue->end_task_id        = nb_task;           // first task_id is 1 (id = 0 is reserved for init)
    this->ptr_dequeue->task_size          = task_size;
    this->ptr_dequeue->result_size        = (dlb_int) result_size;
    this->ptr_dequeue->ptr_task_function  = ptr_task_function;
    this->ptr_dequeue->ptr_task_params    = ptr_task_params;

    
    //----------------------------------------------------------------------------------------------------------
    // Initialization Metadata on result segment
    //----------------------------------------------------------------------------------------------------------
    new (ptr_segment_result) MetadataResult(shared_result_segment_size, segment_result, get_rank(), get_nb_ranks(), nb_task, result_size, result);
 
    //----------------------------------------------------------------------------------------------------------
    // Initialization Metadata on temporary segment
    //----------------------------------------------------------------------------------------------------------
    new (ptr_segment_tmp) MetadataTmp(shared_tmp_result_segment_size, segment_tmp, result_size);
    
    //verify if the size of segment_tmp is enough big
    assert( this->shared_tmp_result_segment_size >= (result_size + sizeof(MetadataTmp)) );
    assert( (dlb_int)this->shared_task_segment_size >= (dlb_int)(task_size + sizeof(MetadataTask)) );
    
    logger.signal_end_problem_init();
    logger.signal_tasks_spawned(nb_task);
    
		
    SUCCESS_OR_DIE( DLB::gaspi_barrier (GASPI_GROUP_ALL, GASPI_BLOCK) );
    if (get_rank() == 0) std::cout << " ---------------- PROBLEM SET ON ALL RANKS ----------------" << std::endl;
    
    //assert(check_status()); // aucun sens hors du mode random
    
    // register termination detection neighbors
    //! TODO : move global_termination_detected detection tree to the one on which is built the small world (infixed numerotation)
    if(get_rank() != 0)                  register_to_rank((get_rank() - 1)/2);
    if(2*get_rank()+1 < get_nb_ranks())  register_to_rank(2*get_rank()+1);
    if(2*get_rank()+2 < get_nb_ranks())  register_to_rank(2*get_rank()+2);
    
    open_segment_to_thieves(segment_result);
    open_segment_to_thieves(segment_tmp);
    open_segment_to_thieves(segment_task);

    std::stringstream outname("");
    outname << "registration_table" << std::setfill('0') << std::setw(4) << get_rank() << ".dat";
       std::ostream * out = new std::ofstream(outname.str().c_str(),std::ios_base::ate);
    print_registration_status(*out);

    //assert(check_status());
        
//	sstring msg("");	
	TITUS_DBG << " set_problem done : "
		<< "shared_task_segment_size = " << shared_task_segment_size
		<< ", shared_result_segment_size = " << shared_result_segment_size
		<< ", shared_tmp_result_segment_size = " << shared_tmp_result_segment_size
		<< ", shared_scratch_segment_size = " << shared_scratch_segment_size
		<< std::endl; //TITUS_DBG.flush();

}


void DLB_Context_impl::submit_results(void * start, size_t nb_results, dlb_int first_task_id) {
    void * user_results_start = get_metadata_result()->usr_results;
    size_t result_size = get_metadata_result()->result_size;
    
    size_t user_results_offset = result_size * first_task_id;
    void * copy_dest = ADD_PTR(user_results_start, user_results_offset);
    size_t copy_size = nb_results * result_size;
    
   	//TITUS_DBG << "DLB_Context_impl::submit_results : submitted " << nb_results << " results, starting with id " << first_task_id << std::endl;

    //TITUS_DBG << "DLB_Context_impl::submit_results(start=" << start << ", nb_resut=" << nb_results << ", first_task_id=" << first_task_id << ")" << std::endl
	//			<< "> memcpy(dest=" << copy_dest << ", src=" << start << ", size=" << copy_size << ")" << std::endl; //TITUS_DBG.flush();
    memcpy(copy_dest, start , copy_size);
    get_metadata_result()->nb_result += nb_results;
	
    //assert that number of result is not greater than expected number of result
    if (get_metadata_result()->nb_result > get_metadata_result()->nb_usr_results){
		TITUS_DBG << "ASSERT IS GOING TO FAIL : " << get_metadata_result()->nb_result << " > " << get_metadata_result()->nb_usr_results << std::endl; //TITUS_DBG.flush();
	}
    ASSERT(get_metadata_result()->nb_result <= get_metadata_result()->nb_usr_results);
}

void DLB_Context_impl::submit_results(BufferElt * arg){
    submit_results(ADD_PTR(arg , sizeof(BufferElt)), arg->nb_result, arg->first_task_id);
}


void DLB_Context_impl::push_results_buffer(){
	get_metadata_result()->try_flush_buffer_elts_packed();
}



std::ostream & operator << (std::ostream & out , const DLB_Context_impl & arg){
	arg.print_state(out);
	return out;
}

void DLB_Context_impl::print_state(std::ostream & out)const{
	out << "DLB_Context_impl : " << std::endl;
    out << "  ";
    out << "shared_task_segment_size = " << shared_task_segment_size << ", ";
    out << "shared_result_segment_size = " << shared_result_segment_size << ", ";
    out << "shared_tmp_result_segment_size = " << shared_tmp_result_segment_size << ", ";
    out << "shared_scratch_segment_size = " << shared_scratch_segment_size << std::endl;
    
    out << "segment_task : (" << sizeof(decltype (*get_metadata_task())) << " bytes" << ((uint64_t)get_metadata_task()) << ")" << std::endl;
    if (get_metadata_task() != nullptr) get_metadata_task()->print(out,"  "); else out << "Warning : segment not initialized" << std::endl;
    out << "segment_result : (" << sizeof(decltype (*get_metadata_result())) << " bytes @" << ((uint64_t)get_metadata_result()) << ")" << std::endl;
    if (get_metadata_result() != nullptr) get_metadata_result()->print(out,"  "); else out << "Warning : segment not initialized" << std::endl;
    out << "segment_tmp : (" << sizeof(decltype (*get_metadata_tmp())) << " bytes)" << ((uint64_t)get_metadata_tmp()) << "" << std::endl;
    if (get_metadata_tmp() != nullptr) get_metadata_tmp()->print(out,"  "); else out << "Warning : segment not initialized" << std::endl;
    //~ out << "segment_scratch : " << std::endl;
    //~ out << *ptr_segment_scratch;
    
	gaspi_number_t queue_size_max; 		SUCCESS_OR_DIE(gaspi_queue_size_max(&queue_size_max));
	gaspi_number_t task_queue_size; 	SUCCESS_OR_DIE(gaspi_queue_size(queue_task, &task_queue_size));
	gaspi_number_t result_queue_size; 	SUCCESS_OR_DIE(gaspi_queue_size(queue_result, &result_queue_size));
	gaspi_number_t scratch_queue_size; 	SUCCESS_OR_DIE(gaspi_queue_size(queue_scratch, &scratch_queue_size));

    out << "queue_task = " << (uint)queue_task << " (" << task_queue_size << "/" << queue_size_max << "), ";
    out << "queue_result = " << (uint)queue_result << " (" << result_queue_size << "/" << queue_size_max << "), ";
    out << "queue_scratch = " << (uint)queue_scratch << " (" << scratch_queue_size << "/" << queue_size_max << "), " << std::endl;
    out << "task_result_counter = " << task_result_counter << ", ";
    out << "comm_send_counter = " << comm_send_counter << ", " << std::endl;

    out << "ptr_segment_task = " << ptr_segment_task << ", ";
    out << "ptr_segment_result = " << ptr_segment_result << ", ";
    out << "ptr_segment_tmp = " << ptr_segment_tmp << ", ";
    out << "ptr_segment_scratch = " << ptr_segment_scratch << ", " << std::endl;
	
}
