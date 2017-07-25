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


#ifndef __DVS_INTERNAL_H__
#define __DVS_INTERNAL_H__

#include <DVS.hpp>

typedef DVS_RANK_TYPE (* select_target_rank_t) (DVS_Context_impl*);
typedef DVS_RANK_TYPE (* select_next_hop_to_t) (DVS_Context_impl*,DVS_RANK_TYPE);

#include "dvs_context.hpp"
#include <cassert>


class DVS_impl{
public :
	static DVS_Context_impl * get_context() {
		return context;
	}
	static void set_context(DVS_Context_impl * arg) {
		assert(arg != nullptr);
		context = arg;
		fptr_select_target_rank = context->get_victim_selection_func();
		fptr_select_next_hop_to = context->select_next_hop_func();
	}
	
	static DVS_RANK_TYPE select_target_rank() {
		//return fptr_select_target_rank(context); // with_vtable
		return context->select_target_rank(); // without vtable
	}
	static DVS_RANK_TYPE select_next_hop_to(DVS_RANK_TYPE dest){
		return context->select_next_hop_to(dest);
	}
private :
	static select_target_rank_t fptr_select_target_rank;
	static select_next_hop_to_t fptr_select_next_hop_to;

	static DVS_Context_impl * context;
	
	friend class DVS;
};

#endif //__DVS_INTERNAL_H__
