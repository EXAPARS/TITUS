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

#ifdef _M_TITUS_LOGGER_H // included from .hpp, provide only template implementation

#include <cassert>
#include <iomanip>
// ********************************************************
// *********** class TITUS_Logger_Entry_counter ***********
// ********************************************************

template <typename counter_t>
TITUS_Logger_Entry_counter<counter_t>::TITUS_Logger_Entry_counter(TITUS_Logger * arg, const std::string & name, counter_t initial_value)
	: TITUS_Logger_Entry_named(nullptr, name), init_val(initial_value), count(init_val)
{
	if (arg != nullptr) arg->add_entry(this);
}


template <typename counter_t>
TITUS_Logger_Entry_counter<counter_t>::TITUS_Logger_Entry_counter(const TITUS_Logger_Entry_counter & arg) // copy constructor
	: TITUS_Logger_Entry_named(arg), init_val(arg.init_val), count(arg.count)
{
	
}

template <typename counter_t>
TITUS_Logger_Entry_base * TITUS_Logger_Entry_counter<counter_t>::clone()const{
	return new TITUS_Logger_Entry_counter(*this);
}

template <typename counter_t>
void TITUS_Logger_Entry_counter<counter_t>::reset(){
	count = init_val;
}

template <typename counter_t>
void TITUS_Logger_Entry_counter<counter_t>::finalize(){
	// do nothing
}

template <typename counter_t>
void TITUS_Logger_Entry_counter<counter_t>::aggregate(const TITUS_Logger_Entry_base & arg){ // add the value of the counter in argument to this
	*this += (const TITUS_Logger_Entry_counter<counter_t> &)arg;
}

template <typename counter_t>
const std::string TITUS_Logger_Entry_counter<counter_t>::get_name()const {
	return TITUS_Logger_Entry_named::get_name();
}

template <typename counter_t>
void TITUS_Logger_Entry_counter<counter_t>::print(std::ostream & out, const std::string & prefix)const {
	out << prefix << get_name() << " : " << count << std::endl;
}



// ********************************************************
// *********** class TITUS_Logger_pow_histogram ***********
// ********************************************************
// base histogram class
template <typename value_t, size_t size>
TITUS_Logger_pow_histogram<value_t, size>::TITUS_Logger_pow_histogram(double power, const value_t & start):
	power(power), start(start)
{
	reset();
}

template <typename value_t, size_t size>
TITUS_Logger_pow_histogram<value_t, size>::TITUS_Logger_pow_histogram(const TITUS_Logger_pow_histogram<value_t, size> & arg) // copy constructor
	: power(arg.power), start(arg.start), histogram(arg.histogram)
{
}

template <typename value_t, size_t size>
void TITUS_Logger_pow_histogram<value_t, size>::insert(const value_t & arg, int64_t increment_value){
	histogram[
		arg < start ? 
			0 : 
			std::min(
				size-1,
				(size_t)(log(arg - start) / log(power))
			)
	] += increment_value;
}

template <typename value_t, size_t size>
void TITUS_Logger_pow_histogram<value_t, size>::reset(){
	histogram.fill(0);
}
template <typename value_t, size_t size>
void TITUS_Logger_pow_histogram<value_t, size>::finalize(){
	// do nothing
}

template <typename value_t, size_t size>
void TITUS_Logger_pow_histogram<value_t, size>::aggregate(const TITUS_Logger_pow_histogram<value_t, size> & arg){ // add the value of the counter in argument to this
	if (power != arg.power) std::cerr << "error aggregating TITUS_Logger_pow_histogram : powers do not match (power=" << power << ", arg.power=" << arg.power << ")" << std::endl;
	if (start != arg.start) std::cerr << "error aggregating TITUS_Logger_pow_histogram : starts do not match (start=" << start << ", arg.start=" << arg.start << ")" << std::endl;
	assert(	power == arg.power );
	assert(	start == arg.start );
	//~ histogram[:] += arg.histogram[:]; // Intel's C++ Extension for Array Notation or GCC's -fcilk for cilk+ array notation
	// DOESNT WORK ON STD::ARRAY ?????? :(
	for (int i = 0 ; i < size ; i ++) histogram[i] += arg.histogram[i];
}


