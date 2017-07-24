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


#ifndef __TITUS_DLB_CONTEXT_HPP__
#define __TITUS_DLB_CONTEXT_HPP__

#include <cstdint>
#include <GASPI.h>
#include <TITUS_DLB_gaspi_tools.hpp>
#include <TITUS_DLB.hpp>
#include "TITUS_DLB_internal.hpp"
#include "TITUS_DLB_segment_metadata.hpp"

#ifdef DEBUG
#define DEBUG_PRINT(...) do{ printf( __VA_ARGS__ ); } while( false )
#else
#define DEBUG_PRINT(...) do{ } while ( false )
#endif

#define FAILED              (0) //! TODO : public enum in TITUS_DLB ?
#define SUCCESS             (1)
#define FINISHED            (2)
#define THIEF        (3)
#define WORKER     (4)

// Special values for segment lock status variables
// any other value represents that the rank with that id has taken lock on the segment
// task segment lock values
//! TODO : move to enum classes in segment metadata.
#ifndef UINT64_MAX
#define UINT64_MAX ((uint)-1)
#endif
#define TASK_AVAILABLE          ((gaspi_atomic_value_t)(UINT64_MAX)) // some tasks can be stolen from the task segment
#define FREE_FOR_COPY           ((gaspi_atomic_value_t)(UINT64_MAX-1)) // there is no more work available fo theft on the task segment
#define NO_TASK                 ((gaspi_atomic_value_t)(UINT64_MAX-2)) // there is no more work available for theft on the task segment and there is no task avilable for filling it.
#define COPY_IN_PROGRESS        ((gaspi_atomic_value_t)(UINT64_MAX-3)) // segment is being filled / purged by owner
// result segment lock values
#define SEGMENT_AVAILABLE       ((gaspi_atomic_value_t)(UINT64_MAX-1)) // some buffer elt can be pushed to the buffered results segment

#define PUSH_REQUEST_MIN_OCCUPANCY (0.8)

#define NB_TASK_MIN         (2) // minimum local tasks required for sharing work

#define MAX_BUFFER_ELTS 						(60000) // sets the limit to the maximum element id in a buffered segment and thus the number of notifications id it reserves.
#define NB_BUFFERED_SEGMENTS 					(1)   // number of buffered segments in TITUS_DLB
#define MIN_BUFFERISED_SEGMENT_NOTIFICATION_ID 	(0)
#define MAX_BUFFERISED_SEGMENT_NOTIFICATION_ID 	(MIN_BUFFERISED_SEGMENT_NOTIFICATION_ID + MAX_BUFFER_ELTS * TITUS_DLB_Context::get_nb_contexts() * NB_BUFFERED_SEGMENTS - 1)

#define NOTIFICATION_ID_PER_BARRIER_PER_GROUP	(3)
#define MIN_BARRIER_NOTIFICATION_ID				(MAX_BUFFERISED_SEGMENT_NOTIFICATION_ID + 1)


struct Dequeue;

//--------------------------------------------------------------
//Define Gaspi structure
//--------------------------------------------------------------
class TITUS_DLB_Context_impl
{
public :
    TITUS_DLB_Context_impl(); //default : from default config file (if env var is set) or default config
    TITUS_DLB_Context_impl(int shared_task_segment_size, int algorithm);
    TITUS_DLB_Context_impl(const char * config_filename); //from config file
    TITUS_DLB_Context_impl(const TITUS_DLB_Context_impl &arg); // clone context
    ~TITUS_DLB_Context_impl();
    
    void set_problem(void *problem, int task_size, int nb_task, void *result, int result_size, void (*ptr_task_function)(void*, void*, void*), void * params);    
//    void set_shared_task_segment_size(size_t arg);
//    size_t get_shared_task_segment_size();
//    void set_algorithm(TITUS_DLB::Algorithm arg);
//    TITUS_DLB::Algorithm get_algorithm();
	
    TITUS_Logger * get_logger() {return &logger;}
    DVS_Context * get_DVS_context() {return DVS_context;}
    void set_DVS_context(DVS_Context * arg) {DVS_context = arg; if (TITUS_DLB_impl::get_context() == this)DVS::set_context(arg);}
    
