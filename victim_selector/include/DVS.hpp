/* 
* This file is part of the TITUS software.
* https://github.com/exapars/TITUS
* Copyright (c) 2015-2016 University of Versailles UVSQ
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


#ifndef __DYNAMIC_VICTIM_SELECTOR_H__
#define __DYNAMIC_VICTIM_SELECTOR_H__

class DVS_Context;
class DVS_impl;
class DVS_Context_impl;

#ifndef DLB_EXTERNAL_LOGGER
#include <DLB.hpp>
#define DLB_EXTERNAL_LOGGER DLB_Logger
#endif

#ifndef DLB_EXTERNAL_CONTEXT
#include <DLB.hpp>
#define DLB_EXTERNAL_CONTEXT DLB_Context 
#define DLB_INTERNAL_CONTEXT DLB_Context_impl 
#endif

#ifndef DVS_RANK_TYPE
#include <GASPI.h>
#define DVS_RANK_TYPE gaspi_rank_t
#endif



class DVS{
public :
	enum DVS_MODE{	DVS_MODE_RANDOM,
					DVS_MODE_SMALL_WORLD
	};

	static DVS_Context * get_context();
	static void set_context(DVS_Context * arg);
	
	static DVS_RANK_TYPE select_target_rank();
	static DVS_RANK_TYPE select_next_hop_to(DVS_RANK_TYPE dest);

};
#define DEFAULT_DVS_MODE DVS::DVS_MODE_SMALL_WORLD
#define DVS_SW_SMALLEST_SIZE 8



class DVS_Context{
	DVS_Context_impl * m_impl;
	friend class DVS;
public :
	DVS_Context(DVS_Context_impl * arg):m_impl(arg) {};
	
	DVS_Context(DLB_EXTERNAL_CONTEXT * dlb_ctx, DVS::DVS_MODE mode = DEFAULT_DVS_MODE);

	//! TODO : init from config file
	//DVS_Context(DVS_RANK_TYPE self, DVS_RANK_TYPE n_rank, const char * config_filename, DVS_RANK_TYPE zero_rank = 0);
	
	//! TODO : context clone
	//DVS_Context(const DVS_Context & arg);

	~DVS_Context();
	DVS::DVS_MODE get_mode();
		
};
#endif // __DYNAMIC_VICTIM_SELECTOR_H__
