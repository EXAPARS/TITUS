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


#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GASPI.h>
#include <stdint.h>
#include <math.h>
#include <TITUS_DLB.hpp>
#include <sys/time.h>
#include <cassert>
#include <ostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <TITUS_DLB_gaspi_tools.hpp>

#if GASPI_MAJOR_VERSION==1 && GASPI_MINOR_VERSION<3
#define GASPI_TOPOLOGY_NONE 0
#define GASPI_TOPOLOGY_STATIC 1
#define GASPI_TOPOLOGY_DYNAMIC 2
#endif

// @ BENCHMARK PARAMETERS
size_t TASK_NUMBER             = 10000;
double TASK_TIME               = 0.0006;
size_t NB_INTEGER_PER_TASK     = 10;            //>= 10 integer
size_t SHARED_SEGMENT_SIZE     = 10000;            //>= 10 000 bytes
TITUS_DLB::TITUS_DLB_Algorithm algorithm   = TITUS_DLB::WORK_STEALING;


double sum_double_array(double *array, int lenght)
{
    double sum = 0.0;
    for (int i=0;i<lenght;++i)
        sum += array[i];
    return sum;
}

int search_index_double_array(double *array, int lenght, double element)
{
    for (int i=0;i<lenght;++i)
    {
        if(array[i] > element)  return i;
    }
            return lenght-1;
}

// generates a normalized crescent cumulative distribution in place of the input data array of size length.
void create_cumulative_distribution_function(double *array,  int lenght)
{
    double sum;
    sum = sum_double_array(array,lenght);
    
    for (int i=0;i<lenght;++i)
    {
        array[i] = array[i]/sum;
    }
    for (int i=1;i<lenght;++i)
    {
        array[i] = array[i] + array[i-1];
    }
}


// draws randomly task repartition using the repartition function specified by array
// both input and results shall be at least of allocated size given by length.
// array_result[...].first is filled with the number of tasks for each process
// array_result[...].second is filled with the id of the first task for each process
void create_distribution_array(double *array, std::pair<size_t,size_t> * array_result, int lenght)
{
    int index, remainder, quotient;
    size_t limit;
    limit = 1000000;
    if (TASK_NUMBER > limit)
    {
        quotient    = TASK_NUMBER/limit;
        remainder    = TASK_NUMBER%limit;
        
        for (int i=0;i<limit;++i)
        {
            index                     = search_index_double_array(array, lenght, (double) rand()/RAND_MAX);
            array_result[index].first += quotient;
        }
        for (int i=0;i<remainder;++i)
        {
            index                     = search_index_double_array(array, lenght, (double) rand()/RAND_MAX);
            array_result[index].first += 1;
        }
    }
    else
    {
        for (size_t i=0;i<TASK_NUMBER;++i)
        {
            index                     = search_index_double_array(array, lenght, (double) rand()/RAND_MAX);
            array_result[index].first += 1;
        }
    }
    array_result[0].second = 0;
    for(size_t i=1 ; i<lenght ; i++){
        array_result[i].second = array_result[i-1].first + array_result[i-1].second;
    }
    return;
}

// generates a gaussian distribution
void gaussian_distribution(std::pair<size_t,size_t> * array_task_distribution, int nb_procs)
{
    double x1, x2, step;
    double mu, sigma;
    double *t;
    
    t        = (double *) malloc(nb_procs * sizeof(double)); ASSERT(t != nullptr);
    x1      = 0;
    x2      = 10;
    step    = (x2 - x1)/nb_procs;
    mu      = 5.0;
    sigma    = 0.5;
    for (int i=0;i<nb_procs;++i)    t[i] = (double)exp((-1.0) * pow(i*step - mu,2)/(2.0*sigma*sigma));
    
    create_cumulative_distribution_function(t, nb_procs);
    create_distribution_array(t, array_task_distribution, nb_procs);
    
    free(t);
    return;
}