	gaspi_rank_t get_rank()const { gaspi_rank_t r; SUCCESS_OR_DIE( gaspi_proc_rank(&r) ) ; return r;}
	gaspi_rank_t get_nb_ranks()const { gaspi_rank_t r; SUCCESS_OR_DIE( gaspi_proc_num(&r) ) ; return r; }
	
	void print_state(std::ostream & out)const;

    // DEBUG PRINTERS
#ifdef DEBUG
    void display_state();
    void display_metadatatask();
    void display_metadataresult();
    void display_metadatatmp();
    void display_dequeue();
    void display_Context();
#endif
    
    size_t get_id()const {return id;}
    static size_t get_context_count() {return context_count;}
    
    void register_to_rank(gaspi_rank_t arg); //! TODO : move to communication layer manager
    
    const MetadataTask            *get_metadata_task()   const { return ((const MetadataTask *)ptr_segment_task);     }
    const MetadataResult          *get_metadata_result() const { return ((const MetadataResult *)ptr_segment_result); }
    const MetadataTmp             *get_metadata_tmp()    const { return ((const MetadataTmp *)ptr_segment_tmp);       }

private :
    void TITUS_DLB_init_segment(gaspi_segment_id_t segment_id, void ** local_mem_ptr, gaspi_size_t segment_size, bool use_simple_alloc);
    void TITUS_DLB_init_context();
    
    void push_results_buffer();
    void submit_results(void * start, size_t nb_results, TITUS_DLB_int start_id);
    void submit_results(BufferElt * arg);
    void reset(); // resets all states and forgets about any task
    
    bool * registrations_status; //! TODO : move to communication layer manager
    void open_segment_to_thieves(gaspi_segment_id_t arg, bool force_connect = false);
    void print_registration_status(std::ostream & out);
    bool check_status();
    
    
// ------------------------------------------------------
// ---------------- TITUS_DLB CONTEXT DATA --------------------
// ------------------------------------------------------
    size_t id;
    static size_t context_count;
    
    gaspi_topology_t gaspi_config_build_infrastructure;
    
    #define logger_histogram_start (TITUS_Logger::to_ns(1000)) //1µs
    static const size_t logger_histogram_size = 8;
    #define logger_histogram_pow ((double)10.0) //1µs
    
    TITUS_Logger logger;
public : // can be accessed by others classes in the module
    TITUS_Logger_Entry_Timer_ns parallel_work_session_time;
	TITUS_Logger_Entry_Timer_ns init_problem_time;
	TITUS_Logger_Entry_Timer_ns init_segment_time;
    TITUS_Logger_Entry_counter<uint> spawned_tasks_count;
    TITUS_Logger_Entry_Timer_ns time_spent_working;
    TITUS_Logger_Entry_Timer_ns_pow_histogram<logger_histogram_size> time_spent_solving_tasks;
    TITUS_Logger_Entry_counter<uint> solved_tasks_count;

	TITUS_Logger_Entry_Timer_ns time_spent_moving_from_segment;
	TITUS_Logger_Entry_counter<uint> tasks_moved_from_segment_count;
	TITUS_Logger_Entry_Timer_ns time_spent_moving_from_dequeue;
	TITUS_Logger_Entry_counter<uint> tasks_moved_from_dequeue_count;

	template<size_t size>
	class Logger_Entry_Time_spent_stealing:protected TITUS_Logger_Entry_Timer_ns{
		// the inherited TITUS_Logger_Entry_Timer_ns serves to count the current theft attempt.
		TITUS_Logger_Entry_counter<uint64_t> time_spent_stealing_hit; // times spent in hitting thefts in ns
		TITUS_Logger_pow_histogram<uint64_t, size> hist_time_spent_stealing_hit;
		TITUS_Logger_Entry_counter<uint64_t> time_spent_stealing_miss; // times spent in missing thefts in ns
		TITUS_Logger_pow_histogram<uint64_t, size> hist_time_spent_stealing_miss;
	public :	
		Logger_Entry_Time_spent_stealing(TITUS_Logger * arg, const std::string & name, double power, TITUS_Logger_raw_time_t start = 0);
		Logger_Entry_Time_spent_stealing(const Logger_Entry_Time_spent_stealing & arg); // copy constructor
		virtual Logger_Entry_Time_spent_stealing * clone() const; // copy constructor
		virtual void print(std::ostream & out = std::cout);
		virtual void reset();
		virtual void aggregate(const TITUS_Logger_Entry_base & arg); // add the value of the counter in argument to this
		
