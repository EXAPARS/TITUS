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

#ifndef __DLB_INTERNAL_H__
#define __DLB_INTERNAL_H__

#include <GASPI.h>
#include <DLB.hpp>
#include <dlb_gaspi_tools.hpp>
#include <map>
#include <neighboring_generator.hpp>
//#include "dlb_context.hpp"
//#include "dlb_logger.hpp"

class DLB_impl
{
public :
    static void parallel_work(uint64_t timeout_ms = 0);
    
    static void set_context(DLB_Context_impl * arg);
    static DLB_Context_impl * get_context();

    // from WORK_REQUESTING_H
    static dlb_int  work_requesting(gaspi_rank_t target_rank);

    // from WORK_STEALING_H
    static dlb_int  work_stealing(gaspi_rank_t target_rank);
	
	// a gaspi standard compliant notifications-based barrier implemented using DLB-managed neighboring
	// uses a b-tree based up&down scheme
	// this collective call is meant to replace gaspi_barrier when gaspi was initialized with build_infrastructure set to false.
	static gaspi_return_t barrier(gaspi_group_t group, gaspi_timeout_t timeout = GASPI_BLOCK);

	static gaspi_return_t gaspi_proc_init_impl(gaspi_timeout_t timeout = GASPI_BLOCK);

private :
    static DLB_Context_impl * context;
    
    // from LOAD_BALANCING_H
    static dlb_int  load_balancing();

    // from RETURN_RESULT_H
    static void return_result();
    static void return_results_buffered();
    static void return_result_simple();

    // from TERMINATION_DETECTION_H
    static dlb_int  termination_detection();

    // from WORK_ON_MY_DEQUEUE_H
    static void write_task_to_remote_segment();
    static void move_task_from_my_dequeue_to_my_segment();
    static void move_task_from_my_segment_to_my_dequeue();
    static void do_next_task();
    static void detection_request();
    static void active_wait_state();
    static void work_on_my_dequeue();

#ifdef DEBUG
    void static display_state();
    void static display_metadatatask();
    void static display_metadataresult();
    void static display_metadatatmp();
    void static display_dequeue();
    void static display_Context();
#endif


	struct barrier_status{
		bool handshake_ok;
		static gaspi_queue_id_t queue_barrier;
		static gaspi_segment_id_t segment_barrier;
		barrier_status(gaspi_group_t group);

		gaspi_group_t group;
		bool barrier_is_in_progress;
		
		gaspi_rank_t rank;
		int me_in_group;
		
		gaspi_number_t group_size;
		gaspi_rank_t * ranks_in_group;
		
		infixed_b_tree_neighbors neighbors;
		bool left_child_ok, right_child_ok, parent_ok;
		
		gaspi_return_t barrier(gaspi_timeout_t timeout);
		
	private :
		void first_barrier_handshake_protocol();
		void init_barrier();
	};


	static bool barrier_segment_is_init;
	static gaspi_segment_id_t segment_barrier;
	static gaspi_pointer_t ptr_segment_barrier;
	static size_t shared_barrier_segment_size;

	static gaspi_queue_id_t queue_barrier;
	
	
	// allocates a segment large enough to contain gaspi_group_max() times the barrier_status_segment_data structure
	// note : as of GPI-2 "#define gaspi_group_max()  (32)"
	// connects and register the new segment to neighbors as given by GASPI_GROUP_ALL infixed btree rank numerotation 
	// ensures all barrier segments are created by performing a notification-based barrier
	// should be called right after gaspi_proc_init in order not to interfere with other notifications-based functionnality
	static void init_barrier_segment();

	static barrier_status * barrier_statuses;
	
	friend struct barrier_status;
};
#endif //__dlb_intERNAL_H__
