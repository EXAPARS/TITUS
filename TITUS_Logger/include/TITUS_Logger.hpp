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

#ifndef _M_TITUS_LOGGER_H
#define _M_TITUS_LOGGER_H

#include <map>
#include <algorithm>
#include <iostream>
#include <string>
#include <atomic>
#include <cmath>

#include <rdtsc.h>

//~ #define _MEASURE_LOGGER_OVERHEAD

class TITUS_Logger;


class TITUS_Logger_Entry_base{
	static std::atomic<int> id_count;
	const int id;
public :
	TITUS_Logger_Entry_base(TITUS_Logger * arg);
	virtual TITUS_Logger_Entry_base * clone()const = 0;
	
	virtual void reset();
	virtual void finalize(); // a chance to ensures consistency before printing. Does not reset.
	virtual void aggregate(const TITUS_Logger_Entry_base & arg); // add the value of the counter in argument to this
	const int & get_id()const{return id;};
	virtual const std::string get_name()const {return std::to_string(id);}
	virtual void print(std::ostream & out = std::cout, const std::string & prefix = "")const {out << prefix << "TITUS_Logger_Entry_base[" << get_name() << "] : nothing" << std::endl; }
};

static std::ostream & operator << (std::ostream & out, const TITUS_Logger_Entry_base & arg){
	arg.print(out);
	return out;
}

// virtual base class for const-named logger entries
class TITUS_Logger_Entry_named : public TITUS_Logger_Entry_base{
	const std::string name;
public :
	TITUS_Logger_Entry_named(TITUS_Logger * arg, const std::string & name);
	TITUS_Logger_Entry_named(const TITUS_Logger_Entry_named & arg); // copy constructor
	virtual TITUS_Logger_Entry_base * clone()const = 0;
	
	virtual void reset();
	virtual void finalize(); // a chance to ensures consistency before printing
	virtual void aggregate(const TITUS_Logger_Entry_base & arg); // add the value of the counter in argument to this
	virtual const std::string get_name()const ;
	virtual void print(std::ostream & out = std::cout, const std::string & prefix = "")const ;
};



template <typename counter_t>
class TITUS_Logger_Entry_counter : public TITUS_Logger_Entry_named{
protected :
	counter_t init_val;
	counter_t count;
public :
	TITUS_Logger_Entry_counter(TITUS_Logger * arg, const std::string & name, counter_t initial_value = 0);
	TITUS_Logger_Entry_counter(const TITUS_Logger_Entry_counter & arg); // copy constructor
	virtual TITUS_Logger_Entry_base * clone()const;
	
	virtual void reset();
	virtual void finalize(); // a chance to ensures consistency before printing
	virtual void aggregate(const TITUS_Logger_Entry_base & arg); // add the value of the counter in argument to this
	virtual const std::string get_name()const;
	virtual void print(std::ostream & out = std::cout, const std::string & prefix = "")const;
	
	operator counter_t &() {return count;}
	operator const counter_t &() const {return count;}
};

// base histogram class
template <typename value_t, size_t size> // log(value_t) must exist and return something that can be casted to an unsigned integer for addressing in the histogram
class TITUS_Logger_pow_histogram{
protected:
	const double power;
	const value_t start;
	std::array<value_t, size> histogram;
public :
	TITUS_Logger_pow_histogram(double power, const value_t & start);
	TITUS_Logger_pow_histogram(const TITUS_Logger_pow_histogram & arg); // copy constructor

	void insert(const value_t & arg, int64_t increment_value = 1);
	virtual void reset();
	virtual void finalize(); // a chance to ensures consistency before printing
	virtual void aggregate(const TITUS_Logger_pow_histogram<value_t, size> & arg); // add the value of the counter in argument to this
	virtual void print(std::ostream & out = std::cout, const std::string & prefix = "")const;
};

template <typename value_t, size_t size> // log(value_t) must exist and return something that can be casted to an unsigned integer for addressing in the histogram
class TITUS_Logger_Entry_pow_histogram : public TITUS_Logger_Entry_named, public TITUS_Logger_pow_histogram<value_t, size>{
public :
	TITUS_Logger_Entry_pow_histogram(TITUS_Logger * arg, const std::string & name, double power, const value_t & start);
	TITUS_Logger_Entry_pow_histogram(const TITUS_Logger_Entry_pow_histogram & arg); // copy constructor
	virtual TITUS_Logger_Entry_base * clone()const;

	virtual void reset();
	virtual void finalize(); // a chance to ensures consistency before printing
	virtual void aggregate(const TITUS_Logger_Entry_base & arg); // add the value of the counter in argument to this
	virtual const std::string get_name()const;
	virtual void print(std::ostream & out = std::cout, const std::string & prefix = "")const;
};


class TITUS_Logger_Entry_Timer_ns : public TITUS_Logger_Entry_counter<TITUS_Logger_raw_time_t>{
protected :
	TITUS_Logger_raw_time_t last_start;
	TITUS_Logger_raw_time_t last_stop;
public :
	TITUS_Logger_Entry_Timer_ns(TITUS_Logger * arg, const std::string & name);
	TITUS_Logger_Entry_Timer_ns(const TITUS_Logger_Entry_Timer_ns & arg); // copy constructor
	virtual TITUS_Logger_Entry_base * clone() const;
	