		void start();
		void stop(bool theft_hit);// specifies the outcome of the theft attempt.
	};
	
	TITUS_Logger_Entry_counter<uint> try_theft_count;
	TITUS_Logger_Entry_counter<uint> hit_count;
	TITUS_Logger_Entry_counter<uint> stolen_tasks_count;
	TITUS_Logger_Entry_counter<uint> miss_notask_count;
	TITUS_Logger_Entry_counter<uint> miss_target_locked_count;
	TITUS_Logger_Entry_counter<uint> miss_free_for_copy;
	TITUS_Logger_Entry_Timer_ns current_theft;
	Logger_Entry_Time_spent_stealing<logger_histogram_size> time_spent_stealing;
	
	TITUS_Logger_Entry_Timer_ns_pow_histogram<logger_histogram_size> time_spent_returning_results;
	TITUS_Logger_Entry_counter<uint> results_pushed_count;

private :
	
    DVS_Context * DVS_context;
    
    Dequeue *ptr_dequeue;
	
    TITUS_DLB_int                 algorithm;
    TITUS_DLB_int (*ptr_algorithm_function)(gaspi_rank_t);
    
    gaspi_number_t gaspi_topo;
    
    gaspi_size_t            shared_task_segment_size;
    gaspi_size_t            shared_result_segment_size;
    gaspi_size_t            shared_tmp_result_segment_size;
    gaspi_size_t            shared_scratch_segment_size;
    
    gaspi_segment_id_t      segment_task; // task sharing segment
    gaspi_segment_id_t      segment_result; // results returning segment used for pushing results back to task owners and routing to them
    gaspi_segment_id_t      segment_tmp; // temporary results segments where results are pushed as they are calculated by local process
    gaspi_segment_id_t      segment_scratch; // scratch segment used for pulling distant segment metadata.

    gaspi_queue_id_t        queue_task;
    gaspi_queue_id_t        queue_result;
    gaspi_queue_id_t        queue_scratch;

    void                    *ptr_segment_task;
    void                    *ptr_segment_result;
    void                    *ptr_segment_tmp;
    void                    *ptr_segment_scratch;

    MetadataTask            *get_metadata_task()   { return ((MetadataTask *)ptr_segment_task);     }
    MetadataResult          *get_metadata_result() { return ((MetadataResult *)ptr_segment_result); }
    MetadataTmp             *get_metadata_tmp()    { return ((MetadataTmp *)ptr_segment_tmp);       }

    friend class TITUS_DLB_impl;
    friend class DVS_impl;
    friend struct Dequeue;
    friend struct SegmentMetadata;
    friend struct BufferedSegmentMetadata;
    friend struct MetadataTask;
    friend struct MetadataResult;
    friend struct MetadataTmp;
};

#include "TITUS_DLB_context.cpp"

std::ostream & operator << (std::ostream & out , const TITUS_DLB_Context_impl & arg);
//--------------------------------------------------------------
//Local Dequeue
//--------------------------------------------------------------
struct Dequeue
{
    TITUS_DLB_int head;
    TITUS_DLB_int tail;
    void *base_task;
    
    TITUS_DLB_int owner_task_rank;
    TITUS_DLB_int end_task_id;
    TITUS_DLB_int task_size;      //BYTES, suppose all tasks are same size //! TODO: just make it a size_t then :D

    TITUS_DLB_int result_size;

    void *problem;
    TITUS_DLB_int problem_size_allocated;
    void (*ptr_task_function)(void*, void*, void*);
    void *ptr_task_params;
    
	void print(std::ostream & out=std::cout, std::string line_prefix = std::string(""))const{
		out << "local task Dequeue :" << std::endl;
		out << line_prefix << "head : " << head << " tail : " << tail << " base_task : " << base_task << std::endl;
		out << line_prefix << "owner_task_rank : " << owner_task_rank << " end_task_id : " << end_task_id << " task_size : " << task_size << " result_size : " << result_size << std::endl;
	}
};
std::ostream & operator << (std::ostream & out , const Dequeue & arg);

#endif

