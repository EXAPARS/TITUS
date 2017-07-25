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

#ifndef __TITUS_DLB_CONTEXT_HPP__
// normal compilation


#include <GASPI.h>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <algorithm>

#include <TITUS_DLB_gaspi_tools.hpp>
#include "TITUS_DLB_internal.hpp"
#include "TITUS_DLB_context.hpp"


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
 * @param algorithm : load balancing algorithm used TITUS_DLB::WORK_REQUESTING or TITUS_DLB::WORK_STEALING
 * 
 */


// *********************************************************************
// ************************** CONSTRUCTORS *****************************
// *********************************************************************




// DEFAULT CONSTRUCTOR
TITUS_DLB_Context_impl::TITUS_DLB_Context_impl():
    id(context_count++),
    algorithm(TITUS_DLB_DEFAULT_ALGORITHM),
    shared_task_segment_size(10000 + sizeof(MetadataTask)),
    shared_result_segment_size(0),shared_tmp_result_segment_size(0),shared_scratch_segment_size(0),
    segment_task(-1), segment_result(-1), segment_tmp(-1),segment_scratch(-1),
    ptr_segment_result(nullptr), ptr_segment_tmp(nullptr), ptr_segment_scratch(nullptr),
    logger(),
    parallel_work_session_time(&logger, "parallel_work_session_time"),
	init_problem_time(&logger, "init_problem_time"),
	init_segment_time(&logger, "init_segment_time"),
    spawned_tasks_count(&logger, "spawned_tasks_count"),
    time_spent_working(&logger, "time_spent_working"),
    time_spent_solving_tasks(&logger, "time_spent_solving_tasks", logger_histogram_pow, logger_histogram_start),
    solved_tasks_count(&logger, "solved_tasks_count"),
	time_spent_moving_from_segment(&logger, "time_spent_moving_from_segment"),
	tasks_moved_from_segment_count(&logger, "tasks_moved_from_segment_count"),
	time_spent_moving_from_dequeue(&logger, "time_spent_moving_from_dequeue"),
	tasks_moved_from_dequeue_count(&logger, "tasks_moved_from_dequeue_count"),
	try_theft_count(&logger, "try_theft_count"),
	hit_count(&logger, "hit_count"),
	stolen_tasks_count(&logger, "stolen_tasks_count"),
	miss_notask_count(&logger, "miss_notask_count"),
	miss_target_locked_count(&logger, "miss_target_locked_count"),
	miss_free_for_copy(&logger, "miss_free_for_copy"),
	current_theft(&logger, "current_theft"),
	time_spent_stealing(&logger, "time_spent_stealing", logger_histogram_pow, logger_histogram_start),
	time_spent_returning_results(&logger, "time_spent_returning_results", logger_histogram_pow, logger_histogram_start),
	results_pushed_count(&logger, "results_pushed_count")
{
    TITUS_DLB_init_context();
    assert(check_status());
    //default : from default config file (if env var is set) or default config
    //! todo get config filename from env
    //SUCCESS_OR_DIE( gaspi_barrier (GASPI_GROUP_ALL, GASPI_BLOCK) );
    DVS_context = new DVS_Context(new TITUS_DLB_Context(this));
}

