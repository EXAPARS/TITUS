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

#include <TITUS_DBG.hpp>

#include <iomanip>

gaspi_rank_t TITUS_gaspi_logger::get_proc_rank(){ // return some available rank id for logging purpose. make no critical use of the returned value.
	// return the closest thing we have to a proc rank.
	gaspi_number_t gaspi_is_init ; gaspi_initialized (& gaspi_is_init);
	if (gaspi_is_init)
		{gaspi_rank_t ret; gaspi_proc_rank(&ret); return ret;}
	// else
#ifdef MPI // fallback : mpi interoperability mode
	int mpi_is_init; MPI_Initialized(&mpi_is_init);
	if (mpi_is_init) // if mpi is init, use mpi rank instead
		{int ret ; MPI_Comm_rank(MPI_COMM_WORLD, &ret); return ret;} //! TODO check type ?
#endif
	//~ else return tid
	return pthread_self();
}

void TITUS_gaspi_logger::add_proc_rank_str(std::ostream & out){
	out << "rank " << TITUS_gaspi_logger::get_proc_rank() << " : ";
}

void TITUS_gaspi_logger::flush(){
	if (str().empty()) {
		//*this << "empty" << std::endl;
		*out << "empty" << std::endl;
		return;
	}
//	rdtsc();

	if (use_gaspi_logger){
		gaspi_printf(str().c_str());
	}
	else {
		std::stringstream msg("");
		double now = ((double)(rdtsc() - timeref)) / TITUS_PROC_FREQ; // time, in seconds, elapsed since last call to reset_timeref()
		msg << std::fixed << std::setfill('0') << std::setw(14) << std::setprecision(8) << now << " ";
		add_proc_rank_str(msg); msg << str();
		(*out) << msg.str();
	}
	str("");
}

gaspi_rank_t TITUS_gaspi_logger_simplistic::get_proc_rank(){ // return some available rank id for logging purpose. make no critical use of the returned value.
	// return the closest thing we have to a proc rank.
	gaspi_number_t gaspi_is_init ; gaspi_initialized (& gaspi_is_init);
	if (gaspi_is_init)
		{gaspi_rank_t ret; gaspi_proc_rank(&ret); return ret;}
	// else
#ifdef MPI // fallback : mpi interoperability mode
	int mpi_is_init; MPI_Initialized(&mpi_is_init);
	if (mpi_is_init) // if mpi is init, use mpi rank instead
		{int ret ; MPI_Comm_rank(MPI_COMM_WORLD, &ret); return ret;} //! TODO check type ?
#endif
	//~ else return tid
	return pthread_self();
}

void TITUS_gaspi_logger_simplistic::add_proc_rank_str(std::ostream & out){
	out << "rank " << TITUS_gaspi_logger_simplistic::get_proc_rank() << " : ";
}

std::ostream & TITUS_gaspi_logger_simplistic::print_header(){
	std::stringstream header("");
	double now = ((double)(rdtsc() - timeref)) / TITUS_PROC_FREQ; // time, in seconds, elapsed since last call to reset_timeref()
	header << std::fixed << std::setfill('0') << std::setw(11) << std::setprecision(5) << now << " ";
	add_proc_rank_str(header);
	return (*out) << header.str();
}


#ifdef TITUS_DBG_SIMPLISTIC
TITUS_gaspi_logger_simplistic * m_TITUS_DBG = nullptr;
TITUS_gaspi_logger_simplistic & get_TITUS_DBG(){
	if (m_TITUS_DBG == nullptr) m_TITUS_DBG = new TITUS_gaspi_logger_simplistic();
	return *m_TITUS_DBG;
}
#endif //TITUS_DBG_SIMPLISTIC

#ifdef TITUS_DBG_WRAPPED_SIMPLISTIC
TITUS_gaspi_logger_simplistic * m_TITUS_DBG = nullptr;
std::ostream * get_TITUS_DBG(){
	if (m_TITUS_DBG == nullptr) m_TITUS_DBG = new TITUS_gaspi_logger_simplistic();
	m_TITUS_DBG->print_header();
	return m_TITUS_DBG;
}
#endif //TITUS_DBG_WRAPPED_SIMPLISTIC
#ifdef TITUS_DBG_WRAPPED
TITUS_gaspi_logger * get_TITUS_DBG(){
	static TITUS_gaspi_logger * m_TITUS_DBG = nullptr;
	if (m_TITUS_DBG == nullptr) m_TITUS_DBG = new TITUS_gaspi_logger();
	return m_TITUS_DBG;
}
#endif //TITUS_DBG_WRAPPED
