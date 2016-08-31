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
//  FILE	: dlb_logger.cpp
//  CONTENT	:
//
//====================================================================================


#include "dlb_logger.hpp"
#include "dlb_context.hpp"
#include <unistd.h>
// *********************************************************************
// *********************** DLB_session_info *****************************
// *********************************************************************
size_t DLB_session_info::session_count = 0;

DLB_session_info::DLB_session_info():
    session_start(0),
    session_end(0),
    session_id(session_count ++),
    spawned_tasks_count(0),
    completed_tasks_count(0),
    last_work_start(0),
    last_work_end(0),
    time_spent_working(0),
    last_task_start(0),
    last_task_end(0),
    time_spent_solving_tasks(0),
    last_move_from_segment_start(0),
    last_move_from_segment_end(0),
    time_spent_moving_from_segment(0),
    tasks_moved_from_segment_count(0),
    tasks_moved_from_dequeue_count(0),
    last_move_from_dequeue_start(0),
    last_move_from_dequeue_end(0),
    time_spent_moving_from_dequeue(0),
    try_theft_count(0),
    hit_count(0),
    stolen_tasks_count(0),
    miss_notask_count(0),
    miss_target_locked_count(0),
    miss_free_for_copy(0),
    last_theft(),
    time_spent_stealing(0),
    time_spent_stealing_hit(0),
   	return_results_start(0),
	time_spent_returning_results(0),
	results_pushed_count(0),
    subtree_termination_detect_date(0),
    session_termination_detect_date(0),
    init_problem_time(0),
    last_init_problem_start(0),
    init_segment_time(0),
    last_init_segment_start(0),
    parallel_work_session_time(0),
    last_parallel_work_session_start(0),
    session_time(0)
{
    
}
DLB_session_info & DLB_session_info::operator+=(DLB_session_info & arg) {
    session_start     = (session_start > arg.session_start) ? arg.session_start:session_start;
    session_end     = (session_start < arg.session_start) ? arg.session_start:session_start;
    
    spawned_tasks_count              += arg.spawned_tasks_count;
    completed_tasks_count            += arg.completed_tasks_count;
    time_spent_working               += arg.time_spent_working;
    time_spent_solving_tasks         += arg.time_spent_solving_tasks;
    time_spent_moving_from_segment   += arg.time_spent_moving_from_segment;
    time_spent_moving_from_dequeue   += arg.time_spent_moving_from_dequeue;
    tasks_moved_from_segment_count   += arg.tasks_moved_from_segment_count;
    tasks_moved_from_dequeue_count   += arg.tasks_moved_from_dequeue_count;
    try_theft_count                  += arg.try_theft_count;
    hit_count                        += arg.hit_count;
    stolen_tasks_count               += arg.stolen_tasks_count;
    miss_notask_count                += arg.miss_notask_count;
    miss_target_locked_count         += arg.miss_target_locked_count;
    miss_free_for_copy               += arg.miss_free_for_copy;
    time_spent_stealing              += arg.time_spent_stealing;
    time_spent_stealing_hit          += arg.time_spent_stealing_hit;
    time_spent_returning_results     += arg.time_spent_returning_results;
    results_pushed_count             += arg.results_pushed_count;
    init_problem_time                += arg.init_problem_time;
    init_segment_time                += arg.init_segment_time;
    parallel_work_session_time       += arg.parallel_work_session_time;
    session_time                     += arg.session_time;
    return *this;
}


void theft_info_buffer::dump() {
    ASSERT(dump_out != nullptr);
    print(*dump_out);
    erase();
    
}

void theft_info_buffer::print(size_t index_start, size_t index_end, std::ostream & out)const{
	if (index_start >= index_end) return;
    for (size_t i = index_start ; i != index_end ; i = (i+1) % size) {//! TODO : make it circular ?
        buf[i].print(out,true);
        //out.flush();
    }
}

void theft_info_buffer::print(std::ostream & out)const{
    for (size_t i = tail ; i != front ; i++) { //! TODO : make it circular ?
        buf[i].print(out,true);
    }
}