// initialises a bit of the problem array, starting at adress prointed to by array_problem
// each tasks_info.first subsequent indices of the array is then numbered starting with tasks_info.second.
void init_problem(int *array_problem, std::pair<size_t, size_t> tasks_info)
{
    int rank; MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    int ctr = 10*tasks_info.second;
    
    for(size_t i=0 ; i < tasks_info.first; ++i)
    {
        for(size_t j=0;j<NB_INTEGER_PER_TASK;++j)
        {
            if(j>=10)    array_problem[i*NB_INTEGER_PER_TASK+j] = rand();
            else
            {
                array_problem[i*NB_INTEGER_PER_TASK+j] = ctr;
                ctr++;
            }
        }
    }
}


void display_solve_problem_mpi(int *array_result, int nb_result)
{
    printf("\n====================BEGIN RESULT========================\n");
    for(int i=0;i<nb_result;++i)
    {
        if(i%20 == 0)    printf("%d ",array_result[i]);
        if(i%100 == 0)  printf("\n");
    }
    printf("\n=====================END RESULT=========================\n");
}


void raz_result(int *array_result, int nb_result)
{
    for(int i=0;i<nb_result;++i)
        array_result[i] = 0;
}

#ifndef __ICC
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
void solve_task(void *task, void *result, void * unused_params)
{
    double t1,t2;
    int r;
    int *t, *res;
    t = (int *)task;
    r=0;

    t1 = TITUS_Logger::to_ns(TITUS_Logger::now()) / 1e9;
    
    for(int j=0 ; j<10 ; ++j)
        r = r + t[j];
    
    do { t2 = TITUS_Logger::to_ns(TITUS_Logger::now()) / 1e9; }
    while( (t2-t1)<TASK_TIME );  // active wait until TASK_TIME us timeout is reached
    
    res = (int *)result;
    *res = r;
}
#ifndef __ICC
	#pragma GCC diagnostic pop
#endif

void resolve_problem(int *array_problem, int *array_result, int nb_task)
{
    for(int i=0;i<nb_task;++i)
        solve_task(&array_problem[i*NB_INTEGER_PER_TASK], &array_result[i], NULL);
}


int str_startswith(char * arg, char * startswith){
    int r = 1;
    for (int i=0;startswith[i] != '\0';i++)
        if (arg[i] != startswith[i]) r = 0;
    return r;
}

void parse_program_args(int argc, char ** argv){
    for (int i=0;i<argc;i++){
        //printf("now parsing %s\n",argv[i]);
        if (str_startswith(argv[i],(char *)"TASK_NUMBER=")) TASK_NUMBER = atoll(argv[i]+12);
        if (str_startswith(argv[i],(char *)"TASK_TIME=")) TASK_TIME = atof(argv[i]+10);
        if (str_startswith(argv[i],(char *)"NB_INTEGER_PER_TASK=")) NB_INTEGER_PER_TASK = atoll(argv[i]+20);
        if (str_startswith(argv[i],(char *)"SHARED_SEGMENT_SIZE=")) SHARED_SEGMENT_SIZE = atoll(argv[i]+20);
        if (!strcmp(argv[i],(char *)"-WS")) algorithm = TITUS_DLB::WORK_STEALING;
        if (!strcmp(argv[i],(char *)"-WR")) algorithm = TITUS_DLB::WORK_REQUESTING;
    }
}
void print_bench_params(){
	std::cout << std::endl << "----------- BENCH PARAMETERS ------------"
	<< std::endl << "TASK_NUMBER=" << TASK_NUMBER
	<< std::endl << "TASK_TIME=" << TASK_TIME
	<< std::endl << "NB_INTEGER_PER_TASK=" << NB_INTEGER_PER_TASK
	<< std::endl << "SHARED_SEGMENT_SIZE=" << SHARED_SEGMENT_SIZE
	<< std::endl;
}


#ifndef __ICC
        #pragma GCC diagnostic push
        #pragma GCC diagnostic warning "-fpermissive"
