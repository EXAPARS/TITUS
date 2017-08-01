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

#include <iostream>
#include <TITUS_DBG.hpp>
#include <TITUS_Logger.hpp>

// ********************************************************
// ******************** C BINDINGS IMPL *******************
// ********************************************************

void * c_TITUS_Logger_new_logger(){
	return ((void*) new TITUS_Logger());
}

void c_TITUS_Logger_print_current_session(void * logger, const char * filename, const char * prefix){
	if (prefix == nullptr) prefix = "";

	if (filename != nullptr && *filename != '\0') {
		std::ofstream out(filename);
		((TITUS_Logger *)logger)->print_current_session(out);
	}
	else{
		((TITUS_Logger *)logger)->print_current_session(std::cout, prefix);
	}

}

void c_TITUS_Logger_print_all_sessions(void * logger, const char * filename, const char * prefix){
	if (prefix == nullptr) prefix = "";

	if (filename != nullptr && *filename != '\0') {
		std::ofstream out(filename);
		((TITUS_Logger *)logger)->print_all_sessions(out);
	}
	else{
		((TITUS_Logger *)logger)->print_all_sessions(std::cout, prefix);
	}

}

void c_TITUS_Logger_reset_all_entries(void * logger){
	((TITUS_Logger *)logger)->reset_all_entries();
}