template <typename value_t, size_t size>
void TITUS_Logger_pow_histogram<value_t, size>::print(std::ostream & out, const std::string & prefix)const {
	out << std::setprecision(std::max(1,(int)std::log10(1/(power -1.0)))+1);
	out << prefix << "  [-inf," << start << "[ : " << histogram[0] << std::endl;
	for (size_t i = 1 ; i < size-1 ; i++){
		out << prefix << "  [" << start * pow(power, i) << "," << start * pow(power, (i+1)) << "[ : " << histogram[i] << std::endl;
	}
	out << prefix << "  [" << start * pow(power, size) << ",+inf] : " << histogram[size-1] << std::endl;
}



// log(value_t) must exist and return something that can be casted to an unsigned integer for addressing in the histogram
// ********************************************************
// ******** class TITUS_Logger_Entry_pow_histogram ********
// ********************************************************

template <typename value_t, size_t size>
TITUS_Logger_Entry_pow_histogram<value_t, size>::TITUS_Logger_Entry_pow_histogram(TITUS_Logger * arg, const std::string & name, double pow, const value_t & start)
	: TITUS_Logger_Entry_named(nullptr, name), TITUS_Logger_pow_histogram<value_t, size>()
{
	if (arg != nullptr) arg->add_entry(this);
}

template <typename value_t, size_t size>
TITUS_Logger_Entry_pow_histogram<value_t, size>::TITUS_Logger_Entry_pow_histogram(const TITUS_Logger_Entry_pow_histogram & arg) // copy constructor
	: TITUS_Logger_Entry_named(arg), TITUS_Logger_pow_histogram<value_t, size>(arg)
{
	
}

template <typename value_t, size_t size>
TITUS_Logger_Entry_base * TITUS_Logger_Entry_pow_histogram<value_t, size>::clone()const{
	return new TITUS_Logger_Entry_pow_histogram<value_t, size>(*this);
}

template <typename value_t, size_t size>
void TITUS_Logger_Entry_pow_histogram<value_t, size>::reset(){
	TITUS_Logger_pow_histogram<value_t, size>::reset();
}

template <typename value_t, size_t size>
void TITUS_Logger_Entry_pow_histogram<value_t, size>::finalize(){
	// do nothing
}

template <typename value_t, size_t size>
void TITUS_Logger_Entry_pow_histogram<value_t, size>::aggregate(const TITUS_Logger_Entry_base & arg){ // add the value of the counter in argument to this
	TITUS_Logger_pow_histogram<value_t, size>::aggregate((const TITUS_Logger_pow_histogram<value_t, size> &)arg);
}

template <typename value_t, size_t size>
const std::string TITUS_Logger_Entry_pow_histogram<value_t, size>::get_name()const {
	return TITUS_Logger_Entry_named::get_name();
}

template <typename value_t, size_t size>
void TITUS_Logger_Entry_pow_histogram<value_t, size>::print(std::ostream & out, const std::string & prefix)const {
	out << prefix << get_name() << " : " << std::endl;
	TITUS_Logger_pow_histogram<value_t, size>::print(prefix, out);
}



// log(value_t) must exist and return something that can be casted to an unsigned integer for addressing in the histogram
// ********************************************************
// **** class TITUS_Logger_Entry_Timer_ns_pow_histogram ***
// ********************************************************

template <size_t size>
TITUS_Logger_Entry_Timer_ns_pow_histogram<size>::TITUS_Logger_Entry_Timer_ns_pow_histogram(TITUS_Logger * arg, const std::string & name, double power, TITUS_Logger_raw_time_t start)
	: TITUS_Logger_Entry_Timer_ns(nullptr, name), histogram(power, start)
{
	if (arg != nullptr) arg->add_entry(this);
	assert(power > 1);
	//~ std::cout << "new TITUS_Logger_Entry_pow_histogram : start = " << start << ", power = " << power << std::endl;
}


