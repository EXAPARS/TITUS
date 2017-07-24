#ifndef _M_TITUS_DBG_H
#define _M_TITUS_DBG_H

#include <GASPI.h>
#include <GASPI_Ext.h>

#define SUCCESS_OR_DIE(f...)                                                                        \
do                                                                                                  \
{                                                                                                   \
    uint64_t f_called = rdtsc();                                                                    \
    const gaspi_return_t __r__ = f;                                                                 \
    uint64_t f_returned = rdtsc();                                                                  \
    double time_ms = (double)(f_returned - f_called) / (((double)TITUS_PROC_FREQ) / 1e3);           \
    if (time_ms > 10.0){                                                                            \
		char str[512];                                                                              \
		sprintf (str, "Warning : %f ms in '%s' [%s:%i]\n", time_ms, #f, __FILE__, __LINE__);        \
		TITUS_DBG << str;                                                                           \
	}                                                                                               \
	if (__r__ != GASPI_SUCCESS)                                                                     \
    {                                                                                               \
        printf("SUCCESS_OR_DIE failed at [%s:%i].\n",__FILE__,__LINE__);                            \
        gaspi_rank_t rank;                                                                          \
        gaspi_proc_rank(&rank);                                                                     \
        fprintf (stderr,"\nError rank %d: '%s' [%s:%i]: %i\n",rank, #f, __FILE__, __LINE__, __r__); \
        fprintf (stderr," \n Error %i : %s\n",__r__, gaspi_error_str(__r__));                       \
        print_stacktrace_and_quit(-1);                                                              \
        abort();                                                                                    \
    }                                                                                               \
} while (0);

#define ASSERT(x...)                                                                    \
if (!(x))                                                                               \
{                                                                                       \
	gaspi_rank_t rank;                                                                  \
    gaspi_proc_rank(&rank);                                                             \
    fprintf (stderr, "Error rank %d : '%s' [%s:%i]\n", rank, #x, __FILE__, __LINE__);   \
    fflush (stderr);                                                                    \
    print_stacktrace_and_quit(-1);                                                      \
    abort();                                                                            \
}

//! todo : cleanup.

class TITUS_gaspi_logger_simplistic{
	std::ostream * out;
	uint64_t timeref;
	static void add_proc_rank_str(std::ostream & out); // appends "rank [rankno] : " to output stream
public:
	static gaspi_rank_t get_proc_rank();// return some available rank id for logging purpose. make no critical use of the returned value.
	// uses either gaspi_logger or cerr
	TITUS_gaspi_logger_simplistic(std::ostream & out = std::cerr) : out(&out){reset_timeref();}

	virtual ~TITUS_gaspi_logger_simplistic(){std::cerr << "deleting TITUS_gaspi_logger_simplistic" << std::endl;}

	void reset_timeref(){ timeref = rdtsc(); }
	void set_output(std::ostream * out){ this->out = out; }
	
	std::ostream & print_header ();
	
	void flush(){};
	
	operator std::ostream&(){return *out;}
	
	template <typename T>
	std::ostream & operator << (const T & arg){ return print_header() << arg;}
};


class TITUS_gaspi_logger : public std::stringstream{
	bool use_gaspi_logger;
	std::ostream * out;
	uint64_t timeref;
	static void add_proc_rank_str(std::ostream & out); // appends "rank [rankno] : " to output stream
public:
	static gaspi_rank_t get_proc_rank();// return some available rank id for logging purpose. make no critical use of the returned value.
	// uses either gaspi_logger or cerr
	TITUS_gaspi_logger(bool use_gaspi_logger = false) : std::stringstream(""),use_gaspi_logger(use_gaspi_logger),out(&std::cerr){reset_timeref();}
	// uses custom ostream
	TITUS_gaspi_logger(std::ostream & out) : std::stringstream(""),use_gaspi_logger(false),out(&out){reset_timeref();}
	
	virtual ~TITUS_gaspi_logger(){flush(); std::cerr << "deleting TITUS_gaspi_logger" << std::endl;}

	void flush();

	void reset_timeref(){ timeref = rdtsc(); }
	void set_output(std::ostream & out){ this->out = &out; }
};

#define TITUS_DBG_SIMPLISTIC
//#define TITUS_DBG_WRAPPED

//#define TITUS_DBG (std::cerr)
#ifdef TITUS_DBG_SIMPLE
#define TITUS_DBG (std::cerr)
#endif // TITUS_DBG_SIMPLE

#ifdef TITUS_DBG_ORIG
static TITUS_gaspi_logger _TITUS_GASPI_LOGGER;
#define TITUS_DBG (_TITUS_GASPI_LOGGER)
#endif // TITUS_DBG_ORIG

#ifdef TITUS_DBG_WRAPPED
extern TITUS_gaspi_logger * get_TITUS_DBG();
#define TITUS_DBG (*get_TITUS_DBG())
#endif // TITUS_DBG_WRAPPED

#ifdef TITUS_DBG_SIMPLISTIC
extern TITUS_gaspi_logger_simplistic & get_TITUS_DBG();
extern TITUS_gaspi_logger_simplistic * m_TITUS_DBG;
#define TITUS_DBG (get_TITUS_DBG())
#endif // TITUS_DBG_WRAPPED_SIMPLISTIC

#ifdef TITUS_DBG_WRAPPED_SIMPLISTIC
extern std::ostream & get_TITUS_DBG();
extern TITUS_gaspi_logger_simplistic * m_TITUS_DBG;
#define TITUS_DBG (get_TITUS_DBG())
#endif // TITUS_DBG_WRAPPED_SIMPLISTIC

#endif // _M_TITUS_DBG_H
