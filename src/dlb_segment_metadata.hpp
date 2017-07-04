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


#include "TITUS_DLB_context.hpp"
#include <string>
#include <sstream>
#include <cstring>

#ifndef _TITUS_DLB_SEGMENT_METADATA_HPP_
#define _TITUS_DLB_SEGMENT_METADATA_HPP_

// *********************************************************************
// ************************* METADATA BASE *****************************
// *********************************************************************

struct SegmentMetadata{
    // *** CONSTRUCTOR ***
    SegmentMetadata(size_t size, gaspi_segment_id_t segment_id);
    
    // *** FIELDS ***

    gaspi_atomic_value_t state; // lock state
    size_t segment_size; // bytesize of the segment
    gaspi_segment_id_t segment_id;
    gaspi_atomic_value_t connected_pairs;

    // *** METHODS ***
    virtual std::string state_str()const {return state_str(state);}
    virtual std::string state_str(gaspi_atomic_value_t value)const {std::stringstream r; r << "state to str not defined (requires overload) (" << value << ")"; return r.str();}
    static gaspi_atomic_value_t compare_and_swap_status(gaspi_rank_t target, gaspi_segment_id_t target_segment, gaspi_atomic_value_t comp, gaspi_atomic_value_t new_val, bool nowait = false);

	SegmentMetadata * read_to_scratch(gaspi_rank_t target_rank, gaspi_offset_t offset_in_scratch = 0)const;
	static SegmentMetadata * read_to_scratch(gaspi_rank_t target_rank, gaspi_segment_id_t target_segment_id, gaspi_offset_t offset_in_scratch);
    
    const size_t & get_size() const {return segment_size;}
    
    virtual void print(std::ostream & out=std::cout, std::string line_prefix = std::string(""))const{
		out << "SegmentMetadata : " << std::endl;
		out << line_prefix << "state : " << this->state_str() << ", segment_size : " << segment_size << ", segment_id : " << (uint)segment_id << ", connected_pairs : " << (uint)connected_pairs << std::endl;
	}
};

std::ostream & operator<<(std::ostream & out, const SegmentMetadata & arg);


// *********************************************************************
// *************** buffered SEGMENT METADATA BASE ********************
// *********************************************************************

#include "TITUS_DLB_buffered_segment_metadata.hpp"

// *********************************************************************
// ****************** METADATA FOR TASK SEGMENT ************************
// *********************************************************************


struct MetadataTask : SegmentMetadata
{
    // *** CONSTRUCTOR ***
    MetadataTask(TITUS_DLB_int shared_task_segment_size, gaspi_segment_id_t segment_id);
    void reset();
    
    // *** FIELDS ***
    gaspi_atomic_value_t owner_task_rank;
    gaspi_atomic_value_t nb_task;
    gaspi_atomic_value_t end_task_id;
    gaspi_atomic_value_t task_size;
    gaspi_atomic_value_t result_size;
    size_t private_tasks_count;
    void (*ptr_task_function)(void*, void*, void*);

    // *** METHODS ***
    // default segment id is autoselect from context
	MetadataTask * read_to_scratch(gaspi_rank_t target_rank, gaspi_offset_t offset_in_scratch = 0)const;
	static MetadataTask * read_to_scratch(gaspi_rank_t target_rank, gaspi_segment_id_t target_segment_id, gaspi_offset_t offset_in_scratch);
	bool is_empty(){return nb_task == 0;}

    virtual std::string state_str()const;
    virtual std::string state_str(gaspi_atomic_value_t value)const;

	virtual void print(std::ostream & out=std::cout, std::string line_prefix = std::string(""))const{
		out << "MetadataTask : " << std::endl;
		out << line_prefix ; SegmentMetadata::print(out, line_prefix + "  ");
		out << line_prefix << "owner_task_rank : " << owner_task_rank << ", nb_task : " << nb_task << ", end_task_id : " << end_task_id << ", task_size : " << task_size << ", result_size : " << result_size << std::endl;
	}
};

std::ostream & operator<<(std::ostream & out, const MetadataTask & arg);

// *********************************************************************
// ***************** METADATA FOR RESULTS SEGMENT **********************
// *********************************************************************

struct TerminationDetectionData{
    gaspi_atomic_value_t left_child_subtree_termination;
    gaspi_atomic_value_t right_child_subtree_termination;
    gaspi_atomic_value_t global_termination_detected;
    