template <size_t size>
TITUS_Logger_Entry_Timer_ns_pow_histogram<size>::TITUS_Logger_Entry_Timer_ns_pow_histogram(const TITUS_Logger_Entry_Timer_ns_pow_histogram<size> & arg) // copy constructor
	: TITUS_Logger_Entry_Timer_ns(arg), histogram(arg.histogram)
{
	
}

template <size_t size>
TITUS_Logger_Entry_base * TITUS_Logger_Entry_Timer_ns_pow_histogram<size>::clone()const{
	return new TITUS_Logger_Entry_Timer_ns_pow_histogram<size>(*this);
}



template <size_t size>
void TITUS_Logger_Entry_Timer_ns_pow_histogram<size>::start(){
	#ifdef _MEASURE_LOGGER_OVERHEAD
	TITUS_Logger::time_spent_logging.start();
	#endif //_MEASURE_LOGGER_OVERHEAD
	auto t = TITUS_Logger::now();
	if (last_start > last_stop){
		uint64_t dt = t - last_start;
		histogram.insert(TITUS_Logger::to_ns(dt));
		*this += dt;
	}
	last_start = t;
	#ifdef _MEASURE_LOGGER_OVERHEAD
	TITUS_Logger::time_spent_logging.stop();
	#endif //_MEASURE_LOGGER_OVERHEAD
}

template <size_t size>
void TITUS_Logger_Entry_Timer_ns_pow_histogram<size>::stop(){
	#ifdef _MEASURE_LOGGER_OVERHEAD
	TITUS_Logger::time_spent_logging.start();
	#endif //_MEASURE_LOGGER_OVERHEAD
	
	auto t = TITUS_Logger::now();
	if (last_start > last_stop){
		uint64_t dt = t - last_start;
		histogram.insert(TITUS_Logger::to_ns(dt));
		*this += dt;
	}
	last_stop = t;

	#ifdef _MEASURE_LOGGER_OVERHEAD
	TITUS_Logger::time_spent_logging.stop();
	#endif //_MEASURE_LOGGER_OVERHEAD
}

template <size_t size>
void TITUS_Logger_Entry_Timer_ns_pow_histogram<size>::reset(){
	TITUS_Logger_Entry_Timer_ns::reset() ; histogram.reset();
}

template <size_t size>
void TITUS_Logger_Entry_Timer_ns_pow_histogram<size>::finalize(){
	TITUS_Logger_Entry_Timer_ns::finalize();
}

template <size_t size>
void TITUS_Logger_Entry_Timer_ns_pow_histogram<size>::aggregate(const TITUS_Logger_Entry_base & arg){ // add the value of the counter in argument to this
	TITUS_Logger_Entry_Timer_ns::aggregate(arg);

	histogram.aggregate(((const TITUS_Logger_Entry_Timer_ns_pow_histogram &)arg).histogram);
}

template <size_t size>
const std::string TITUS_Logger_Entry_Timer_ns_pow_histogram<size>::get_name()const {
	return TITUS_Logger_Entry_Timer_ns::get_name();
}

template <size_t size>
void TITUS_Logger_Entry_Timer_ns_pow_histogram<size>::print(std::ostream & out, const std::string & prefix)const{
	TITUS_Logger_Entry_Timer_ns::print(out, prefix);
	histogram.print(out, prefix);
	out << std::endl;
}




#ifdef _MEASURE_LOGGER_OVERHEAD