#endif
void print_results(char * filename, int* array_result, std::pair<size_t, size_t> tasks_info){
    MPI_File out;
    std::stringstream msg ("");
    
    int mpi_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    
    //sprintf(BUFFER+strlen(BUFFER),"\n\nrank %d \n", mpi_rank);
    for (size_t i=0;i<tasks_info.first;++i){
        msg << array_result[i] << " ";
	if ((tasks_info.second + i + 1) % 30 == 0) msg << std::endl;
    }
    if (MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_WRONLY | MPI_MODE_CREATE, MPI_INFO_NULL, &out))		  { MPI_Abort(MPI_COMM_WORLD, 911);    }
    if (MPI_File_write_ordered(out, ((void *)msg.str().c_str()), msg.str().length(), MPI_CHAR, MPI_STATUS_IGNORE))                         { MPI_Abort(MPI_COMM_WORLD, 911);    }
    if (MPI_File_close(&out))                                                                                     { MPI_Abort(MPI_COMM_WORLD, 911);    }

    MPI_Barrier(MPI_COMM_WORLD);
}
#ifndef __ICC
        #pragma GCC diagnostic pop
#endif
	
int sequential_work(int * array_problem, int * array_result, int nb_task){
    int t1,t2;
    
    t1 = rdtsc();
    resolve_problem(&array_problem[0], &array_result[0], nb_task);
    MPI_Barrier(MPI_COMM_WORLD);
    t2 = rdtsc();
    
    return t2-t1;
}


int main(int argc, char **argv)
{    //TITUS_DLB_sig_handler dummy;

	uint64_t rdtsc_start = rdtsc();

    parse_program_args(argc,argv);
	
    MPI_Init(&argc,&argv);
	
    int rank_mpi;
    int nb_procs_mpi;
    int * array_problem;
    int * array_result;
    std::pair<size_t, size_t> nb_task;
    int nb_result;
    std::pair<size_t, size_t> * array_task_distribution = nullptr;
    int task_size;
    
// ***************************************************
// ******************* MPI INIT **********************
// ***************************************************

    MPI_Comm_size(MPI_COMM_WORLD, &nb_procs_mpi);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank_mpi);
	
	if (rank_mpi==0)print_bench_params();
	
    
// ***************************************************
// ***************** PROBLEM INIT ********************
// ***************************************************
// distributed generation of the "problem" data :
// process 0 generates the distribution of tasks among processes and scatters the information
    if  (rank_mpi == 0)
    {
		array_task_distribution = new std::pair<size_t, size_t> [nb_procs_mpi];
        memset(array_task_distribution,0,nb_procs_mpi * sizeof(std::pair<size_t, size_t>));
        gaussian_distribution(array_task_distribution, nb_procs_mpi);
        printf("\n");
    }
    MPI_Scatter(array_task_distribution, 2, MPI_UINT64_T , &nb_task, 2, MPI_UINT64_T, 0, MPI_COMM_WORLD);
    fprintf(stderr, "rank %d : nb_task=%lu\n",rank_mpi, nb_task.first);

// each process then allocates an appropriatly sized table and initializes it.
    nb_result        = nb_task.first;
    task_size        = NB_INTEGER_PER_TASK*sizeof(int);
    if (task_size < 40) {TITUS_DBG << "WARNING : task_size < 10 integers"; TITUS_DBG.flush();}
    array_problem    = (int *) malloc(nb_task.first*task_size); //attention la dequeue doit être au moins de la taille du segment soit 10000 octets (4000)
    ASSERT(array_problem != nullptr);
    array_result    = (int *) malloc(nb_result*sizeof(int));
    ASSERT(array_result != nullptr);
    
    if(rank_mpi == 0)    free(array_task_distribution);
    
    init_problem(&array_problem[0], nb_task);

    MPI_Barrier(MPI_COMM_WORLD);
  
    