// SIMPLE CONSTRUCTOR
TITUS_DLB_Context_impl::TITUS_DLB_Context_impl(int shared_task_segment_size, int algorithm):
    id(context_count++), DVS_context(nullptr), algorithm(algorithm), shared_task_segment_size(shared_task_segment_size + sizeof(MetadataTask)),
    shared_result_segment_size(0),shared_tmp_result_segment_size(0),shared_scratch_segment_size(0),
    segment_task(-1), segment_result(-1), segment_tmp(-1),segment_scratch(-1),
    ptr_segment_result(nullptr), ptr_segment_tmp(nullptr), ptr_segment_scratch(nullptr),
    logger(),
    parallel_work_session_time(&logger, "parallel_work_session_time"),
	init_problem_time(&logger, "init_problem_time"),
	init_segment_time(&logger, "init_segment_time"),
    spawned_tasks_count(&logger, "spawned_tasks_count"),
    time_spent_working(&logger, "time_spent_working"),
    time_spent_solving_tasks(&logger, "time_spent_solving_tasks", logger_histogram_pow, logger_histogram_start),
    solved_tasks_count(&logger, "solved_tasks_count"),
	time_spent_moving_from_segment(&logger, "time_spent_moving_from_segment"),
	tasks_moved_from_segment_count(&logger, "tasks_moved_from_segment_count"),
	time_spent_moving_from_dequeue(&logger, "time_spent_moving_from_dequeue"),
	tasks_moved_from_dequeue_count(&logger, "tasks_moved_from_dequeue_count"),
	try_theft_count(&logger, "try_theft_count"),
	hit_count(&logger, "hit_count"),
	stolen_tasks_count(&logger, "stolen_tasks_count"),
	miss_notask_count(&logger, "miss_notask_count"),
	miss_target_locked_count(&logger, "miss_target_locked_count"),
	miss_free_for_copy(&logger, "miss_free_for_copy"),
	current_theft(&logger, "current_theft"),
	time_spent_stealing(&logger, "time_spent_stealing", logger_histogram_pow, logger_histogram_start),
	time_spent_returning_results(&logger, "time_spent_returning_results", logger_histogram_pow, logger_histogram_start),
	results_pushed_count(&logger, "results_pushed_count")
{
    TITUS_DLB_init_context();
    assert(check_status());
    DVS_context = new DVS_Context(new TITUS_DLB_Context(this));
}

// FROM CONFIG FILE CONSTRUCTOR
TITUS_DLB_Context_impl::TITUS_DLB_Context_impl(const char * config_file_name):
    id(context_count++),
    shared_result_segment_size(0),shared_tmp_result_segment_size(0),shared_scratch_segment_size(0),
    segment_task(-1), segment_result(-1), segment_tmp(-1),segment_scratch(-1),
    ptr_segment_result(nullptr), ptr_segment_tmp(nullptr), ptr_segment_scratch(nullptr),
    logger(),
    parallel_work_session_time(&logger, "parallel_work_session_time"),
	init_problem_time(&logger, "init_problem_time"),
	init_segment_time(&logger, "init_segment_time"),
    spawned_tasks_count(&logger, "spawned_tasks_count"),
    time_spent_working(&logger, "time_spent_working"),
    time_spent_solving_tasks(&logger, "time_spent_solving_tasks", logger_histogram_pow, logger_histogram_start),
    solved_tasks_count(&logger, "solved_tasks_count"),
	time_spent_moving_from_segment(&logger, "time_spent_moving_from_segment"),
	tasks_moved_from_segment_count(&logger, "tasks_moved_from_segment_count"),
	time_spent_moving_from_dequeue(&logger, "time_spent_moving_from_dequeue"),
	tasks_moved_from_dequeue_count(&logger, "tasks_moved_from_dequeue_count"),
	try_theft_count(&logger, "try_theft_count"),
	hit_count(&logger, "hit_count"),
	stolen_tasks_count(&logger, "stolen_tasks_count"),
	miss_notask_count(&logger, "miss_notask_count"),
	miss_target_locked_count(&logger, "miss_target_locked_count"),
	miss_free_for_copy(&logger, "miss_free_for_copy"),
	current_theft(&logger, "current_theft"),
	time_spent_stealing(&logger, "time_spent_stealing", logger_histogram_pow, logger_histogram_start),
	time_spent_returning_results(&logger, "time_spent_returning_results", logger_histogram_pow, logger_histogram_start),
	results_pushed_count(&logger, "results_pushed_count")
{
    TITUS_DLB_init_context();
    assert(check_status());
    DVS_context = new DVS_Context(new TITUS_DLB_Context(this));
}