template <size_t size> // log(value_t) must exist and return something that can be casted to an unsigned integer for addressing in the histogram
void Logger_Overhead_Timer<size>::start(){ // override start and stop to avoid infinite recursive call
	auto t = TITUS_Logger::now();
	if (TITUS_Logger_Entry_Timer_ns_pow_histogram<size>::last_start > TITUS_Logger_Entry_Timer_ns_pow_histogram<size>::last_stop){
		uint64_t dt = t - TITUS_Logger_Entry_Timer_ns_pow_histogram<size>::last_start;
		TITUS_Logger_Entry_Timer_ns_pow_histogram<size>::histogram.insert(TITUS_Logger::to_ns(dt));
		*this += dt;
	}
	TITUS_Logger_Entry_Timer_ns_pow_histogram<size>::last_start = t;
}
template <size_t size> // log(value_t) must exist and return something that can be casted to an unsigned integer for addressing in the histogram
void Logger_Overhead_Timer<size>::stop(){ // override start and stop to avoid infinite recursive call
	auto t = TITUS_Logger::now();
	if (TITUS_Logger_Entry_Timer_ns_pow_histogram<size>::last_start > TITUS_Logger_Entry_Timer_ns_pow_histogram<size>::last_stop){
		uint64_t dt = t - TITUS_Logger_Entry_Timer_ns_pow_histogram<size>::last_start;
		TITUS_Logger_Entry_Timer_ns_pow_histogram<size>::histogram.insert(TITUS_Logger::to_ns(dt));
		*this += dt;
	}
	TITUS_Logger_Entry_Timer_ns_pow_histogram<size>::last_stop = t;
}

template <size_t size> // log(value_t) must exist and return something that can be casted to an unsigned integer for addressing in the histogram
Logger_Overhead_Timer<size>::Logger_Overhead_Timer(TITUS_Logger * arg, const std::string & name, double power, TITUS_Logger_raw_time_t start)
	: TITUS_Logger_Entry_Timer_ns_pow_histogram<size>(arg,name,power,start)
{
}

template <size_t size> // log(value_t) must exist and return something that can be casted to an unsigned integer for addressing in the histogram
Logger_Overhead_Timer<size>::Logger_Overhead_Timer(const Logger_Overhead_Timer & arg) // copy constructor
	: TITUS_Logger_Entry_Timer_ns_pow_histogram<size>(arg)
{
	
}

template <size_t size> // log(value_t) must exist and return something that can be casted to an unsigned integer for addressing in the histogram
TITUS_Logger_Entry_base * Logger_Overhead_Timer<size>::clone()const{
	return new Logger_Overhead_Timer<size>(*this);
}

template <size_t size> // log(value_t) must exist and return something that can be casted to an unsigned integer for addressing in the histogram
void Logger_Overhead_Timer<size>::reset(){return TITUS_Logger_Entry_Timer_ns_pow_histogram<size>::reset();}
template <size_t size> // log(value_t) must exist and return something that can be casted to an unsigned integer for addressing in the histogram
void Logger_Overhead_Timer<size>::finalize(){return TITUS_Logger_Entry_Timer_ns_pow_histogram<size>::finalize();} // a chance to ensures consistency before printing
template <size_t size> // log(value_t) must exist and return something that can be casted to an unsigned integer for addressing in the histogram
void Logger_Overhead_Timer<size>::aggregate(const TITUS_Logger_Entry_base & arg){return TITUS_Logger_Entry_Timer_ns_pow_histogram<size>::aggregate(arg);} // add the value of the counter in argument to this
template <size_t size> // log(value_t) must exist and return something that can be casted to an unsigned integer for addressing in the histogram
const std::string Logger_Overhead_Timer<size>::get_name()const {return TITUS_Logger_Entry_Timer_ns_pow_histogram<size>::get_name();}
template <size_t size> // log(value_t) must exist and return something that can be casted to an unsigned integer for addressing in the histogram
void Logger_Overhead_Timer<size>::print(std::ostream & out, const std::string & prefix)const {return TITUS_Logger_Entry_Timer_ns_pow_histogram<size>::print(out, prefix);}

#endif //_MEASURE_LOGGER_OVERHEAD



#else // compiling file normally








#include <TITUS_Logger.hpp>
#include <fstream>


// from http://anki3d.org/spinlock/
void SpinLock::lock(){
	while(lck.test_and_set(std::memory_order_acquire))
	{}
}

void SpinLock::unlock(){
	lck.clear(std::memory_order_release);
}


// ********************************************************
// ****************** class TITUS_Logger ******************
// ********************************************************