const char * theft_status_to_string(gaspi_atomic_value_t val){
	switch(val){
		case TASK_AVAILABLE: return "TASK_AVAILABLE" ; break;
		case FREE_FOR_COPY: return "FREE_FOR_COPY" ; break;
		case NO_TASK: return "NO_TASK" ; break;
		case COPY_IN_PROGRESS: return "COPY_IN_PROGRESS" ; break;
		default : 
			if (val < DLB::get_context()->get_nb_ranks())
				return "SEGMENT_LOCKED";
			return "UNKNOWN_THEFT_STATUS ???";
	}
}
void theft_info::print(std::ostream & out,bool one_line_print)const {
	char * theft_status_str = nullptr;
	if (theft_status < DLB::get_context()->get_nb_ranks()){
		theft_status_str = new char[64];
		sprintf(theft_status_str, "SEGMENT_LOCKED_BY_RANK_%lu", theft_status); 
	}
	
    if (one_line_print) {
        out << 
            thief << " ; " << victim << " ; " << theft_id << " ; " << 
            start_date << " ; " << end_date << " ; " << 
            ((theft_status_str != nullptr) ? theft_status_str : theft_status_to_string(theft_status)) << 
            " ; " << nb_task_theft << " ; " << first_task_id << " ; " << tasks_owner << 
            std::endl;
    }
    else{
        out << "thief : " << thief << std::endl;
        out << "victim : " << victim << std::endl;
        out << "theft_id : " << theft_id << std::endl;
        
        out << "start_date : " << start_date << std::endl;
        out << "end_date : " << end_date << std::endl;
        
        out << "theft_status : " << theft_status_to_string(theft_status) << std::endl;
        out << "nb_task_theft : " << nb_task_theft << std::endl;
        out << "first_task_id : " << first_task_id << std::endl;
        out << "tasks_owner : " << tasks_owner << std::endl;
    }
}
    
void DLB_session_info::print(std::ostream & out)const {
    // SESSION
    out << "session_id : " << session_id << std::endl;
    
    // WORKER
    out << "spawned_tasks_count : " << spawned_tasks_count << std::endl;
    out << "completed_tasks_count : " << completed_tasks_count << std::endl;
    out << "time_spent_working : " << time_spent_working << std::endl;
    out << "time_spent_solving_tasks : " << time_spent_solving_tasks << std::endl;
    out << "time_spent_moving_from_segment : " << time_spent_moving_from_segment << std::endl;
    out << "tasks_moved_from_segment_count : " << tasks_moved_from_segment_count << std::endl;
    
    // VICTIM
    out << "tasks_moved_from_dequeue_count : " << tasks_moved_from_dequeue_count << std::endl;
    out << "time_spent_moving_from_dequeue : " << time_spent_moving_from_dequeue << std::endl;
    
    // THIEF
    out << "try_theft_count : " << try_theft_count << std::endl;
    out << "hit_count : " << hit_count << std::endl;
    out << "stolen_tasks_count : " << stolen_tasks_count << std::endl;
    out << "miss_notask_count : " << miss_notask_count << std::endl;
    out << "miss_target_locked_count : " << miss_target_locked_count << std::endl;
    out << "miss_free_for_copy : " << miss_free_for_copy << std::endl;
    out << "time_spent_stealing : " << time_spent_stealing << std::endl;
    out << "time_spent_stealing_hit : " << time_spent_stealing_hit << std::endl;
    
    // RETURN RESULTS
    out << "time_spent_returning_results : " << time_spent_returning_results << std::endl;
    out << "results_pushed_count : " << results_pushed_count << std::endl;

    // TERMINATION DETECTION
    out << "subtree_termination_detect_date : " << subtree_termination_detect_date - session_start << std::endl;
    out << "session_termination_detect_date : " << session_termination_detect_date - session_start << std::endl;
    out << "init_problem_time : " << init_problem_time << std::endl;
    out << "init_segment_time : " << init_segment_time << std::endl;
    out << "parallel_work_session_time : " << parallel_work_session_time << std::endl;
    out << "session_time : " << session_time << std::endl;
}


// *********************************************************************
// *********************** DLB_Logger_impl *****************************
// *********************************************************************

