#
# This file is part of the TITUS software.
# https://github.com/exapars/TITUS
# Copyright (c) 2015-2016 University of Versailles UVSQ
#
# TITUS  is a free software: you can redistribute it and/or modify  
# it under the terms of the GNU Lesser General Public License as   
# published by the Free Software Foundation, version 3.
#
# TITUS is distributed in the hope that it will be useful, but 
# WITHOUT ANY WARRANTY; without even the implied warranty of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#

#!/usr/bin/python

#
# Parses logs and bundles everything into a single data file for further processing.
# format of output data file is <runId> : <nprocs> : <key> : <value|valuelist>
# all process-related values (those taken directly from log files) are represented using a value list of length nbprocs
# others contain a sole value
#

import os
import subprocess
import operator
import shutil
import sys
import run_requested

machine = run_requested.DLB_Bench_job_runner.machine

def my_print_runs( runs, out = sys.stdout) :
	i = 0
	for run in runs :
		i = i+1
		for key,value in run.iteritems() : 
			out.write(str(i) + " : " + run["proc_count"] + " : " + key + " : " + str(value) + "\n")
	return


if len(sys.argv) < 2 :
	log_root = machine.logs_dir
	print("no first argument, logs folder defaults to " + log_root)
else :
	log_root = sys.argv[1]


def get_paths_to_traces_files(logdir_path):
	paths_to_files = []
	for filename in os.listdir(logdir_path) :
		if filename.startswith("dbg_out_") :
			paths_to_files.append(logdir_path + "/" + filename)
	return paths_to_files



def add_entry_to_traces(traces_dict, date, owner, first_task_id, nb_results, rank_emit, rank_recv, entry_type):
	if (not owner in traces_dict) :
		traces_dict [owner] = {}
	if (not first_task_id in traces_dict[owner]) :
		traces_dict[owner][first_task_id] = {}
	if (not entry_type in traces_dict[owner][first_task_id]) :
		traces_dict[owner][first_task_id][entry_type] = []
	traces_dict[owner][first_task_id][entry_type].append( (date, rank_emit, rank_recv) )


def parse_traces(traces_dict, logfiles_paths) :
# dict of owners -> dict of fist_task_ids -> dict of entry types -> list of tuples (date, rank_emit, rank_recv)
	results_production = 0
	results_returned = 0
	results_routing_from_segment = 0
	print ("\r" + "results_production=" + str(results_production) + ", results_returned=" + str(results_returned) + ", results_routing_from_segment=" + str(results_routing_from_segment))
	for logfile_path in logfiles_paths :
		with open(logfile_path,"r") as trace_file :
			#print "Opened " + logfile_path
			for line in trace_file :
				tmp = line.split(":",1)
				if (len(tmp) < 2) : continue
				tmp = tmp[1].strip()
				try : date = float(line.split()[0])
				except ValueError : continue
				if   tmp.startswith("MetadataTmp::push_tmp_results") :
# 00012.71074670 [rank 23] : MetadataTmp::push_tmp_results : pushing results first_task_id=157826, nb_result=13, data_size=52, owner=31
					tmp = line.split("=")
					first_task_id = int(tmp[1].split(",")[0].strip()) # after the 1st '='
					nb_results = int(tmp[2].split(",")[0].strip()) # after the 2nd '='
					data_size = int(tmp[3].split(",")[0].strip()) # after the 3rd '='
					owner = int(tmp[4].split()[0].strip(" ,")) # after the 4rd '='
					rank_emit = int(line.split("]")[0].split()[-1]) # the space separated value right before the ']'
					rank_recv = rank_emit
					add_entry_to_traces(traces_dict, date, owner, first_task_id, nb_results, rank_emit, rank_recv,  "Results production")
					results_production = results_production + 1;
					
				elif tmp.startswith("BufferedSegmentMetadata::try_push_local_transiting_results >") :