// CLONE CONTEXT CONSTRUCTOR
TITUS_DLB_Context_impl::TITUS_DLB_Context_impl(const TITUS_DLB_Context_impl & arg):
    id(context_count++), DVS_context(nullptr), algorithm(arg.algorithm), shared_task_segment_size(arg.shared_task_segment_size),
    shared_result_segment_size(0),shared_tmp_result_segment_size(0),shared_scratch_segment_size(0),
    segment_task(-1), segment_result(-1), segment_tmp(-1),segment_scratch(-1),
    ptr_segment_result(nullptr), ptr_segment_tmp(nullptr), ptr_segment_scratch(nullptr),
    logger(),
    parallel_work_session_time(&logger, "parallel_work_session_time"),
	init_problem_time(&logger, "init_problem_time"),
	init_segment_time(&logger, "init_segment_time"),
    spawned_tasks_count(&logger, "spawned_tasks_count"),
    time_spent_working(&logger, "time_spent_working"),
    time_spent_solving_tasks(&logger, "time_spent_solving_tasks", logger_histogram_pow, logger_histogram_start),
    solved_tasks_count(&logger, "solved_tasks_count"),
	time_spent_moving_from_segment(&logger, "time_spent_moving_from_segment"),
	tasks_moved_from_segment_count(&logger, "tasks_moved_from_segment_count"),
	time_spent_moving_from_dequeue(&logger, "time_spent_moving_from_dequeue"),
	tasks_moved_from_dequeue_count(&logger, "tasks_moved_from_dequeue_count"),
	try_theft_count(&logger, "try_theft_count"),
	hit_count(&logger, "hit_count"),
	stolen_tasks_count(&logger, "stolen_tasks_count"),
	miss_notask_count(&logger, "miss_notask_count"),
	miss_target_locked_count(&logger, "miss_target_locked_count"),
	miss_free_for_copy(&logger, "miss_free_for_copy"),
	current_theft(&logger, "current_theft"),
	time_spent_stealing(&logger, "time_spent_stealing", logger_histogram_pow, logger_histogram_start),
	time_spent_returning_results(&logger, "time_spent_returning_results", logger_histogram_pow, logger_histogram_start),
	results_pushed_count(&logger, "results_pushed_count")
    
{
    TITUS_DLB_init_context();
    assert(check_status());
    DVS_context = new DVS_Context(new TITUS_DLB_Context(this));
}

TITUS_DLB_Context_impl::~TITUS_DLB_Context_impl() {
    //! TODO FREE MEMORY, DELETE STUFF
}

size_t TITUS_DLB_Context_impl::context_count = 0;


// *********************************************************************
// ****************** UTILS & CONNECTION MANAGEMENT ********************
// *********************************************************************


void TITUS_DLB_Context_impl::reset() { // resets all states and forgets about any task
    get_metadata_task()->reset();
    get_metadata_result()->reset(get_rank(),get_nb_ranks());
}

//! TODO : move to communication layer manager
void TITUS_DLB_Context_impl::register_to_rank(gaspi_rank_t arg) {
    //std::cout << "rank " << get_rank() << " : register_to_rank(" << arg << ")" << std::endl;
    
    
    assert(arg < get_nb_ranks());
    assert(segment_task != (decltype(segment_task))-1);
    assert(segment_result != (decltype(segment_task))-1);
    assert(segment_tmp != (decltype(segment_task))-1);
    
    if (registrations_status[arg] == true) return;
    registrations_status[arg] = true;
    
    gaspi_connect(arg,GASPI_BLOCK);
    
    //SUCCESS_OR_DIE( gaspi_segment_register( segment_task, arg, TITUS_DLB_GASPI_TIMEOUT));
    //SUCCESS_OR_DIE( gaspi_segment_register( segment_result,    arg, TITUS_DLB_GASPI_TIMEOUT));
    //SUCCESS_OR_DIE( gaspi_segment_register( segment_tmp, arg, TITUS_DLB_GASPI_TIMEOUT));
    
}

