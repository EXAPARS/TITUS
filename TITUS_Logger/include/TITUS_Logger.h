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


#ifndef __c_TITUS_LOGGER__H__
#define __c_TITUS_LOGGER__H__

extern "C"
{
	void * c_TITUS_Logger_new_logger();
	void c_TITUS_Logger_print_current_session(void * logger, const char * filename, const char * prefix);
	void c_TITUS_Logger_print_all_sessions(void * logger, const char * filename, const char * prefix);

	void c_TITUS_Logger_reset_all_entries(void * logger);
}

#endif //__c_TITUS_LOGGER__H__
