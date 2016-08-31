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


#ifndef _DLB_SEGMENT_METADATA_HPP_
#include "dlb_segment_metadata.hpp"
#endif

#ifndef _DLB_BUFFERED_SEGMENT_METADATA_HPP_
#define _DLB_BUFFERED_SEGMENT_METADATA_HPP_
#include <deque>
#include <vector>
#include <map>


// *********************************************************************
// *************** buffered SEGMENT METADATA BASE ********************
// *********************************************************************

// BufferElt contains information about the buffer entry
// It is located at the beginning of the entry and is followed in memory by the data attached to this buffer element.
// Copying sizeof(BufferElt) + data_size adequetely copies the buffer entry as a whole.
struct BufferElt{
    // *** CONSTRUCTOR ***
    BufferElt(dlb_int first_task_id, dlb_int nb_result, dlb_int task_size, gaspi_rank_t owner);
    
    // *** FIELDS ***
    dlb_int first_task_id;
    size_t nb_result;
    size_t data_size;
    gaspi_rank_t owner;
    dlb_int remote_buffer_alloc_attemps;
    // *** METHODS ***
    size_t buffer_entry_size()const;
    BufferElt* next()const;
//    void try_push_buffer_elt_to(gaspi_pointer_t buffer_elt, gaspi_segment_id_t src_seg_id, gaspi_rank_t dst_rank, gaspi_segment_id_t dst_seg_id);
	void print(std::ostream & out = std::cout) const;
	void print(std::ostream && out) const;
	
	bool is_valid()const;

};

std::ostream & operator<<(std::ostream & out, const BufferElt & arg);

struct write_list_args_t{
	size_t elt_count;
	gaspi_rank_t target;
	gaspi_segment_id_t *segment_id_local;
	gaspi_offset_t *offset_local;
	gaspi_segment_id_t *segment_id_remote;
	gaspi_offset_t *offset_remote;
	gaspi_size_t *size;
	
	write_list_args_t(size_t num, gaspi_rank_t target):elt_count(num), target(target){
		segment_id_local = new gaspi_segment_id_t[elt_count];
		offset_local = new gaspi_offset_t[elt_count];
		segment_id_remote = new gaspi_segment_id_t[elt_count];
		offset_remote = new gaspi_offset_t[elt_count];
		size = new gaspi_size_t[elt_count];
	}
	write_list_args_t(write_list_args_t const & arg):elt_count(arg.elt_count), target(arg.target){
		segment_id_local = new gaspi_segment_id_t[elt_count]; memcpy(segment_id_local, arg.segment_id_local,sizeof(gaspi_segment_id_t)*elt_count);
		offset_local = new gaspi_offset_t[elt_count]; memcpy(offset_local, arg.offset_local,sizeof(gaspi_offset_t)*elt_count);
		segment_id_remote = new gaspi_segment_id_t[elt_count]; memcpy(segment_id_remote, arg.segment_id_remote,sizeof(gaspi_segment_id_t)*elt_count);
		offset_remote = new gaspi_offset_t[elt_count]; memcpy(offset_remote, arg.offset_remote,sizeof(gaspi_offset_t)*elt_count);
		size = new gaspi_size_t[elt_count]; memcpy(size, arg.size,sizeof(gaspi_size_t)*elt_count);
	}
	write_list_args_t(write_list_args_t && arg):elt_count(arg.elt_count), target(arg.target){
		arg.elt_count = 0;
		segment_id_local = arg.segment_id_local; arg.segment_id_local = nullptr;
		offset_local = arg.offset_local; arg.offset_local = nullptr;
		segment_id_remote = arg.segment_id_remote; arg.segment_id_remote = nullptr;
		offset_remote = arg.offset_remote; arg.offset_remote = nullptr;
		size = arg.size; arg.size = nullptr;
	}
	~write_list_args_t(){
		if (segment_id_local != nullptr) delete segment_id_local;
		if (offset_local != nullptr) delete offset_local;
		if (segment_id_remote != nullptr) delete segment_id_remote;
		if (offset_remote != nullptr) delete offset_remote;
		if (size != nullptr) delete size;
	}

};

class BufferEltIterator : public std::iterator<std::forward_iterator_tag, BufferElt>
{
  BufferElt* p;
public:
  BufferEltIterator(BufferElt* x) 				 :p(x) {}
  BufferEltIterator(const BufferEltIterator& mit):p(mit.p) {}
  BufferEltIterator&operator++()   {p = (BufferElt*)ADD_PTR(p,p->buffer_entry_size()); return *this;}
  BufferEltIterator operator++(int){BufferEltIterator tmp(*this); operator++(); return tmp;}
  bool operator==(const BufferEltIterator& rhs)const {return p==rhs.p;}
  bool operator!=(const BufferEltIterator& rhs)const {return p!=rhs.p;}
  bool operator< (const BufferEltIterator& arg)const {return p< arg.p;}
  bool operator<=(const BufferEltIterator& arg)const {return p<=arg.p;}
  bool operator> (const BufferEltIterator& arg)const {return p> arg.p;}
  bool operator>=(const BufferEltIterator& arg)const {return p>=arg.p;}
  BufferElt& operator* () {return*p;}
  BufferElt* operator->() {return p;}
};


struct BufferedSegmentMetadata : SegmentMetadata{
    // *** CONSTRUCTOR ***
    BufferedSegmentMetadata(size_t segment_size, gaspi_segment_id_t segment_id, size_t size_of_object = sizeof(BufferedSegmentMetadata));

    // *** FIELDS ***
    // offset relative to `this`, assuming segment metadata start at 0 in segment (and it should), this is the offset in the segment.
    gaspi_offset_t buffer_tail; // offset of element 0
    gaspi_offset_t buffer_head; // offset of past-last element