# 00013.35301511 [rank 23] : BufferedSegmentMetadata::try_push_local_transiting_results > successfuly reserved memory for elt[first_task_id=168997, nb_result=1, data_size=4, owner=32] to segment 1 on rank 32 as elt 4 @offset=320
					tmp = line.split("=")
					first_task_id = int(tmp[1].split(",")[0].strip())
					nb_results = int(tmp[2].split(",")[0].strip())
					data_size = int(tmp[3].split(",")[0].strip())
					owner = int(tmp[4].split("]")[0].strip())
					rank_emit = int(line.split("]")[0].split()[2])
					rank_recv = int(line.split(" on rank ")[1].split(" ")[0].strip())
					add_entry_to_traces(traces_dict, date, owner, first_task_id, nb_results, rank_emit, rank_recv,  "Results routing from segment")
					results_routing_from_segment = results_routing_from_segment + 1
					
				elif tmp.startswith("TITUS_DLB_Context_impl::submit_results") :
# 00005.48300303 [rank 23] : TITUS_DLB_Context_impl::submit_results : submitted 554 results, starting with id 1170
					first_task_id = int(line.split("id ")[1].strip())
					nb_results = int(line.split("submitted ")[1].split()[0].strip())
					data_size = 0
					owner = int(line.split("]")[0].split()[2])
					rank_emit = owner
					rank_recv = owner
					add_entry_to_traces(traces_dict, date, owner, first_task_id, nb_results, rank_emit, rank_recv,  "Results returned")
					results_returned = results_returned + 1

				elif tmp.startswith(" try_flush_buffer_elts_packed : pushed elt :") :
# NOT
# 00009.62966835 [rank 123] : try_flush_buffer_elts_packed : pushed elt : first_task_id=112617, nb_result=83, data_size=332, owner=236 @target=237
					tmp = line.split("=")
					first_task_id = int(tmp[1].split(",")[0].strip())
					nb_results = int(tmp[2].split(",")[0].strip())
					data_size = int(tmp[3].split(",")[0].strip())
					owner = int(tmp[4].split().strip())
					rank_emit = int(line.split("]")[0].split()[2])
					rank_recv = int(tmp[5].strip())
					add_entry_to_traces(traces_dict, date, owner, first_task_id, nb_results, rank_emit, rank_recv,  "Results routing from segment")
					results_routing_from_segment = results_routing_from_segment + 1 

# TODO : ?
# 00009.93799075 [rank 222] : BufferedSegmentMetadata::try_push_vector_elt_to : reserved 340 bytes at rank 253 for 5 / 5elements, starting at index 181, @offset 18268
					



### !! OLD !! ###
				#~ elif tmp.startswith("BufferedSegmentMetadata::try_push_buffer_elt_to") :
#~ # 00012.84515972 [rank 23] : BufferedSegmentMetadata::try_push_buffer_elt_to : reserved elt 57 on rank 32 : first_task_id=141516, nb_result=51, data_size=204, owner=32
					#~ tmp = line.split("=")
					#~ first_task_id = int(tmp[1].split(",")[0].strip())
					#~ nb_results = int(tmp[2].split(",")[0].strip())
					#~ data_size = int(tmp[3].split(",")[0].strip())
					#~ owner = int(tmp[4].strip())
					#~ rank_emit = int(line.split("]")[0].split()[2])
					#~ rank_recv = int(line.split(" on rank ")[1].split(":")[0].strip())
					#~ add_entry_to_traces(traces_dict, date, owner, first_task_id, nb_results, rank_emit, rank_recv,  "Results routing from segment")
				
		print ("\r" + "results_production=" + str(results_production) + ", results_returned=" + str(results_returned) + ", results_routing_from_segment=" + str(results_routing_from_segment))
	print("\n")
# TODO :
# 00013.34885413 [rank 23] : TITUS_DLB_impl::work_stealing > 14 task stolen from rank 20 end_task_id=169454

fields_count = 0