// ***************************************************
// **************** SEQUENTIAL WORK ******************
// ***************************************************
  
   sequential_work(array_problem, array_result, nb_task.first);
    
   print_results("result_ref.txt", array_result, nb_task);
    //raz_result(array_result, nb_result);
  

    
// ***************************************************
// ****************** GASPI INIT *********************
// ***************************************************

    uint64_t before_gaspi_proc_init = rdtsc();
    
    
    //set gaspi topology infrastructure mode
    //~ gaspi_config_t config;
    //~ SUCCESS_OR_DIE( gaspi_config_get(&config));
    
    //~ config.build_infrastructure = GASPI_TOPOLOGY_DYNAMIC;
    
    //~ SUCCESS_OR_DIE( gaspi_config_set(config));
    //~ if (rank_mpi == 0) print_gaspi_config();
	
    //~ SUCCESS_OR_DIE( gaspi_proc_init(GASPI_BLOCK));
    ASSERT ( TITUS_DLB::gaspi_proc_init(GASPI_BLOCK) == GASPI_SUCCESS );
    uint64_t gaspi_proc_init_time = (rdtsc() - before_gaspi_proc_init) / (TITUS_PROC_FREQ /1000);
    uint64_t time_so_far = (rdtsc() - rdtsc_start) / (TITUS_PROC_FREQ / 1000);

    
    TITUS_DBG << " gaspi_proc_init_time : " << gaspi_proc_init_time << " ms"
			  << ", time until now = " << time_so_far << " ms" << std::endl;
	TITUS_DBG.flush();
    
    gaspi_rank_t rank;
    gaspi_rank_t nb_procs;
    SUCCESS_OR_DIE( gaspi_proc_rank(&rank)      );
    SUCCESS_OR_DIE( gaspi_proc_num(&nb_procs)    );
    assert(rank == rank_mpi);

// ***************************************************
// ****************** TITUS_DLB INIT ***********************
// ***************************************************
    
    uint64_t before_barrier_1 = rdtsc();
	
	TITUS_DBG << "entering TITUS_DLB::gaspi_barrier" << std::endl; TITUS_DBG.flush();
	
    SUCCESS_OR_DIE( TITUS_DLB::gaspi_barrier (GASPI_GROUP_ALL, GASPI_BLOCK) );
    uint64_t before_context_init = rdtsc();

    TITUS_DLB_Context * context = new TITUS_DLB_Context(SHARED_SEGMENT_SIZE, algorithm);
    TITUS_DLB::set_context(context);
	// set theft attempt traces autodump in their own set of files
	//std::stringstream theft_dump_filename("");
	//theft_dump_filename << "theft_dump_" << context->get_context_id() << "_rank" << context->get_rank() << ".log"; //! TODO ajouter le numero de context, ajouter le nom de la machine ?
	//std::ofstream theft_dump_out(theft_dump_filename.str());
    //context->get_logger()->set_autodump(true,theft_dump_out);

    uint64_t before_set_problem = rdtsc();

    context->set_problem((void *) &array_problem[0], task_size, nb_task.first,(void *) &array_result[0], sizeof(int), solve_task, nullptr);
    
    uint64_t after_set_problem = rdtsc();

    SUCCESS_OR_DIE( TITUS_DLB::gaspi_barrier (GASPI_GROUP_ALL, GASPI_BLOCK) );

    uint64_t after_barrier_2 = rdtsc();
    
    uint64_t barrier_1_time = (before_context_init - before_barrier_1) / (TITUS_PROC_FREQ / 1000);
    uint64_t barrier_2 = (after_barrier_2 - after_set_problem) / (TITUS_PROC_FREQ / 1000);
    uint64_t new_context_time = (before_set_problem - before_context_init) / (TITUS_PROC_FREQ / 1000);
    uint64_t set_problem_time = (after_set_problem - before_set_problem) / (TITUS_PROC_FREQ / 1000);
    time_so_far = (rdtsc() - rdtsc_start) / (TITUS_PROC_FREQ / 1000);
    TITUS_DBG << "gaspi_proc_init_time = " << gaspi_proc_init_time << "ms, "
			  << "time_so_far = " << time_so_far << " ms, "
			  << "barrier_1_time = " << barrier_1_time << " ms, "
			  << "barrier_2 = " << barrier_2 << " ms, "
			  << "new_context_time = " << new_context_time << " ms, "
			  << "set_problem_time = " << set_problem_time << " ms" << std::endl;
	//TITUS_DBG.flush();