DLB_Logger_impl::DLB_Logger_impl(DLB_Context_impl * arg, bool log_all, size_t local_buffer_size_in_bytes,bool use_rdtsc,bool autodump):
    context(arg), current_session(nullptr), log_all_thefts(log_all), buffer(local_buffer_size_in_bytes / sizeof(theft_info))
{
    if (use_rdtsc) now = &now_rdtsc;
    else now = &now_gtod;
    set_autodump(autodump);
}

void DLB_Logger_impl::set_autodump(bool val, std::ostream & output)
{
    buffer.set_autodump(val,output);
}
void DLB_Logger_impl::set_autodump(bool val, const char *logdir){
	if (!val){
		set_autodump(false);
	}
	std::stringstream theft_dump_filename("");
	if (logdir == NULL)
		theft_dump_filename << ".";
	else 
		theft_dump_filename << logdir;

	theft_dump_filename << "theft_dump_" << context->get_id() << "_rank" << context->get_rank() << ".log"; //! TODO : handle cases where communication layer cannot yield a proper rank id (random hash ? core_id ? pid ?thread id ?)
	std::ofstream theft_dump_out(theft_dump_filename.str().c_str());
	set_autodump(val,theft_dump_out);
}

// *********************************************************************
// ******************* DLB_Logger_impl::signal_... *********************
// *********************************************************************

// INIT AND WORK SPAWNING
// debut de session de travail (DLB_work)
void DLB_Logger_impl::signal_start_DLB_session() {
    sessions.push_front(new DLB_session_info() );
    current_session = sessions.front();
    current_session->session_start = now();
    
}
void DLB_Logger_impl::signal_end_DLB_session() {
    current_session->session_end = now();
    current_session->session_time = current_session->session_end - current_session->session_start;
    agregated_sessions += *current_session;
}

// ajout de tâches
// nb task
void DLB_Logger_impl::signal_tasks_spawned(int nb_tasks) { // default value stands for self
    current_session->spawned_tasks_count += nb_tasks;
}

// WORKER

// début de travail
void DLB_Logger_impl::signal_start_work() {
    current_session->last_work_start = now();
}

// fin de travail
void DLB_Logger_impl::signal_end_work() {
    current_session->last_work_end = now();
    current_session->time_spent_working += current_session->last_work_end - current_session->last_work_start;
}

// début de tache
void DLB_Logger_impl::signal_start_task() {
    current_session->last_task_start = now();
}

// fin de tache
void DLB_Logger_impl::signal_end_task() {
    current_session->last_task_end = now();
    current_session->time_spent_solving_tasks += current_session->last_task_end - current_session->last_task_start;
    current_session->completed_tasks_count ++;
}

// propriétaire de tâche
//signal_task_owner(); // ??

// début récupération des taches
void DLB_Logger_impl::signal_start_get_from_segment() { // rename ?
    current_session->last_move_from_segment_start = now();
}

// fin récupération des taches
// nb taches récupérées
void DLB_Logger_impl::signal_end_get_from_segment(int nb_tasks) { // rename ?
    current_session->last_move_from_segment_end = now();
    current_session->time_spent_moving_from_segment += current_session->last_move_from_segment_end - current_session->last_move_from_segment_start;
    current_session->tasks_moved_from_segment_count += nb_tasks;
}

// VICTIM

// début de réapprovisionnement de segment
void DLB_Logger_impl::signal_start_put_on_segment() {
    current_session->last_move_from_dequeue_start = now();
}

// fin de réapprovisionnement de segment
// nb taches provisionnées
void DLB_Logger_impl::signal_end_put_on_segment(int nb_tasks) {
    current_session->tasks_moved_from_dequeue_count += nb_tasks;
    current_session->last_move_from_dequeue_end = now();
    current_session->time_spent_moving_from_dequeue += current_session->last_move_from_dequeue_end - current_session->last_move_from_dequeue_start;
}

// THIEF