    uint segment_wipe_id; //! TODO : cleanup or move to logger

    BufferEltIterator begin() const;
    BufferEltIterator end() const;
    gaspi_atomic_value_t buffer_elt_count;
    
    bool is_empty () const{return buffer_tail == buffer_head && local_transiting_results.size() == 0; }
    void purge () {
		//memset(ADD_PTR(this,buffer_tail),0xFF,buffer_head-buffer_tail);//TODO : DEBUG mode
		buffer_head = buffer_tail; buffer_elt_count = 0;
	    pending_write_lists->clear();
	    segment_wipe_id ++;
	}
    
    // *** METHODS ***
    virtual std::string state_str()const;
    virtual std::string state_str(gaspi_atomic_value_t value)const;
	// insert data from local segment
	void insert_buffer_elt(BufferElt * arg);
	// forward data
	
	gaspi_notification_t wait_notification_for_elt(int elt_index)const;
	void wait_notifications_for_all_elts()const;
	
	// old
    void try_flush_buffer_elts(); // defaults to self segment id (should not be necessary, remove arg)
    std::pair<gaspi_offset_t,dlb_int> try_remote_buffer_alloc(size_t bytes, gaspi_segment_id_t seg_id, gaspi_rank_t target)const;
    bool try_push_buffer_elt_to(gaspi_pointer_t buffer_elt, gaspi_rank_t dst_rank, gaspi_segment_id_t dst_seg_id)const;
	
	// newer better faster stronger
	typedef std::multimap<gaspi_rank_t,std::pair<gaspi_segment_id_t,BufferElt *>> buffer_elts_by_dest;
	
	struct selected_elts_to_push{
		selected_elts_to_push():elt_count(0),to_reserve(0),selected(){}
		selected_elts_to_push(size_t size):elt_count(0),to_reserve(0),selected(size){}
		selected_elts_to_push(selected_elts_to_push && arg):elt_count(arg.elt_count),to_reserve(arg.to_reserve),selected(std::move(arg.selected)){arg.elt_count = 0; arg.to_reserve = 0;}
		selected_elts_to_push(selected_elts_to_push const& arg):elt_count(arg.elt_count),to_reserve(arg.to_reserve),selected(arg.selected){}
		~selected_elts_to_push(){}
		size_t size(){return selected.size();}
		gaspi_number_t elt_count; // number of true in selected
		size_t to_reserve; // bytes to reserve
		std::vector<bool> selected;
	};

    void try_flush_buffer_elts_packed();
	template<class InputIterator>
	static selected_elts_to_push select_elts_to_push(InputIterator begin, InputIterator end, size_t available_size, size_t max_elts_count);
	template <class InputIterator> // some class of iterator "pointing" to std::pair<segment_id_t, BufferElt*>
	selected_elts_to_push try_push_vector_elt_to(InputIterator begin, InputIterator end, gaspi_rank_t target, gaspi_segment_id_t dst_seg_id);
	std::vector<write_list_args_t> * pending_write_lists;
	
	// Storage for buffer elements on non-segment memory
    buffer_elts_by_dest local_transiting_results;
    void print_local_transiting_results(std::ostream & out, std::string line_prefix = std::string(""))const;
    void try_push_local_transiting_results();
	
	BufferedSegmentMetadata * read_to_scratch(gaspi_rank_t target_rank, gaspi_offset_t offset_in_scratch = 0)const;
	static BufferedSegmentMetadata * read_to_scratch(gaspi_rank_t target_rank, gaspi_segment_id_t target_segment_id, gaspi_offset_t offset_in_scratch = 0);
	
	virtual void print(std::ostream & out=std::cout, std::string line_prefix = std::string(""))const{
		out << "BufferedSegmentMetadata : " << std::endl;
		out << line_prefix ; SegmentMetadata::print(out, line_prefix + "  ");
		out << line_prefix << "buffer_tail=" << buffer_tail << ", buffer_head=" << buffer_head << ", buffer_elt_count=" << buffer_elt_count;
		
		gaspi_notification_id_t i=0;
		out << line_prefix << std::endl;
		for (auto it = begin(); it != end() && i < buffer_elt_count; it = it->next()) {
			out << line_prefix << "  [" << i << "] " << "@" << DIFF_PTR(&(*it),&(*begin())) << ", " << *it;
			if (it->is_valid() == false) {
				out << "(invalid buffer element. abort iterating (" << buffer_elt_count-i-1 << "more unreachable elements in buffer)" << std::endl;
				break;
			}
			out << std::endl;
			i++;
		}
		print_local_transiting_results(out, line_prefix + "  ");

	}
	
	friend std::ostream & operator << (std::ostream & , selected_elts_to_push const &);
};

//typedef std::vector<std::pair<gaspi_segment_id_t, BufferElt *>> elts_collection;
BufferedSegmentMetadata::buffer_elts_by_dest & operator <<(BufferedSegmentMetadata::buffer_elts_by_dest & arg, BufferedSegmentMetadata::buffer_elts_by_dest::value_type const & val);
BufferedSegmentMetadata::buffer_elts_by_dest & operator <<(BufferedSegmentMetadata::buffer_elts_by_dest & arg, BufferedSegmentMetadata::buffer_elts_by_dest::mapped_type const & val);
template<typename iterator_type>
BufferedSegmentMetadata::buffer_elts_by_dest & operator << (BufferedSegmentMetadata::buffer_elts_by_dest & arg, std::pair<iterator_type, iterator_type> const & val);
std::ostream & operator<<(std::ostream & out, const BufferedSegmentMetadata & arg);
std::ostream & operator << (std::ostream & out , BufferedSegmentMetadata::selected_elts_to_push const & arg);


#endif //_DLB_BUFFERED_SEGMENT_METADATA_HPP_
