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


#ifndef __TITUS_DLB_LOGGER_H__
#define __TITUS_DLB_LOGGER_H__

#include <TITUS_DLB.hpp>
#include <list>
#include <sys/time.h>
#include <string>
#include <ostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstdint>

static uint64_t rdtsc();

#ifndef GPI2_H
typedef unsigned short gaspi_rank_t;
#endif
#ifndef _GLIBCXX_CSTDINT
typedef long unsigned int uint64_t;
#endif

#define TITUS_DLB_LOGGER_THEFT_BUFFER_SIZE (sizeof(theft_info) * 100)

#define TITUS_DLB_LOGGER_BUFFERED_THEFT_DUMP



struct theft_info{
    theft_info() {}
    theft_info(size_t theft_id, size_t context_id, size_t session_id, uint64_t start_date, uint64_t end_date, gaspi_rank_t thief, gaspi_rank_t victim, gaspi_rank_t tasks_owner, TITUS_DLB_int theft_status, size_t nb_task_theft):
        theft_id(theft_id),
        context_id(context_id),
        session_id(session_id),
        start_date(start_date),
        end_date(end_date),
        thief(thief),
        victim(victim),
        tasks_owner(tasks_owner),
        theft_status(theft_status),
        nb_task_theft(nb_task_theft)
        {}
        
    theft_info(const theft_info & arg):
        theft_id(arg.theft_id),
        context_id(arg.context_id),
        session_id(arg.session_id),
        start_date(arg.start_date),
        end_date(arg.end_date),
        thief(arg.thief),
        victim(arg.victim),
        tasks_owner(arg.tasks_owner),
        theft_status(arg.theft_status),
        nb_task_theft(arg.nb_task_theft)
        {}
    
    size_t theft_id;
    size_t context_id;
    size_t session_id;
    uint64_t start_date;
    uint64_t end_date;
    
    gaspi_rank_t thief;
    gaspi_rank_t victim;
    gaspi_rank_t tasks_owner;
        
    TITUS_DLB_int theft_status;
    size_t nb_task_theft;
    TITUS_DLB_int first_task_id;
    
    void print(std::ostream & out = std::cout,bool one_line_print = false)const;
};

static inline std::ostream & operator << (std::ostream & out, const theft_info & arg) {
    arg.print(out,true);
    return out;
}

struct TITUS_DLB_session_info{
    TITUS_DLB_session_info();
    // SESSION
    uint64_t session_start;
    uint64_t session_end;
    size_t session_id;
    static size_t session_count;
    
    // WORKER
    size_t spawned_tasks_count;
    size_t completed_tasks_count;

    uint64_t last_work_start;
    uint64_t last_work_end;
    uint64_t time_spent_working;

    uint64_t last_task_start;
    uint64_t last_task_end;
    uint64_t time_spent_solving_tasks;

    uint64_t last_move_from_segment_start;
    uint64_t last_move_from_segment_end;
    uint64_t time_spent_moving_from_segment;
    size_t tasks_moved_from_segment_count;
    
    // VICTIM
    size_t tasks_moved_from_dequeue_count;
    uint64_t last_move_from_dequeue_start;
    uint64_t last_move_from_dequeue_end;
    uint64_t time_spent_moving_from_dequeue;
    
    // THIEF
    unsigned int try_theft_count;
    unsigned int hit_count;
    unsigned int stolen_tasks_count;
    unsigned int miss_notask_count;
    unsigned int miss_target_locked_count; 
    unsigned int miss_free_for_copy; 

    theft_info last_theft;
    uint64_t time_spent_stealing;
    uint64_t time_spent_stealing_hit;
    
	// RETURN RESULTS
	uint64_t return_results_start;
	uint64_t time_spent_returning_results;
	size_t results_pushed_count;

    // TERMINATION DETECTION
    uint64_t subtree_termination_detect_date;
    uint64_t session_termination_detect_date;
    
    uint64_t init_problem_time;
    uint64_t last_init_problem_start;
    uint64_t init_segment_time;
    uint64_t last_init_segment_start;

    uint64_t parallel_work_session_time;
    uint64_t last_parallel_work_session_start;
    
    uint64_t session_time;
    TITUS_DLB_session_info & operator+=(TITUS_DLB_session_info & arg);
    void print(std::ostream & out = std::cout)const;
};
static inline std::ostream & operator<<(std::ostream & out, const TITUS_DLB_session_info & arg) {
    arg.print(out);
    return out;
}

// theft info circular buffer
class theft_info_buffer{
    theft_info * buf;
    size_t size; // max number of theft_info
    size_t front;
    size_t tail; 
    bool autodump;
    std::ostream * dump_out;
public :
    theft_info_buffer(size_t size, bool autodump = false, std::ostream & dump_out = std::cout):
        buf(new theft_info[size]),
        size(size),
        front(0),
        tail(0),
        autodump(autodump),
        dump_out(&dump_out)
    {
        
    }
    ~theft_info_buffer() {
        if (autodump) dump();
    }
    const theft_info & get_info(size_t index)const
        {return buf[index];}
    
    size_t get_front_index() {return (front - tail)%size;}