	virtual void reset();
	virtual void finalize(); // a chance to ensures consistency before printing
	virtual void aggregate(const TITUS_Logger_Entry_base & arg); // add the value of the counter in argument to this
	virtual const std::string get_name()const;
	virtual void print(std::ostream & out = std::cout, const std::string & prefix = "")const;
	
	// measures time deltas and increments time counter if
	// - start was used after start
	// - stop was used after start
	void start();
	void stop();
	
	// ! returns raw time. use TITUS_Logger::to_ns() to get a value in nanoseconds
	//~ TITUS_Logger_raw_time_t operator(TITUS_Logger_raw_time_t)const;
	//~ TITUS_Logger_raw_time_t & operator(TITUS_Logger_raw_time_t &);
	//~ const TITUS_Logger_raw_time_t & operator(TITUS_Logger_raw_time_t &)const;
};



template <size_t size> // log(value_t) must exist and return something that can be casted to an unsigned integer for addressing in the histogram
class TITUS_Logger_Entry_Timer_ns_pow_histogram : public TITUS_Logger_Entry_Timer_ns{
protected:
	TITUS_Logger_pow_histogram<uint64_t, size> histogram; //in ns
public :
	TITUS_Logger_Entry_Timer_ns_pow_histogram(TITUS_Logger * arg, const std::string & name, double power, TITUS_Logger_raw_time_t start = 1);
	TITUS_Logger_Entry_Timer_ns_pow_histogram(const TITUS_Logger_Entry_Timer_ns_pow_histogram & arg); // copy constructor
	virtual TITUS_Logger_Entry_base * clone()const;

	void start();
	void stop();

	virtual void reset();
	virtual void finalize(); // a chance to ensures consistency before printing
	virtual void aggregate(const TITUS_Logger_Entry_base & arg); // add the value of the counter in argument to this
	virtual const std::string get_name()const;
	virtual void print(std::ostream & out = std::cout, const std::string & prefix = "")const;
};

template <size_t size> // log(value_t) must exist and return something that can be casted to an unsigned integer for addressing in the histogram
class Logger_Overhead_Timer: public TITUS_Logger_Entry_Timer_ns_pow_histogram<size>{
	public:
	Logger_Overhead_Timer(TITUS_Logger * arg, const std::string & name, double power, TITUS_Logger_raw_time_t start = 1);
	Logger_Overhead_Timer(const Logger_Overhead_Timer & arg); // copy constructor
	virtual TITUS_Logger_Entry_base * clone()const;

	void start(); // just override start and stop to avoid infinite recursive call
	void stop(); // just override start and stop to avoid infinite recursive call

	virtual void reset();
	virtual void finalize(); // a chance to ensures consistency before printing
	virtual void aggregate(const TITUS_Logger_Entry_base & arg); // add the value of the counter in argument to this
	virtual const std::string get_name()const;
	virtual void print(std::ostream & out = std::cout, const std::string & prefix = "")const;
};

// from http://anki3d.org/spinlock/
class SpinLock
{
    std::atomic_flag lck = ATOMIC_FLAG_INIT;
public:
    void lock();
    void unlock();
};

typedef std::map<const std::string, TITUS_Logger_Entry_base *> TITUS_Logger_session_t;

class TITUS_Logger{
	SpinLock insertion_locked; // must be init before session_time
	TITUS_Logger_session_t current_session; // must be init before session_time
	TITUS_Logger_session_t aggregated_sessions; // must be init before session_time
	std::vector<TITUS_Logger_session_t> previous_sessions;
	TITUS_Logger_Entry_Timer_ns session_time;
public:
	#ifdef _MEASURE_LOGGER_OVERHEAD
	static Logger_Overhead_Timer<10> time_spent_logging;
	#endif //_MEASURE_LOGGER_OVERHEAD

	TITUS_Logger();
	void add_entry(TITUS_Logger_Entry_base * arg);
	
	void print_current_session(std::ostream & out = std::cout, const std::string & prefix = "")const;
	void print_all_sessions(std::ostream & out = std::cout, const std::string & prefix = "")const;
	void print_aggregated_sessions(std::ostream & out = std::cout, const std::string & prefix = "")const;
	void reset_all_entries();
	
	void start_session();
	void end_session();
	
	// generic timing measurement mean and its ad-hoc conversion to nanoseconds
	// TODO : extend to support systems where rdtsc is not available
	static TITUS_Logger_raw_time_t now(){
		return rdtsc();
	}
	static uint64_t to_ns(const TITUS_Logger_raw_time_t & arg){
		return arg / 2.4;
	}
	static TITUS_Logger_raw_time_t to_raw(const uint64_t & arg){
		return arg * 2.4;
	}
	
	//~ static inline double now_gtod()
	//~ {
		//~ struct timeval tp;
		//~ if (gettimeofday (&tp, NULL) < 0)	{	perror ("gettimeofday failed");		}
		//~ return (double) tp.tv_sec + ((double) tp.tv_usec) * 1.e-6;
	//~ }

};

static std::ostream & operator << (std::ostream & out, const TITUS_Logger & arg){
	arg.print_current_session(out);
	return out;
}

#include "../src/TITUS_Logger.cpp"

#endif // _M_TITUS_LOGGER_H