void TITUS_DLB_Context_impl::open_segment_to_thieves(gaspi_segment_id_t arg, bool force_connect) {
	// check that segment exists
    //~ if (gaspi_config_build_infrastructure != GASPI_TOPOLOGY_NONE
    //~ &&  gaspi_config_build_infrastructure != GASPI_TOPOLOGY_DYNAMIC)
		//~ return;
    gaspi_pointer_t tmp;
    assert(gaspi_segment_ptr(arg,&tmp) == GASPI_SUCCESS);

    
	if (force_connect && gaspi_config_build_infrastructure == GASPI_TOPOLOGY_DYNAMIC) {
	// issue dummy notifications to make sure connection is initialized
		for (size_t i=0;i<get_nb_ranks();i++) {
			if (registrations_status[i]==false) continue;
			
			gaspi_atomic_value_t dummy;
			TITUS_DBG << "TITUS_DLB_Context_impl::open_segment_to_thieves : initializing pair with " << (uint)i << std::endl;
			//SUCCESS_OR_DIE( gaspi_atomic_fetch_add( arg, offsetof(SegmentMetadata, connected_pairs), (gaspi_rank_t)i, 1, &dummy, GASPI_BLOCK));
			//~ TITUS_DBG << "gaspi_atomic_compare_swap(" << (uint)arg << ", " << (void *) offsetof(SegmentMetadata, connected_pairs) << ", " << (uint)(gaspi_rank_t)i << ", -1, 2, &dummy, GASPI_BLOCK)" << std::endl;
			//~ SUCCESS_OR_DIE( gaspi_atomic_compare_swap( arg, offsetof(SegmentMetadata, connected_pairs), (gaspi_rank_t)i, -1, 2, &dummy, GASPI_BLOCK));
			
			
			wait_if_queue_full(0,1);
			SUCCESS_OR_DIE(gaspi_notify(arg, (gaspi_rank_t)i, get_rank(), 1, 0, GASPI_BLOCK));
		}
		
		SUCCESS_OR_DIE( gaspi_wait(arg, GASPI_BLOCK) );
		
		for (size_t i=0;i<get_nb_ranks();i++) {
			if (registrations_status[i]==false) continue;
			gaspi_notification_id_t dummy;
			SUCCESS_OR_DIE( gaspi_notify_waitsome(arg, i, 1, &dummy, GASPI_BLOCK) );
			assert(dummy == i);
			gaspi_notification_t val;
			SUCCESS_OR_DIE( gaspi_notify_reset(arg, i, &val) );
			assert(val == 1);
		}
	}

	if (gaspi_config_build_infrastructure == GASPI_TOPOLOGY_NONE){
		for (size_t i=0;i<get_nb_ranks();i++) {
			if (registrations_status[i]==true){
				// if not connected SUCCESS_OR_DIE( gaspi_connect( arg, GASPI_BLOCK)); ?
				SUCCESS_OR_DIE( gaspi_segment_register( arg, i, GASPI_BLOCK));
			}
			//msg << " [" << i << "]";
		}
	}
    //TITUS_DBG << msg.str() << std::endl; //TITUS_DBG.flush();
}

void TITUS_DLB_Context_impl::print_registration_status(std::ostream & out) {
    assert(registrations_status != nullptr);
    
    out << "rank " << get_rank() << " :";
    for (size_t i=0;i<get_nb_ranks();i++) {
        if (registrations_status[i]==true){
			out << " [" << i << "]";
		}
    }
    out << std::endl;
}