    void push(const theft_info & arg) {
        new (& buf[front ++]) theft_info(arg);
        if (autodump){
			if(is_full()) dump();
		}
		else {
			front %= size;
			if (front == tail) (++tail) %= size;
		}
    }
    void erase() {
        tail = front = 0;
    }
    size_t is_empty() { 
        return (front == tail);
    }
    size_t is_full() { 
        return (front == size);
    }
    size_t get_length() {
        return front - tail;
    }
    size_t get_size() {
        return size;
    }
    bool autodump_is_enabled() { return autodump; }

    void set_autodump(bool val, std::ostream &out = TITUS_DBG) {
		if (autodump){
			// dump before setting autodump anew
			dump();
		}
        autodump = val;
        dump_out = &out;
    }
    void dump();
    static void print_header(std::ostream &out = std::cout) {
        out << "# thief  ; victim ; theft_id ; start_date ; end_date ; theft_status ; nb_task_theft ; first_task_id ; tasks_owner " << std::endl;
    }
    void print(size_t index_start, size_t index_end, std::ostream & out = std::cout)const;
    void print(std::ostream & out = std::cout)const;
};
static inline std::ostream & operator<<(std::ostream & out, const theft_info_buffer & arg) {
    arg.print(out);
    return out;
}

class TITUS_DLB_Logger_impl{
    TITUS_DLB_Context_impl * context;
    
    std::list<TITUS_DLB_session_info*> sessions;
    TITUS_DLB_session_info * current_session;
    TITUS_DLB_session_info agregated_sessions;
    
    static uint64_t now_rdtsc() {return ((double) rdtsc()) / ((double)TITUS_PROC_FREQ / 1e9);}
    static uint64_t now_gtod() {struct timeval tv;gettimeofday(&tv,0);return tv.tv_sec * 1e9 + tv.tv_usec * 1000;}
    
    uint64_t (*now)();
    
    bool log_all_thefts;
    
    theft_info_buffer buffer;

public:
    TITUS_DLB_Logger_impl(TITUS_DLB_Context_impl * arg, bool log_all = true, size_t local_buffer_size_in_bytes = TITUS_DLB_LOGGER_THEFT_BUFFER_SIZE,bool use_rdtsc = true, bool autodump = false);
    ~TITUS_DLB_Logger_impl() {
    }

    void print_current_session(std::ostream & out = std::cout) const {current_session->print(out);}

    void print_all_sessions(std::ostream & out = std::cout) const {
        for (std::list<TITUS_DLB_session_info*>::const_iterator it = sessions.begin(); it != sessions.end() ; it ++){
            (*it)->print(out);
            out << std::endl;
		}
    }
    void print_agregated_session_info(std::ostream & out = std::cout) const {
        agregated_sessions.print(out);
    }

    
    int get_some_metric() {
        return 0;
    }
    void activate_some_metric() {
        if (context == nullptr) {} //! TODO : burn the world to ashes
    }
    
    void dump_buffer() {
        buffer.dump();
    }
    void print_buffer(std::ostream & out)const{
        buffer.print(out);
    }
    void set_autodump(bool val, std::ostream & output = TITUS_DBG); //! TODO : ensure autodump dumps everything so that the user dont have to bother 
    void set_autodump(bool val, const char *logdir);
    
    // notification types (in TITUS_DLB)
    
    // INIT AND WORK SPAWNING
    // debut de session de travail (TITUS_DLB_work)
    void signal_start_TITUS_DLB_session();
    void signal_end_TITUS_DLB_session();

    // ajout de tâches
    // nb task
    void signal_tasks_spawned(int nb_tasks); // default value stands for self
    
    
    // WORKER

    // début de travail
    void signal_start_work();

    // fin de travail
    void signal_end_work();

    // début de tache
    void signal_start_task();

    // fin de tache
    void signal_end_task();

    // propriétaire de tâche
    //signal_task_owner(); // ??

    // début récupération des taches
    void signal_start_get_from_segment(); // rename ?

    // fin récupération des taches
    // nb taches récupérées
    void signal_end_get_from_segment(int nb_tasks); // rename ?

    // VICTIM

    // début de réapprovisionnement de segment
    void signal_start_put_on_segment();

    // fin de réapprovisionnement de segment
    // nb taches provisionnées
    void signal_end_put_on_segment(int nb_tasks);
    
    // THIEF

    // début du vol
    void signal_start_theft(gaspi_rank_t target_rank);

    // fin du vol
    // etat de la cible du vol
    // cible vol
    // nb tache volées
    void signal_end_theft(TITUS_DLB_int theft_status, size_t nb_task_theft, TITUS_DLB_int first_task_id, gaspi_rank_t target_rank, gaspi_rank_t owner);
    
	// RETURN RESULTS
	void signal_start_return_results();
	void signal_end_return_results();
	
	void signal_results_pushed(size_t result_count);

    // TERMINATION DETECTION
    // push all results (self + sons) ok
    void signal_subtree_termination_detected();
    // fin détéctée (entrée barrier)
    void signal_end_notification_received();
    
    void signal_start_problem_init();
    void signal_end_problem_init();
    void signal_start_segment_init();
    void signal_end_segment_init();
	
    void signal_start_parallel_work_session();
    void signal_end_parallel_work_session();
    
};

#endif //__TITUS_DLB_LOGGER_H__
