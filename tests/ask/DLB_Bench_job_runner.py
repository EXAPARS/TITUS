#! /usr/bin/python3

import statistics
import os
import subprocess
import operator
import shutil
import sys
import math

TASK_COUNT 			= "TASK_NUMBER="
TASK_TIME 			= "TASK_TIME="
NB_INTEGER_FOR_TASK = "NB_INTEGER_FOR_TASK="
SHARED_SEGMENT_SIZE = "SHARED_SEGMENT_SIZE="
THEFT_STYLE 		= "-"
WORK_STEALING 		= "WS"
WORK_REQUESTING 	= "WR"

class machine_dependant_stuff(object):
	# default example implementation is for anselm@it4i
	machine_name = "Anselm@it4i"
	core_freq = 2.4e9
	cores_per_node = 16
	agregate_runs = True
	
	def script_header_str(self, job) :
		return  "#! /bin/sh \n" + \
				"module load openmpi\n"

	def get_mpi_launch_str(self,run) :
		return "mpirun -n " + str(run.get_rank_count()) + " "
				
	def submit_job(self,job) :
#		if job['nodes'] < 4 :
#			job['queue'] = "qexp"
#		else :
#			job['queue'] = "qprod"
		job.queue = "qprod"
		minutes = 1 + job.get_expected_runtime() // 60
		hours = int(minutes) // 60
		minutes = int(minutes) % 60
		job.submit_str = "qsub -A DD-15-6  -q " + job.queue + \
		" -l select=" + str(job.node_count) + ":ncpus=" + str(self.cores_per_node) + ":cpu_freq=24,walltime=" + str(hours).zfill(2) + ":" + str(minutes).zfill(2) + ":00 " + \
		"-l cpu_turbo_boost=0 " + job.path_to_script 
		print (job.submit_str)
		
		p = subprocess.Popen(job.submit_str,shell=True, stdout = subprocess.PIPE, stderr = subprocess.PIPE)
		r = p.communicate()
		#print(r) # is like (b'1170192.dm2\n', b'')
		job.job_id = r[0].decode("utf-8").strip()
		for l in r :
			print(l.decode("utf-8").strip())
		job.status = "running"
		self.jobs[job.job_id] = job
	
	def init_no_match_response(self) :
		p = subprocess.Popen("qstat | grep \"azertyuiopqsdfghjklm\" | wc -l",shell=True, stdout = subprocess.PIPE, stderr = subprocess.PIPE)
		self.no_match_response = p.communicate()
		
	
	def refresh_job_status(self,job):
		if job.status != "finished" :
			p = subprocess.Popen("qstat | grep \"" + job.job_id + "\" | wc -l",shell = True, stdout = subprocess.PIPE, stderr = subprocess.PIPE)
			r = p.communicate()
			if r == self.no_match_response : job.status = "finished"
			else : job.status = "running"
		
	
	def wait_for_job_termination(self):
		flag = True
		tries = 0
		while flag == True :
			running = 0
			flag = False
			for k in self.jobs :
				self.refresh_job_status(self.jobs[k])
				if self.jobs[k].status == "running" : 
					flag = True
					running = running + 1
			if flag == True :
				sys.stdout.write("\r waiting for " + str(running) + " jobs (" + str(len(self.jobs)) + " total)       " + str(tries * self.sleeptime // 60) + " minutes elapsed    ")
				sys.stdout.flush()
				tries = tries + 1
				if flag == True : os.system("sleep " + str(self.sleeptime))
				
	
	def __init__(self) :
		self.jobs = {}
		self.sleeptime = 60
		self.tries = 0
		
		self.init_no_match_response()

		self.DLB_HOME = os.getenv("DLB_HOME","~/DLB_Library")
		self.program = "DLB_Bench.bin"
		self.program_path = self.DLB_HOME  + "/tests/bin/" + self.program
		self.logs_dir = os.getenv("DLB_BENCH_LOGS",self.DLB_HOME + "/logs/DLB_Bench")
		self.launch_scripts_dir = self.DLB_HOME + "/tests/ask/scripts"



# not machine dependant
class DLB_Bench_run():

	def get_rank_count(self) :
		return self.node_count * machine.cores_per_node

	def get_task_count(self) :
		return self.tasks_per_rank * self.get_rank_count()

	def get_expected_runtime(self,without_load_balancing = True): # in seconds
		# expected runtime with load balancing
		expected_runtime = 2* self.task_time * float(self.tasks_per_rank) + self.get_rank_count() * math.log(self.get_rank_count(),2) / 50 + 60
		# expected runtime without load balancing, assuming most of the work is spread across 1/10th of the processes
		if without_load_balancing == True :
			expected_runtime = expected_runtime + (self.task_time * self.get_task_count()) / ((self.get_rank_count() / 10) + 1)
		return expected_runtime

	def get_log_dir(self) :
		log_dir_str = ("p" + str(self.get_rank_count()) + "-tn" + str(self.get_task_count()) + "-tx" + str(self.task_time) + "-tl" + str(self.nb_integer_per_task) + "-s" + str(self.shared_segment_size) + "-" + str(self.theft_style)).replace(".","_")
		if self.repetition != None : log_dir_str = log_dir_str + "_" + self.repetition
		return log_dir_str

	def get_log_dir_path(self) :
		return (self.logs_root_path + "/" + self.get_log_dir())
	
	def logdir_exists(self) :
		return os.path.isdir(self.get_log_dir_path())

	def write_request_in_log_dir(self) :
		run_request = open(self.get_log_dir_path() + "/request.dat",'w')
		run_request.write(self.request)
		run_request.close()

	def create_log_dir(self) :
		if self.logdir_exists() == False :
			os.makedirs(self.get_log_dir_path())
			self.write_request_in_log_dir()


	def get_command_line(self) :
		return self.program_path + " " +\
				TASK_COUNT + str(self.get_task_count()) + " " + \
				TASK_TIME + str(self.task_time) + " " + \
				NB_INTEGER_FOR_TASK + str(self.nb_integer_per_task) + " " + \
				SHARED_SEGMENT_SIZE + str(self.shared_segment_size) + " " + \
				THEFT_STYLE + self.theft_style + " "

	def start_gaspi_logger_str(self) :
		return "gaspi_logger &> gaspi_logger.out &\n"

	def stop_gaspi_logger_str(self) :
		return "sleep 2 && kill -2 `ps -a | grep gaspi_logger | sed \"s/^ \+//\" | cut -d' ' -f1 | xargs `\n"

	def get_run_str(self) :
		r = "cd " + self.get_log_dir_path() + "\n" + self.start_gaspi_logger_str() + machine.get_mpi_launch_str(self) + self.get_command_line() + "\n"
		if (machine.agregate_runs) :
			r = r + self.stop_gaspi_logger_str()
		return r

	def get_response_path(self) :
		return self.get_log_dir_path() + "/response.dat"

	# returns false or the efficiency (float ]0,1] ) for the run. also prints the response in the "response.dat" file in the logs directory
	def make_response(self) :
		if (os.path.exists(self.get_log_dir_path() + "/logs_" + str(0).zfill(4) + ".out") == False) :
			print("oops ! seems like the following run didnt went ok... (missing " + self.get_log_dir_path() + "/logs_" + str(0).zfill(4) + ".out)\n  " + self.get_log_dir_path() + "\n")
			return False
		if (os.path.exists(self.get_response_path())) : # response already exists, just return last field
			return float(open(self.get_response_path(),"r").read().split()[-1])
		self.completed_tasks_count = 0
		self.agregated_time_spent_solving_tasks = 0
		self.max_session_time = 0
		self.max_self_time = 0
		self.max_parallel_work_session_time = 0
		self.procs_stats = []
		for i in range(0,self.get_rank_count()) :
			self.procs_stats.append(type('', (), {})()) # empty class trick ;)
			if os.path.exists(self.get_log_dir_path() + "/logs_" + str(i).zfill(4) + ".out") == False :
					print("oops ! seems like the following run didnt went ok... (missing " + self.get_log_dir_path() + "/logs_" + str(i).zfill(4) + ".out)\n  " + self.get_log_dir_path() + "\n")
					return False # drop run
			#print("parsing " + self.get_log_dir_path() + "/logs_" + str(i).zfill(4) + ".out")
			with open(self.get_log_dir_path() + "/logs_" + str(i).zfill(4) + ".out") as log :
				for line in log.readlines() :
					if line.startswith("completed_tasks_count : "):
						self.procs_stats[i].completed_tasks_count = int(line.replace("completed_tasks_count : ",""))
					if line.startswith("time_spent_solving_tasks : "):
						self.procs_stats[i].time_spent_solving_tasks = int(line.replace("time_spent_solving_tasks : ",""))
					if line.startswith("session_time : "):
						self.procs_stats[i].session_time = int(line.replace("session_time : ",""))
					if line.startswith("init_problem_time : "):
						self.procs_stats[i].init_problem_time = int(line.replace("init_problem_time : ",""))
					if line.startswith("init_segment_time : "):
						self.procs_stats[i].init_segment_time = int(line.replace("init_segment_time : ",""))
					if line.startswith("parallel_work_session_time : "):
						self.procs_stats[i].parallel_work_session_time = int(line.replace("parallel_work_session_time : ",""))
						
				self.completed_tasks_count = self.completed_tasks_count + self.procs_stats[i].completed_tasks_count 
				self.agregated_time_spent_solving_tasks = self.agregated_time_spent_solving_tasks + self.procs_stats[i].time_spent_solving_tasks 
				self.max_session_time = max(self.max_session_time , self.procs_stats[i].session_time )
				self.max_self_time = max(self.max_self_time , self.procs_stats[i].session_time - self.procs_stats[i].init_segment_time )
				self.max_parallel_work_session_time = max(self.max_parallel_work_session_time , self.procs_stats[i].parallel_work_session_time )
		print("agregated_time_spent_solving_tasks = " + str(self.agregated_time_spent_solving_tasks))		
		print("max_session_time = " + str(self.max_session_time))		
		print("init_segment_time = " + str(self.max_session_time))		
		self.speedup = float(self.agregated_time_spent_solving_tasks) / float(self.max_parallel_work_session_time) 
		self.efficiency = float(self.speedup) / float(self.get_rank_count())
		response_str = self.request.replace("\n","") + " " + str(self.efficiency)
		with open(self.get_response_path(),'w') as response :
			response.write(response_str + "\n")
		return self.efficiency

	def __init__(self, request_line, program_path, logs_root_path, repetition = None, output_path = None):
		self.program_path = program_path
		self.request = request_line.strip()
		self.logs_root_path = logs_root_path
		self.repetition = None
		if repetition != None :
			self.repetition = str(repetition)

		args = request_line.split()
		#~ {"name": "log_node_count",
		 #~ "type": "integer",
		#~ },
		self.node_count = 2**int(args[0])
		if args[0] == 11 : self.node_count = 20508 / 16 #! TODO : REMOVE ME
		#~ {"name": "decomposition_factor"
		 #~ "type": "float",
		 #~ "range": {"min": 0.0, "max": 1.0 }
		#~ },
		decomposition_factor = float(args[1])
		#~ {"name": "work_per_proc",
		 #~ "type": "float",
		 #~ "range": {"min" : 1, "max": 60} 
		#~ },
		work_per_proc = float(args[2])
		#~ {"name": "mem_per_proc",
		 #~ "type": "integer",
		 #~ "range": {"min": 4000, "max": 43000000}
		#~ },
		mem_per_proc = int(args[3])
		#~ {"name": "segment_size_ratio",
		 #~ "type": "float",
		 #~ "range": {"min": 0.1, "max": 5}
		#~ }
		segment_size_ratio = float(args[4])

		min_tasks = 200
		max_tasks = mem_per_proc / 40
		self.tasks_per_rank = min_tasks + (max_tasks - min_tasks) * decomposition_factor
		self.task_time = work_per_proc / self.tasks_per_rank
		self.nb_integer_per_task = mem_per_proc / self.tasks_per_rank
		self.shared_segment_size = segment_size_ratio * mem_per_proc
		self.theft_style = WORK_STEALING # not a param for now

		if output_path == None :
			self.output_path = self.logs_root_path
		else :
			self.output_path = output_path
		self.ignored = self.logdir_exists()
		
		
		


# machine dependant
class DLB_Bench_job_script():
	"""The base job script definition for (Anselm@it4i)"""
	
	def add_run(self,run) :
		if self.node_count != 0 :
			if self.node_count != run.node_count :
				print ("DLB_Bench_job_script error : trying to add a run with a different node request requirement")
		else :
			self.node_count = run.node_count
		self.runs.append(run)
		self.ignored = self.ignored and run.ignored

	def add_runs(runs) :
		for run in runs :
			self.add_run(run)

	def write_script(self):
		if self.ignored == False :
			with open(self.path_to_script,'w') as script :
				script.write(machine.script_header_str(self))
				#script.write(self.get_kill_before_timeout_str())
				for run in self.runs :
					if run.ignored == False :
						script.write(run.get_run_str())
						run.create_log_dir()
				#script.write(self.get_kill_killer_str())

	def get_expected_runtime(self,without_load_balancing = False):
		r = 0;
		for run in self.runs :
			if run.ignored == False : r = r + run.get_expected_runtime()
		# add 20 sec to the actual sum of expected runtime of each run in order to get a chance to get a proper abort debug output
		return r + 120

	def get_kill_before_timeout_str(self):
		# kill all $PROGRAM processes after job expected runtime - 20 sec elapsed
		return "(sleep " + str(int(self.get_expected_runtime()) - 20) + " && kill -2 `ps -a | grep " + self.path_to_script.split("/")[-1] + " | sed \"s/^ \+//\" | cut -d' ' -f1 | tr \"\\n\" \" \"`)&\n"

	def get_kill_killer_str(self):
		# kill all sleep processes still pending after all runs have completed to finis cleaning up all child processes
		return "kill -2 `ps -a | grep sleep | sed \"s/^ \+//\" | cut -d' ' -f1 | tr \"\\n\" \" \"`\n"

	def print_report(self):
		print ("\njob " + self.path_to_script + " :")
		for i in range(len(self.runs)) :
			print ("runs[" + str(i) + "] : " + self.runs[i].get_log_dir_path())
			print ("   " +  machine.get_mpi_launch_str(self.runs[i]) + self.runs[i].get_command_line())
			if self.runs[i].ignored == True :
				print ("   ignored.")
			else :
				print ("   expected " + str(int(self.runs[i].get_expected_runtime())) + " secs runtime on " + str(self.runs[i].get_rank_count()) + " processes")
		if self.ignored == True :
			print ("ignored")
		else :
			print ("expected " + str(int(self.get_expected_runtime())) + " secs total runtime")
	
	def __init__(self,path_to_script):
		# todo fill up with relevant stuff
		self.path_to_script = path_to_script
		self.node_count = 0
		self.runs = []
		self.ignored = True


#not machine dependant
class DLB_Bench_ask_request_runner():
	"""The base job runner class (for Anselm@it4i)"""	
	
	def parse_request(self) :
		print ("\n################## parsing request ######################\n")
		run  = {}
		self.runs = []
		# parse requested
		with open(self.requested_filename) as requested :
			for line in requested :
				repetitions = None
				if line.startswith("#") or line.strip(" \n\t") == "" : continue
				if self.repeat_runs < 2 : repetitions = [DLB_Bench_run(line,machine.program_path,machine.logs_dir)]
				else : 
					repetitions = []
					for i in range(0,self.repeat_runs) : repetitions.append(DLB_Bench_run(line,machine.program_path,machine.logs_dir,i))
				self.runs.append(repetitions)

			#self.runs.sort(key=operator.methodcaller('get_rank_count')) #todo : sort using runs[0].get_rank_count()

			for repetitions in self.runs :
				for run in repetitions :
					job_key = machine.launch_scripts_dir + "/" + machine.program.split('.')[0] + "_"
					if self.agregate_runs == True :
						job_key = job_key + str(run.node_count).zfill(3) + ".sh"
					else :
						job_key = job_key + run.get_log_dir() + ".sh"
					
					if job_key in self.jobs :
						self.jobs[job_key].add_run(run)
					else :
						self.jobs[job_key] = DLB_Bench_job_script(job_key)
						self.jobs[job_key].add_run(run)
						print ("new job : " + job_key)
					if run.ignored :
						print ("new run ignored (logs dir already exist): " + run.get_command_line())
					else :
						print ("new run : " + run.get_command_line())

	def clean_scripts_dir(self) :
		if os.path.isdir(machine.launch_scripts_dir) :
			shutil.rmtree(machine.launch_scripts_dir)
		os.mkdir(machine.launch_scripts_dir)

	def make_scripts(self):
		print ("\n################# generating scripts ####################\n")
		for k in self.jobs :
			self.jobs[k].print_report()
			self.jobs[k].write_script()
	
	def submit_jobs(self) :
		print ("\n################### submitting jobs #####################\n")
		for k in self.jobs :
			if os.path.exists(self.jobs[k].path_to_script) :
				machine.submit_job(self.jobs[k])
			
	def wait_for_job_termination(self) :
		print ("\n############## waiting for job completion ###############\n")
		machine.wait_for_job_termination()

	def make_response(self) :
		print ("\n######### parsing logs and generating response ##########\n")
		# make response for each run and cat all responses
		responses = []
		flag = True
		for repetitions in self.runs :
			successful_runs = 0
			print ("\n" + repetitions[0].request)
			efficiencies = []
			for run in repetitions : # requires at least one successful run
				response = run.make_response()
				if response != False :
					successful_runs = successful_runs + 1
					efficiencies.append(response)
					print ( "  " + run.get_log_dir_path() + "\n" + 
							"    " + str(response))
			if successful_runs == 0 :
				flag = False
			else :
				response_path = repetitions[0].logs_root_path + "/res_" + repetitions[0].request.replace(" ","_")
				median = statistics.median(efficiencies)
				with open(response_path, "w") as response_file :
					response_file.write(repetitions[0].request + " " + str(median) + "\n")
				print (str(median))
				responses.append(response_path)
		cmd = "cat "
		if len(responses) == 0 : return
		for response in responses :
			cmd = cmd + response + " "
		if flag == False :
			self.response_filename = self.response_filename + ".part"
			print ("All runs did not reached completion but terminated. A partial response will be written in " + self.response_filename)

		cmd = cmd + " | sort > " + self.response_filename
		os.system(cmd)
		
	
	# variables shared by all objects
	def __init__(self,requested_filename = None, response_filename = None):
		self.repeat_runs = 3
		# variable belonging to instance of the class
		if requested_filename != None :
			self.requested_filename = requested_filename
		else :
			self.requested_filename = machine.DLB_HOME + "/tests/ask/requested.dat"
		if response_filename != None :
				self.response_filename = response_filename
		else :
			self.response_filename = machine.DLB_HOME + "/tests/ask/response.dat"

		self.agregate_runs = machine.agregate_runs
		
		# jobs dict
		# if agregate_runs is True, indexed by the number of nodes required for each set of runs requirering the same number of compute nodes
		# else, one entry for each run, indexed by the log directory name
		self.jobs = {}
		

	def run(self):
		self.clean_scripts_dir()
		self.parse_request()
		self.clean_scripts_dir()
		self.make_scripts()
		self.submit_jobs()
		self.wait_for_job_termination()
		self.make_response()
		
# default machine is anselm
# replace machine with some other implementation of machine_dependant_stuff to handle script generation for other systems
machine = machine_dependant_stuff()

C = DLB_Bench_ask_request_runner()

if __name__ == '__main__':
	C.run()