    virtual void print(std::ostream & out=std::cout, std::string line_prefix = std::string(""))const{
		out << "TerminationDetectionData : " << std::endl;
		out << line_prefix << "left_child_subtree_termination : " << left_child_subtree_termination << ", right_child_subtree_termination : " << right_child_subtree_termination << ", global_termination_detected : " << global_termination_detected << std::endl;
	}
};

std::ostream & operator<<(std::ostream & out, const TerminationDetectionData & arg);

struct MetadataResult : BufferedSegmentMetadata
{
    // *** CONSTRUCTOR ***
    MetadataResult(size_t segment_size,  gaspi_segment_id_t segment_id, TITUS_DLB_int rank, TITUS_DLB_int nb_procs, TITUS_DLB_int nb_usr_results, TITUS_DLB_int result_size, void *usr_results);
    void reset(TITUS_DLB_int ratank, TITUS_DLB_int nb_procs);
    
    // *** FIELDS ***
    gaspi_atomic_value_t nb_result; // completion advancement
    TerminationDetectionData termination_status;
    gaspi_atomic_value_t nb_usr_results; // quantity of awaited results
    size_t result_size; // the size in bytes of the results of a task
    void *usr_results; // location of user allocated memory where results must be returned

    // *** METHODS ***
    // default segment id is autoselect from context 
	MetadataResult * read_to_scratch(gaspi_rank_t target_rank, gaspi_offset_t offset_in_scratch = 0)const;
	static MetadataResult * read_to_scratch(gaspi_rank_t target_rank, gaspi_segment_id_t target_segment_id, gaspi_offset_t offset_in_scratch);
	MetadataResult * copy_local_segment_to_scratch();

	float mem_occupancy()const ;
	float elt_count_occupancy()const ;
	float occupancy()const ;
	
    virtual std::string state_str()const;
    virtual std::string state_str(gaspi_atomic_value_t value)const;

	virtual void print(std::ostream & out=std::cout, std::string line_prefix = std::string(""))const{
		out << "MetadataResult : " << std::endl;
		out << line_prefix;	BufferedSegmentMetadata::print(out, line_prefix + "  ");
		out << line_prefix << "nb_result : " << nb_result << ", nb_usr_results : " << nb_usr_results << ", result_size : " << result_size << ", usr_results : " << usr_results << std::endl;
		out << line_prefix << "termination_status : " << termination_status;
	}
};

std::ostream & operator<<(std::ostream & out, const MetadataResult & arg);

// *********************************************************************
// *************** METADATA FOR TMP RESULTS SEGMENT ********************
// *********************************************************************

struct MetadataTmp : SegmentMetadata
{
    // *** CONSTRUCTOR ***
    MetadataTmp(size_t size, gaspi_segment_id_t segment_id, size_t result_size);
	
    // *** FIELDS ***
    gaspi_atomic_value_t owner_result_rank;
    gaspi_atomic_value_t last_result_id;
    gaspi_atomic_value_t nb_result; // qty of results to be returned
    gaspi_atomic_value_t nb_result_max;
    size_t result_size;
    void *ptr_next_result_to_store;
    BufferElt dummy; // placeholder : ensures there is always enough room to build a buffer elt before pushing tmp results
    
    void push_tmp_results();
	void reset();
    // *** METHODS ***
    // default segment id is autoselect from context 
	MetadataTmp * read_to_scratch(gaspi_rank_t target_rank, gaspi_offset_t offset_in_scratch = 0)const;
	static MetadataTmp * read_to_scratch(gaspi_rank_t target_rank, gaspi_segment_id_t target_segment_id, gaspi_offset_t offset_in_scratch);
	bool is_empty(){return nb_result == 0;}
	
	virtual std::string state_str()const;
	virtual std::string state_str(gaspi_atomic_value_t value)const;

	virtual void print(std::ostream & out=std::cout, std::string line_prefix = std::string(""))const{
		out << "MetadataTmp : " << std::endl;
		out << line_prefix;	SegmentMetadata::print(out, line_prefix + "  ");
		out << line_prefix << "owner_result_rank : " << owner_result_rank << ", last_result_id : " << last_result_id << ", nb_result : " << nb_result << ", nb_result_max : " << nb_result_max << ", result_size : " << result_size << ", ptr_next_result_to_store : " << ptr_next_result_to_store << std::endl;
	}
};
std::ostream & operator<<(std::ostream & out, const MetadataTmp & arg);

#endif //_TITUS_DLB_SEGMENT_METADATA_HPP_