TITUS_Logger::TITUS_Logger()
	:session_time(this,"session_time")
{
#ifdef _MEASURE_LOGGER_OVERHEAD
	add_entry(&time_spent_logging);
#endif //_MEASURE_LOGGER_OVERHEAD
}

#ifdef _MEASURE_LOGGER_OVERHEAD
Logger_Overhead_Timer<10> TITUS_Logger::time_spent_logging(nullptr, "logger_overhead", 10.0);
#endif //_MEASURE_LOGGER_OVERHEAD



void TITUS_Logger::start_session(){
	session_time.start();
}
void TITUS_Logger::end_session(){
	session_time.stop();
	previous_sessions.push_back(TITUS_Logger_session_t());
	// copy all entries to a new map
	// also increment map of aggregated counters
	decltype(aggregated_sessions)::iterator it = aggregated_sessions.begin();
	for (auto & e : current_session){
		e.second->finalize();
		previous_sessions[previous_sessions.size()-1].insert(TITUS_Logger_session_t::value_type(e.first, e.second->clone()));
		
		// merge two sorted maps using a virtual aggregate function
		while (it != aggregated_sessions.end() && it->first < e.first) it++;
		if (it != aggregated_sessions.end() && it->first == e.first) it->second->aggregate(*e.second);
		else if (it == aggregated_sessions.end() || it->first > e.first) {it = aggregated_sessions.insert(decltype(aggregated_sessions)::value_type(e.first, e.second->clone())).first; ; };
	}
}

void TITUS_Logger::add_entry(TITUS_Logger_Entry_base * arg){
	insertion_locked.lock();
	auto r = current_session.insert(
		std::pair<const std::string &,TITUS_Logger_Entry_base *>(arg->get_name(), arg)
	);
	insertion_locked.unlock();
	if (r.second == false){
		std::cerr << "error : TITUS_Logger::add_entry : another entry with the same name already exists (" << arg->get_name() << "). assert is going to fail." << std::endl;
	}
	assert(r.second);
}

void TITUS_Logger::print_current_session(std::ostream & out, const std::string & prefix)const{
	//~ for (auto & e : *this) e.second->print(out, prefix);
	for (auto & e : current_session) e.second->print(out, prefix);
}

void TITUS_Logger::print_all_sessions(std::ostream & out, const std::string & prefix)const{
	//~ for (auto & e : *this) e.second->print(out, prefix);
	for (auto & e : current_session) e.second->print(out, prefix);
	for (auto & s : previous_sessions)
		for (auto & e : s) e.second->print(out, prefix);
}

void TITUS_Logger::print_aggregated_sessions(std::ostream & out, const std::string & prefix)const{
	for (auto & e : current_session) e.second->print(out, prefix);	
}

void TITUS_Logger::reset_all_entries(){
	for (auto & e : current_session) e.second->reset();
}



// base class for logger entries
// ********************************************************
// ************* class TITUS_Logger_Entry_base ************
// ********************************************************

std::atomic<int> TITUS_Logger_Entry_base::id_count(0);

TITUS_Logger_Entry_base::TITUS_Logger_Entry_base(TITUS_Logger * arg)
	: id(id_count++)
{
	if (arg != nullptr) arg->add_entry(this);
}

void TITUS_Logger_Entry_base::reset(){}
void TITUS_Logger_Entry_base::finalize(){} // a chance to ensures consistency before printing. Does not reset.
void TITUS_Logger_Entry_base::aggregate(const TITUS_Logger_Entry_base & arg){} // add the value of the counter in argument to this


// base class for const-named logger entries
// ********************************************************
// ************ class TITUS_Logger_Entry_named ************
// ********************************************************

TITUS_Logger_Entry_named::TITUS_Logger_Entry_named(TITUS_Logger * arg, const std::string & name)
	: TITUS_Logger_Entry_base(nullptr), name(name)
{
	if (arg != nullptr) arg->add_entry(this);
}