// début du vol
void DLB_Logger_impl::signal_start_theft(gaspi_rank_t target_rank) {
    //std::cout << "rank " << context->get_rank() << " entered theft" << std::endl;
    static bool first_theft = true;
    if (first_theft == true){
		TITUS_DBG << "first theft (target = " << target_rank << ")" << std::endl; //TITUS_DBG.flush();
		first_theft = false;
	}
    current_session->last_theft.victim = target_rank;
    current_session->last_theft.thief = context->get_rank();
    current_session->last_theft.start_date = now();
    current_session->last_theft.theft_id = current_session->try_theft_count++;
}

// fin du vol
// etat de la cible du vol
// cible vol
// nb tache volées
void DLB_Logger_impl::signal_end_theft(dlb_int theft_status, size_t nb_task_theft, dlb_int first_task_id, gaspi_rank_t target_rank, gaspi_rank_t owner) {
    static bool first_theft = 1;
    if (first_theft && theft_status == TASK_AVAILABLE) {
        //std::cout << "rank " << context->get_rank() << " first successful theft : target = " << target_rank << ", nb_task_theft = " << nb_task_theft << std::endl;
        first_theft = ! first_theft;
    }
    current_session->last_theft.end_date = now();
    current_session->time_spent_stealing += current_session->last_theft.end_date - current_session->last_theft.start_date;
    current_session->last_theft.theft_status = theft_status;
    current_session->last_theft.nb_task_theft = nb_task_theft;
    current_session->last_theft.victim = target_rank;
    current_session->last_theft.tasks_owner = owner;
    current_session->last_theft.first_task_id = first_task_id;
    switch(theft_status) {
        case FREE_FOR_COPY : current_session->miss_free_for_copy ++; break;
        case NO_TASK : current_session->miss_notask_count ++; break;
        case TASK_AVAILABLE : // success, theft status is the number of tasks stolen
            current_session->hit_count ++;
            current_session->stolen_tasks_count += nb_task_theft;
            current_session->time_spent_stealing_hit += current_session->last_theft.end_date - current_session->last_theft.start_date;
            break;
        default : // failed : segment was locked by another thief, status is the rank id of that thief
            current_session->miss_target_locked_count ++; break;
    };
    //if (log_all_thefts)
        buffer.push(current_session->last_theft);

    //std::cout << "rank " << context->get_rank() << " exited theft" << std::endl;
    //usleep(50000);
}

// RETURN RESULTS
void DLB_Logger_impl::signal_start_return_results() {
    current_session->return_results_start = now();    
}
void DLB_Logger_impl::signal_end_return_results() {
    uint64_t _now = now();
    current_session->time_spent_returning_results += _now - current_session->return_results_start;
    current_session->return_results_start = _now;
}


void DLB_Logger_impl::signal_results_pushed(size_t result_count){
	current_session->results_pushed_count += result_count;
}

// TERMINATION DETECTION
// push all results (self + sons) ok
void DLB_Logger_impl::signal_subtree_termination_detected() {
    TITUS_DBG << "signal_subtree_termination_detected" << std::endl;
    current_session->subtree_termination_detect_date = now();
}
// fin détéctée (entrée barrier)
void DLB_Logger_impl::signal_end_notification_received() {
    TITUS_DBG << "signal_end_notification_received" << std::endl;
    current_session->session_termination_detect_date = now();
}

void DLB_Logger_impl::signal_start_problem_init() {
    current_session->last_init_problem_start = now();    
}
void DLB_Logger_impl::signal_end_problem_init() {
    uint64_t _now = now();
    current_session->init_problem_time += _now - current_session->last_init_problem_start;
    current_session->last_init_problem_start = _now;
}


void DLB_Logger_impl::signal_start_segment_init() {
    current_session->last_init_segment_start = now();    
}
void DLB_Logger_impl::signal_end_segment_init() {
    uint64_t _now = now();
    current_session->init_segment_time += _now - current_session->last_init_segment_start;
    current_session->last_init_segment_start = _now;
}
void DLB_Logger_impl::signal_start_parallel_work_session() {
    current_session->last_parallel_work_session_start = now();    
}
void DLB_Logger_impl::signal_end_parallel_work_session() {
    uint64_t _now = now();
    current_session->parallel_work_session_time += _now - current_session->last_parallel_work_session_start;
    current_session->last_parallel_work_session_start = _now;
}
