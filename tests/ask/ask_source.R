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

#!/usr/bin/env Rscript
# A simple source module
# Parse the module arguments
args <- commandArgs(trailingOnly = T)
if (length(args) != 3) {
    stop("Usage: source <configuration.conf> <requested_file> <output_file>")
}

# marenostrum
ssh_address <- "mn3.bsc.es"
ssh_user <- "pr1e8a11"
ssh_id <- "~/.ssh/id_rsa"
# anselm
#~ ssh_id <- "~/.ssh/anselm_credentials/id_rsa"
#~ ssh_user <- "cfontenaille"
#~ ssh_address <- "anselm.it4i.cz"

distant_requested <- "~/DLB_Library/tests/ask/requested.dat"
distant_response <- "~/DLB_Library/tests/ask/response.dat"
distant_script <- "~/DLB_Library/tests/ask/run_requested.sh"

scp_push <- function (src,dest,id=ssh_id,user=ssh_user,address=ssh_address) {
	sys_command <- paste("scp -i ", id, " ", src, " ", user, "@", address, ":", dest,sep="")
	print(sys_command)
	return (system(sys_command))
}
scp_pull <- function (src,dest,id=ssh_id,user=ssh_user,address=ssh_address) {
	sys_command <- paste("scp -i ", id, " ", user, "@", address, ":", src, " ", dest,sep="")
	print(sys_command)
	return (system(sys_command))
}
ssh <- function (command,id=ssh_id,user=ssh_user,address=ssh_address) {
	sys_command <- paste("echo \"", command, "\" | ssh -i ", id, " ", user, "@", address,sep="")
	print(sys_command)
	return (system(sys_command,intern=T))
}
ssh_nowait <- function (command,id=ssh_id,user=ssh_user,address=ssh_address) {
	sys_command <- paste("echo \"", command, "\" | ssh -i ", id, " ", user, "@", address,sep="")
	print(sys_command)
	return (system(sys_command,wait=F))
}

# send requested file to cluster
scp_push(args[2],distant_requested)

# clean any previous output file and get the "no such file response"
ssh(paste("rm ", distant_response))
no_file_response <- ssh(paste("ls ",distant_response))

# launch distant job launcher script
ssh_nowait(distant_script)

# poll for result file to spawn every minute (or maybe 5)
test_response = ssh(paste("ls " , distant_response))
while (identical(test_response, no_file_response)){
	system(paste( "sleep ", 300))
	test_response = ssh(paste("ls " , distant_response))
}

# pull back result file
scp_pull(distant_response,args[3])

# check output file correctness ?