TITUS_Logger_Entry_named::TITUS_Logger_Entry_named(const TITUS_Logger_Entry_named & arg) // copy constructor
	: TITUS_Logger_Entry_base(nullptr), name(arg.name)
{
	
}

void TITUS_Logger_Entry_named::reset(){}
void TITUS_Logger_Entry_named::finalize(){} // a chance to ensures consistency before printing
void TITUS_Logger_Entry_named::aggregate(const TITUS_Logger_Entry_base & arg){} // add the value of the counter in argument to this


const std::string TITUS_Logger_Entry_named::get_name()const {
	return name;
}

void TITUS_Logger_Entry_named::print(std::ostream & out, const std::string & prefix)const {
	out << prefix << get_name() << " : nothing" << std::endl;
}



// ********************************************************
// *********** class TITUS_Logger_Entry_Timer_ns **********
// ********************************************************
TITUS_Logger_Entry_Timer_ns::TITUS_Logger_Entry_Timer_ns(TITUS_Logger * arg, const std::string & name)
	: TITUS_Logger_Entry_counter(nullptr, name), last_start(0), last_stop(0)
{
	if (arg != nullptr) arg->add_entry(this);
}

TITUS_Logger_Entry_Timer_ns::TITUS_Logger_Entry_Timer_ns(const TITUS_Logger_Entry_Timer_ns & arg) // copy constructor
	: TITUS_Logger_Entry_counter(arg), last_start(arg.last_start), last_stop(arg.last_stop)
{
	
}

TITUS_Logger_Entry_base * TITUS_Logger_Entry_Timer_ns::clone() const{
	return new TITUS_Logger_Entry_Timer_ns(*this);
}

void TITUS_Logger_Entry_Timer_ns::reset(){
	TITUS_Logger_Entry_counter::reset();
}

void TITUS_Logger_Entry_Timer_ns::finalize(){
	stop();
}

void TITUS_Logger_Entry_Timer_ns::aggregate(const TITUS_Logger_Entry_base & arg){ // add the value of the counter in argument to this
	TITUS_Logger_Entry_counter<TITUS_Logger_raw_time_t>::aggregate(arg);
}

const std::string TITUS_Logger_Entry_Timer_ns::get_name()const {
	return TITUS_Logger_Entry_counter::get_name();
}

void TITUS_Logger_Entry_Timer_ns::print(std::ostream & out, const std::string & prefix)const {
	out << prefix << get_name() << " : " << TITUS_Logger::to_ns(*this) << std::endl;
}

// measures time deltas and increments time counter if
// - start was used after start
// - stop was used after start
void TITUS_Logger_Entry_Timer_ns::start(){
	#ifdef _MEASURE_LOGGER_OVERHEAD
	TITUS_Logger::time_spent_logging.start();
	#endif //_MEASURE_LOGGER_OVERHEAD
	auto t = TITUS_Logger::now(); 
	if (last_start > last_stop) *this += t - last_start;
	last_start = t;
	#ifdef _MEASURE_LOGGER_OVERHEAD
	TITUS_Logger::time_spent_logging.stop();
	#endif //_MEASURE_LOGGER_OVERHEAD
}
void TITUS_Logger_Entry_Timer_ns::stop(){
	#ifdef _MEASURE_LOGGER_OVERHEAD
	TITUS_Logger::time_spent_logging.start();
	#endif //_MEASURE_LOGGER_OVERHEAD
	auto t = TITUS_Logger::now(); 
	if (last_start > last_stop) *this += t - last_start;
	last_stop = t;
	#ifdef _MEASURE_LOGGER_OVERHEAD
	TITUS_Logger::time_spent_logging.stop();
	#endif //_MEASURE_LOGGER_OVERHEAD
}

//~ uint64_t TITUS_Logger_Entry_Timer_ns::operator(uint64_t)const {return count;}
//~ uint64_t & TITUS_Logger_Entry_Timer_ns::operator(uint64_t &) {return count;}
//~ const uint64_t & TITUS_Logger_Entry_Timer_ns::operator(uint64_t &)const {return count;}



#endif // _M_TITUS_LOGGER_H