def parse_run_logs(log_root, logdir_name, parse_debug_traces = False) :
	global fields_count

	logdir_path = log_root + "/" + logdir_name

	# found new run
	i = 0
	run = {}
	run["logdir_name"] = logdir_name
	run["logdir_path"] = logdir_path
	if os.path.exists(logdir_path+"/request.dat") :
		with open(logdir_path+"/request.dat") as request :
			run["request"]=request.readline()
	else :
		print("error : request.dat not found : " + logdir_path + "/request.dat")
		return False
	run["request"] = run["request"].split()
	run["log_nodes"] = run["request"][0]
	run["proc_count"] = 2**int(run["log_nodes"]) * machine.cores_per_node
	run["tasks_per_proc"] = run["request"][1]
	run["time_per_task"] = run["request"][2]
	run["pb_size_per_proc"] = run["request"][3]
	run["segmentSizeRatio"] = run["request"][4]
	
	run["failed"] = False

	logfile_name = "logs_" + str(0).zfill(4) + ".out"
	logfile_path = logdir_path + "/" + logfile_name
	fields_count_in_run = 0

	if os.path.exists(logfile_path) == False :#or os.path.exists(logdir_path + "/result.txt") == False :
		print("WARNING : ignored failed log dir : " + logdir_path + "\n")
		run["failed"] = True
		return False

	with open(logfile_path) as log_file_example :
		for line in log_file_example.readlines() :
			fieldname = line.split(":")[0].strip(" ")
			if fieldname != "" :
				run[fieldname] = []
				fields_count_in_run = fields_count_in_run + 1

	if (fields_count == 0) : 
		if fields_count_in_run == 0 :
			print ("error : no fields found while parsing initial field names in " + logdir_path)
			exit(0)
		else :
			fields_count = fields_count_in_run;
			print (str(fields_count_in_run) + " runs found, field found : ")
			for k in run.keys() :
				print(str(k))
	else : 
		if (fields_count_in_run != fields_count) :
			print("error parsing initial field names for run " + logdir_path + " : \nfound " + str(len(run)) + " fields, expected" + str(fields_count))
			exit(0)
	
	# ignore failed runs
	if (run["failed"] == True) : return False
	for j in range(0,run["proc_count"]) :
		logfile_name="logs_" + str(j).zfill(4) + ".out"
		logfile_path = logdir_path + "/" + logfile_name
		i = i + 1
		if os.path.exists(logfile_path) == False :
			print("WARNING : cannot find " + logfile_name + " (proc_count=" + str(run["proc_count"]) + ")\n")
			print("WARNING : ignored failed log dir : " + logdir_path + "\n")
			run["failed"] = True
			break
		with open(logfile_path) as log :
			#print("opened " + logfile_path)
			line_count = 0;
			for line in log.readlines() :
				line_count = line_count + 1
				fieldname = line.split(":")[0].strip(" ")
				if fieldname in run.keys() :
					run[fieldname].append(int(line.replace(fieldname + " : ","")))
				else :
					print ("error unknown field name : " + fieldname + " in " + logfile_path)
			if (line_count != fields_count) :
				print("error parsing " + logfile_path + " : \nfound " + str(line_count) + " fields, expected" + str(fields_count))
				exit(0)
	if parse_debug_traces == True :
		run["traces"] = {}
		parse_traces(run["traces"], get_paths_to_traces_files(logdir_path))
	
	return run;


if __name__ == '__main__':
	for logdir_name in os.listdir(log_root):
		logdir_path = log_root + "/" + logdir_name
		if logdir_name.startswith("p") and os.path.isdir(logdir_path) :
			print (logdir_name)
			dump_file_path = logdir_path + "/report.pydump";
			if os.path.exists(dump_file_path) : 
				print ("  " + dump_file_path + " already extists. skipping")
				continue
			run = parse_run_logs(log_root, logdir_name)
			if (run == False) : continue
			with open(dump_file_path,"w") as out :
				out.write(str(run))
