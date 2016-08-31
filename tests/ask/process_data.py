#!/usr/bin/env python3

#
# Processes bundled data from file (genrated by make_full_report.py) as a simple evaluable dump of a python data structure
# outputs a table file usable by R and ggplot
#

import os
import subprocess
import operator
import shutil
import sys
import ast
import make_full_report

machine = make_full_report.machine

if len(sys.argv) < 2 :
	log_root = machine.logs_dir
	print("no first argument, logs folder defaults to " + log_root)
else :
	log_root = sys.argv[1]

if len(sys.argv) < 3 :
	print("no second argument, output file defaults to plot_data.dat")
	output_filename = "plot_data.dat"
else :
	output_filename = sys.argv[2]

# define fields to print for parsing with R
fields = ("proc_count","run_time","tasks_per_proc","time_per_task","pb_size_per_proc","segmentSizeRatio","efficiency","total_theft_attemps","total_theft_hits","total_results_pushed_count","total_time_spent_solving_tasks","total_return_results_time","max_return_results_time","first_decile_return_result_time","total_time_spent_stealing","avg_time_to_return_results", "total_hops")

def get_plot_data(run) :
	run["proc_count"] = 2**int(run["log_nodes"]) * machine.cores_per_node
	run["run_time"] = float(max(run["parallel_work_session_time"])) / 1e9
	run["total_time_spent_solving_tasks"] = float(sum(run["time_spent_solving_tasks"]))/1e9
	run["speedup"] = run["total_time_spent_solving_tasks"] / run["run_time"]
	run["efficiency"] = run["speedup"] / float(run["proc_count"])
	run["total_return_results_time"] = float(sum(run["time_spent_returning_results"]))/1e9
	run["max_return_results_time"] = float(max(run["time_spent_returning_results"]))/1e9
	run["first_decile_return_result_time"] = sorted(run["time_spent_returning_results"])[-len(run["time_spent_returning_results"]) // 10]/1e9
	print("first_decile_return_result_time = " + str(run["first_decile_return_result_time"]))
	print("         avg_return_result_time = " + str(run["total_return_results_time"] / len(run["time_spent_returning_results"]) ))
	with open(run["logdir_path"]+"/histogram", "w") as histogram_file:
		rankno = 0;
		histogram_file.write("rankno time_spent_returning_results")
		for e in sorted(run["time_spent_returning_results"]) : 
			histogram_file.write(str(rankno) + str(e) + "\n")
			rankno = rankno + 1
	run["total_time_spent_stealing"] = float(sum(run["time_spent_stealing"]))/1e9
	run["total_results_pushed_count"] = sum(run["results_pushed_count"])
	run["total_theft_attemps"] = sum(run["try_theft_count"])
	run["total_theft_hits"] = sum(run["hit_count"])
	run["total_time_to_return_results"] = 0
	run["total_returned_buffer_elts"] = 0
	run["total_hops"] = 0
	
	if "traces" in run : 
		for owner in run["traces"] :
			run["traces"][owner]["returned_buffer_elts"] = len(run["traces"][owner])
			run["total_returned_buffer_elts"] = run["total_returned_buffer_elts"] + run["traces"][owner]["returned_buffer_elts"]
			run["traces"][owner]["sum_time_to_return_results"] = 0
			for results_id in run["traces"][owner] :
				if not(type(results_id) is int) : continue
				traces = run["traces"][owner][results_id]
				#print "traces " + str(results_id) + " = " + str(traces)
				#print "traces[\"Results returned\"][0][0] = " + str(traces["Results returned"][0][0])
				#print "traces[\"Results production\"][0][0] = " + str(traces["Results production"][0][0])
				try : traces["time_to_return"] = traces["Results returned"][0][0] - traces["Results production"][0][0]
				except KeyError : 
					print("ERROR : invalid traces for results " + str(results_id) + " from rank " + str(owner))
					print("  > While parsing " + " ".join(run["request"]))
					print("  > " + run["logdir_name"])
					print("  > " + str(traces))
					print("  > run informations : proc_count = " + str(run["proc_count"]) + ", tasks_per_proc=" + str(run["tasks_per_proc"]) + ", time_per_task=" + str(run["time_per_task"]) + ", pb_size_per_proc=" + str(run["pb_size_per_proc"]) + ", segmentSizeRatio=" + str(run["segmentSizeRatio"]) + ", efficiency=" + str(run["efficiency"]))
				if "Results routing from segment" in traces :
					run["total_hops"] = run["total_hops"] + len(traces["Results routing from segment"])
				run["traces"][owner]["sum_time_to_return_results"] = run["traces"][owner]["sum_time_to_return_results"] + traces["time_to_return"]
			run["total_time_to_return_results"] = run["total_time_to_return_results"] + run["traces"][owner]["sum_time_to_return_results"]
	if run["total_returned_buffer_elts"] == 0 :
		run["avg_time_to_return_results"] = 0
	else : 
		run["avg_time_to_return_results"] = float(run["total_time_to_return_results"]) / float(run["total_returned_buffer_elts"])

	#print table
	r = ""
	for field in fields :
		r = r + str(run[field]) + " "
	return r

if __name__ == '__main__':
	with open(output_filename,"w") as out :
		# print header
		for field in fields : out.write(field + " ")

		for logdir_name in os.listdir(log_root):
			logdir_path = log_root + "/" + logdir_name
			if (os.path.isdir(logdir_path) ):
				if os.path.exists(logdir_path + "/logs_0000.out") == False : continue
				print (logdir_name)
				run = make_full_report.parse_run_logs(log_root, logdir_name)
				if run == False : continue
				out.write("\n" + get_plot_data(run))