// ***************************************************
// ****************** TITUS_DLB WORK ***********************
// ***************************************************
    
    uint64_t before_parallel_work = rdtsc();
	TITUS_DBG.reset_timeref();
	TITUS_DBG << "time reset" << std::endl; TITUS_DBG.flush();

    TITUS_DLB::parallel_work();

    uint64_t after_parallel_work = rdtsc();
    
    uint64_t parallel_work_time = (after_parallel_work - before_parallel_work) / (TITUS_PROC_FREQ / 1000);
    
    SUCCESS_OR_DIE( TITUS_DLB::gaspi_barrier (GASPI_GROUP_ALL, GASPI_BLOCK) );

    time_so_far = (rdtsc() - rdtsc_start) / (TITUS_PROC_FREQ / 1000);
    TITUS_DBG << "time_so_far = " << time_so_far << " ms, " << std::endl;
    TITUS_DBG << "parallel_work_time = " << parallel_work_time << " ms" << std::endl;
	//TITUS_DBG.flush();

// ***************************************************
// ***************** PRINT LOGS **********************
// ***************************************************
    if ( rank == 0 ){
		time_so_far = (rdtsc() - rdtsc_start) / (TITUS_PROC_FREQ / 1000);
		std::cout << "time_so_far = " << time_so_far << " ms, ";
		std::cout << "done." << std::endl;
	}

    std::stringstream outname("");
    outname << "logs_" << std::setfill('0') << std::setw(4) << rank << ".out";
    std::ostream * out = new std::ofstream(outname.str().c_str());
    context->get_logger()->print_aggregated_sessions(*out); // in logdir
    //context->get_logger()->dump_buffer(); // ensure full log dump in log dir
	
	if (rank == 0){
		std::stringstream report_str("");
		report_str << "echo \"## Assuming constant clock frequency @ 2600 MHz\" ;";
		report_str << "working=`cat logs_* | grep time_spent_solving_tasks | cut -d':' -f2 | xargs | tr ' ' '+' | bc | xargs | tr ' ' '+' | bc` ;";
		report_str << "ms_working=`echo $working / 1000000 | bc` ;";
		report_str << "echo total_work_time = $ms_working ms ;";
		report_str << "session_time=`cat logs_* | grep parallel_work_session_time | cut -d':' -f2 | sort -n | tail -n1` ;";
		report_str << "ms_session_time=`echo $session_time / 1000000 | bc` ;";
		report_str << "echo session_time = $ms_session_time ms;";
		report_str << "proc_count=`ls -1 logs_* | wc -l` ;";
		report_str << "echo proc_count = $proc_count ;";
		report_str << "echo efficiency = `echo \"$ms_working * 100 / $ms_session_time / $proc_count\" | bc`% ;";
		std::system(report_str.str().c_str());
	}

	TITUS_DBG << "calling gaspi_proc_term" << std::endl;
    SUCCESS_OR_DIE( gaspi_proc_term(GASPI_BLOCK) );
	TITUS_DBG << "gaspi_proc_term terminated" << std::endl;


// ***************************************************
// **************** PRINT RESULTS ********************
// ***************************************************
	
	print_results("result.txt"	, array_result, nb_task);
    

	TITUS_DBG << "calling MPI_Finalize" << std::endl;
    MPI_Finalize();
	TITUS_DBG << "MPI_Finalize terminated" << std::endl;

}