bool TITUS_DLB_Context_impl::check_status() {
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

void TITUS_DLB_Context_impl::TITUS_DLB_init_segment(gaspi_segment_id_t segment_id, void ** local_mem_ptr, gaspi_size_t segment_size, bool use_simple_alloc){
    init_segment_time.start();

    if (use_simple_alloc)
     { SUCCESS_OR_DIE( gaspi_segment_alloc(    segment_id,   segment_size,   GASPI_BLOCK)); }
    else
     { SUCCESS_OR_DIE( gaspi_segment_create(   segment_id,   segment_size,   GASPI_GROUP_ALL, GASPI_BLOCK, GASPI_ALLOC_DEFAULT)); }
    SUCCESS_OR_DIE( gaspi_segment_ptr ( segment_id, local_mem_ptr ));
    ASSERT(*local_mem_ptr != nullptr);
    init_segment_time.stop();
}

static TITUS_DLB_sig_handler dummy_sig_handler;

void TITUS_DLB_Context_impl::TITUS_DLB_init_context() { //shared_task_segment_size
    //TITUS_DLB_sig_handler dummy;
    //ASSERT(dummy_sig_handler.is_init());
	//std::cout << "TITUS_DLB_Context_impl::TITUS_DLB_init_context : tid = " << pthread_self() << ", pid = " << getpid() << std::endl;
	//TITUS_DBG << "TITUS_PROC_FREQ = " << TITUS_PROC_FREQ << std::endl; TITUS_DBG.flush();
    logger.start_session();
    
    DEBUG_PRINT("=====================TITUS_DLB::init_library=====================\n");
    
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
    
    if(algorithm == TITUS_DLB::WORK_STEALING)          this->ptr_algorithm_function = TITUS_DLB_impl::work_stealing;
    else if(algorithm == TITUS_DLB::WORK_REQUESTING)   this->ptr_algorithm_function = TITUS_DLB_impl::work_requesting;
    
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
    
    
    TITUS_DLB_init_segment(segment_task, &ptr_segment_task, shared_task_segment_size, gaspi_config_build_infrastructure == GASPI_TOPOLOGY_NONE);
    new (ptr_segment_task) MetadataTask(this->shared_task_segment_size, segment_task);
    
    
    // init registration status
    registrations_status = new bool[get_nb_ranks()];//! TODO : move to communication layer manager
    for (gaspi_rank_t i = 0 ; i < get_nb_ranks() ; i++) registrations_status[i] = false;//! TODO : move to communication layer manager
    registrations_status[get_rank()] = true;
    
    // init termination detection topology
	if(get_rank() != 0)                  register_to_rank((get_rank() - 1)/2);
    if(2*get_rank()+1 < get_nb_ranks())  register_to_rank(2*get_rank()+1);
    if(2*get_rank()+2 < get_nb_ranks())  register_to_rank(2*get_rank()+2);

	SUCCESS_OR_DIE( TITUS_DLB::gaspi_barrier (GASPI_GROUP_ALL, GASPI_BLOCK) );
    if (get_rank() == 0) std::cout << " ---------------- CONTEXT INIT DONE ON ALL RANKS  ---------------- " << std::endl;
    
    logger.end_session();
    
}


void TITUS_DLB_Context_impl::set_problem(void *problem, int task_size, int nb_task, void *result, int result_size, void (*ptr_task_function)(void*, void*, void*), void * ptr_task_params)
{
    TITUS_DBG << "TITUS_DLB_Context_impl::set_problem(void *problem=" << problem << ", int task_size=" << task_size << ", int nb_task=" << nb_task << ", void *result=" << result << ", int result_size=" << result_size << ", void (*ptr_task_function)(void*, void*, void*)=" << ptr_task_function << ", void * ptr_task_params=" << ptr_task_params << ");" << std::endl; TITUS_DBG.flush();
	if (nb_task > 0){
		ASSERT(problem != nullptr);
		ASSERT(result != nullptr);
	}
	ASSERT(task_size > 0);
	ASSERT(result_size > 0);
	
    logger.end_session();

    int first = 1;
    if(this->shared_result_segment_size == 0)    first = 0;

    //reallocation si la taille allouer est trop petite
    if(task_size*nb_task > this->ptr_dequeue->problem_size_allocated)
    {
        TITUS_DBG << "WARNING : TITUS_DLB_Context_impl::set_problem : dequeue reallocation : " << this->ptr_dequeue->problem_size_allocated << " -> " << task_size*nb_task << " bytes" << std::endl; TITUS_DBG.flush();
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
    TITUS_DBG << "TITUS_DLB_Context_impl::set_problem : copying problem to dequeue : memcpy(" << this->ptr_dequeue->problem << ", " << problem << ", " << (size_t)task_size*nb_task << ");" << std::endl; TITUS_DBG.flush();
    memcpy(this->ptr_dequeue->problem, problem, (size_t)task_size*nb_task);
    TITUS_DBG << "copy completed" << std::endl; TITUS_DBG.flush();

    //! TODO :reallocation des segment si cela est necessaire
    //if (this->ptr_segment_result != nullptr)  SUCCESS_OR_DIE( gaspi_segment_delete(this->segment_result));
    //if (this->ptr_segment_tmp != nullptr)     SUCCESS_OR_DIE( gaspi_segment_delete(this->segment_tmp));

    //premiere allocation des segments
    if(first == 0){
        //calcul de la taille necessaire de segment
        this->shared_tmp_result_segment_size   = (gaspi_size_t)((this->shared_task_segment_size/task_size)*result_size + sizeof(MetadataTmp));         //calcul of this->shared_tmp_result_segment_size for save memory
        this->shared_result_segment_size       = (gaspi_size_t)((this->shared_task_segment_size/task_size)*result_size + sizeof(MetadataResult))*2;  //calcul of this->shared_result_segment_size for save memory
        this->shared_scratch_segment_size      = std::max(shared_result_segment_size,shared_tmp_result_segment_size);

        //! TODO : cleanup
        uint64_t start = rdtsc();
        TITUS_DLB_init_segment(segment_result,      &ptr_segment_result,  shared_result_segment_size,     gaspi_config_build_infrastructure == GASPI_TOPOLOGY_NONE);
        TITUS_DLB_init_segment(segment_tmp,         &ptr_segment_tmp,     shared_tmp_result_segment_size, gaspi_config_build_infrastructure == GASPI_TOPOLOGY_NONE);
		TITUS_DLB_init_segment(segment_scratch, 	  &ptr_segment_scratch, shared_scratch_segment_size,    gaspi_config_build_infrastructure == GASPI_TOPOLOGY_NONE);
        TITUS_DBG << "TITUS_DLB_Context_impl::set_problem : segments init done (" << (start - rdtsc()) / (TITUS_PROC_FREQ/1000) << " ms)" << std::endl; TITUS_DBG.flush();
    }

    //assert si le probleme change de taille
    //assert(this->shared_result_segment_size == ((this->shared_task_segment_size/task_size)*result_size + sizeof(MetadataResult)));
    //assert(this->shared_tmp_result_segment_size == (gaspi_size_t)((this->shared_task_segment_size/task_size)*result_size + sizeof(MetadataTmp)));
    
    init_problem_time.start();

    //----------------------------------------------------------------------------------------------------------
    // Initialization Dequeue
    //----------------------------------------------------------------------------------------------------------
    this->ptr_dequeue->owner_task_rank    = get_rank();
    this->ptr_dequeue->head               = 0;
    this->ptr_dequeue->tail               = (TITUS_DLB_int) nb_task;
    this->ptr_dequeue->base_task          = this->ptr_dequeue->problem;
    
    this->ptr_dequeue->end_task_id       = (TITUS_DLB_int) nb_task;                // task_id start to 1 (id = 0 is for init)
    this->ptr_dequeue->task_size          = task_size;                        //taille d'une tache en octets (la taille du tableau doit être un multiple de task_size)
    this->ptr_dequeue->result_size        = (TITUS_DLB_int) result_size;
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
    
    //verify if the size of segment_tmp is large enough
    assert( this->shared_tmp_result_segment_size >= (result_size + sizeof(MetadataTmp)) );
    assert( (TITUS_DLB_int)this->shared_task_segment_size >= (TITUS_DLB_int)(task_size + sizeof(MetadataTask)) );
    
    init_problem_time.stop();
    spawned_tasks_count += nb_task;
    
		
    SUCCESS_OR_DIE( TITUS_DLB::gaspi_barrier (GASPI_GROUP_ALL, GASPI_BLOCK) );
    if (get_rank() == 0) std::cout << " ---------------- PROBLEM SET ON ALL RANKS ----------------" << std::endl;
    
    //assert(check_status()); // aucun sens hors du mode random
    
    // register termination detection neighbors
    //! TODO : move global_termination_detected detection tree to infixed numerotation for optimized latencies
   std::stringstream outname("");
    outname << "registration_table" << std::setfill('0') << std::setw(4) << get_rank() << ".dat";
       std::ostream * out = new std::ofstream(outname.str().c_str(),std::ios_base::ate);
    print_registration_status(*out);
    
    
    // ensures connections are set up on the first execution of set_problem.
    // this serves as isolating connection setup time.
    //! TODO : disable : set second arg of open_segment_to_thieves to false
    TITUS_DBG << "gaspi_config_build_infrastructure = " << gaspi_topology_str(gaspi_config_build_infrastructure) << std::endl;
    open_segment_to_thieves(segment_result, first == 0);
    open_segment_to_thieves(segment_tmp, first == 0);
    open_segment_to_thieves(segment_task, first == 0);

    //assert(check_status());
        
//	sstring msg("");	
	TITUS_DBG << " set_problem done : "
		<< "shared_task_segment_size = " << shared_task_segment_size
		<< ", shared_result_segment_size = " << shared_result_segment_size
		<< ", shared_tmp_result_segment_size = " << shared_tmp_result_segment_size
		<< ", shared_scratch_segment_size = " << shared_scratch_segment_size
		<< std::endl; //TITUS_DBG.flush();

}


void TITUS_DLB_Context_impl::submit_results(void * start, size_t nb_results, TITUS_DLB_int first_task_id) {
    void * user_results_start = get_metadata_result()->usr_results;
    size_t result_size = get_metadata_result()->result_size;
    
    size_t user_results_offset = result_size * first_task_id;
    void * copy_dest = ADD_PTR(user_results_start, user_results_offset);
    size_t copy_size = nb_results * result_size;
    
   	//TITUS_DBG << "TITUS_DLB_Context_impl::submit_results : submitted " << nb_results << " results, starting with id " << first_task_id << std::endl;

    //TITUS_DBG << "TITUS_DLB_Context_impl::submit_results(start=" << start << ", nb_resut=" << nb_results << ", first_task_id=" << first_task_id << ")" << std::endl
	//			<< "> memcpy(dest=" << copy_dest << ", src=" << start << ", size=" << copy_size << ")" << std::endl; //TITUS_DBG.flush();
    memcpy(copy_dest, start , copy_size);
    get_metadata_result()->nb_result += nb_results;
	
    //assert that number of result is not greater than expected number of result
    if (get_metadata_result()->nb_result > get_metadata_result()->nb_usr_results){
		TITUS_DBG << "ASSERT IS GOING TO FAIL : " << get_metadata_result()->nb_result << " > " << get_metadata_result()->nb_usr_results << std::endl; //TITUS_DBG.flush();
	}
    ASSERT(get_metadata_result()->nb_result <= get_metadata_result()->nb_usr_results);
}

void TITUS_DLB_Context_impl::submit_results(BufferElt * arg){
    submit_results(ADD_PTR(arg , sizeof(BufferElt)), arg->nb_result, arg->first_task_id);
}


void TITUS_DLB_Context_impl::push_results_buffer(){
	get_metadata_result()->try_flush_buffer_elts_packed();
}



std::ostream & operator << (std::ostream & out , const TITUS_DLB_Context_impl & arg){
	arg.print_state(out);
	return out;
}

void TITUS_DLB_Context_impl::print_state(std::ostream & out)const{
	out << "TITUS_DLB_Context_impl : " << std::endl;
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

    out << "ptr_segment_task = " << ptr_segment_task << ", ";
    out << "ptr_segment_result = " << ptr_segment_result << ", ";
    out << "ptr_segment_tmp = " << ptr_segment_tmp << ", ";
    out << "ptr_segment_scratch = " << ptr_segment_scratch << ", " << std::endl;
	
}

#else // __TITUS_DLB_CONTEXT_HPP__

// include template implementation from dlb_context_impl.hpp

template<size_t size>
TITUS_DLB_Context_impl::Logger_Entry_Time_spent_stealing<size>::Logger_Entry_Time_spent_stealing(TITUS_Logger * arg, const std::string & name, double power, TITUS_Logger_raw_time_t start):
	TITUS_Logger_Entry_Timer_ns(nullptr, name),
	time_spent_stealing_hit(nullptr, name + "_hit"),
	hist_time_spent_stealing_hit(power, start),
	time_spent_stealing_miss(nullptr, name + "_miss"),
	hist_time_spent_stealing_miss(power, start)
	
{
	if (arg != nullptr) arg->add_entry(this);
	assert(power > 1);
}

template<size_t size>
TITUS_DLB_Context_impl::Logger_Entry_Time_spent_stealing<size>::Logger_Entry_Time_spent_stealing(const Logger_Entry_Time_spent_stealing & arg) // copy constructor
	: TITUS_Logger_Entry_Timer_ns(arg),
	time_spent_stealing_hit(arg.time_spent_stealing_hit),
	hist_time_spent_stealing_hit(arg.hist_time_spent_stealing_hit),
	time_spent_stealing_miss(arg.time_spent_stealing_miss),
	hist_time_spent_stealing_miss(arg.hist_time_spent_stealing_miss)
{
	
}

template<size_t size>
TITUS_DLB_Context_impl::Logger_Entry_Time_spent_stealing<size> * TITUS_DLB_Context_impl::Logger_Entry_Time_spent_stealing<size>::clone() const{
	return new TITUS_DLB_Context_impl::Logger_Entry_Time_spent_stealing<size>(*this);
}

template<size_t size>
void TITUS_DLB_Context_impl::Logger_Entry_Time_spent_stealing<size>::print(std::ostream & out){
	time_spent_stealing_hit.print(out);
	hist_time_spent_stealing_hit.print(out);
	out << std::endl;
	time_spent_stealing_miss.print(out);
	hist_time_spent_stealing_miss.print(out);
	out << std::endl;
}

template<size_t size>
void TITUS_DLB_Context_impl::Logger_Entry_Time_spent_stealing<size>::reset(){
	TITUS_Logger_Entry_Timer_ns::reset();
	time_spent_stealing_hit.reset();
	time_spent_stealing_miss.reset();
	hist_time_spent_stealing_hit.reset();
	hist_time_spent_stealing_miss.reset();
}

template<size_t size>
void TITUS_DLB_Context_impl::Logger_Entry_Time_spent_stealing<size>::aggregate(const TITUS_Logger_Entry_base & arg){ // add the value of the counter in argument to this
	TITUS_Logger_Entry_Timer_ns::aggregate(arg);
	const TITUS_DLB_Context_impl::Logger_Entry_Time_spent_stealing<size> & dynarg = (const TITUS_DLB_Context_impl::Logger_Entry_Time_spent_stealing<size> & )arg;
	time_spent_stealing_hit.aggregate(dynarg.time_spent_stealing_hit);
	hist_time_spent_stealing_hit.aggregate(dynarg.hist_time_spent_stealing_hit);
	time_spent_stealing_miss.aggregate(dynarg.time_spent_stealing_miss);
	hist_time_spent_stealing_miss.aggregate(dynarg.hist_time_spent_stealing_miss);
}

template<size_t size>
void TITUS_DLB_Context_impl::Logger_Entry_Time_spent_stealing<size>::start(){
	TITUS_Logger_Entry_Timer_ns::start();
}

template<size_t size>
void TITUS_DLB_Context_impl::Logger_Entry_Time_spent_stealing<size>::stop(bool theft_hit){
	TITUS_Logger_Entry_Timer_ns::stop();
	if (theft_hit){
		time_spent_stealing_hit += *this;
		hist_time_spent_stealing_hit.insert(TITUS_Logger::to_ns(*this));
	}
	else{
		time_spent_stealing_miss += *this;
		hist_time_spent_stealing_miss.insert(TITUS_Logger::to_ns(*this));
	}
	TITUS_Logger_Entry_Timer_ns::reset(); // resets current theft timer;
}

#endif // __TITUS_DLB_CONTEXT_HPP__
