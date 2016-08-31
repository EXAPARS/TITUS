/* 
* This file is part of the TITUS software.
* https://github.com/exapars/
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


//====================================================================================
//
//  AUTHOR  :   UEMURA Seijilo, FONTNAILLE Clément, PETIT Eric 
//  FILE	: dvs_context.hpp
//  CONTENT	:
//
//====================================================================================


#ifndef __DVS_CONTEXT_H__
#define __DVS_CONTEXT_H__

#include <dlb_gaspi_tools.hpp>
#include <neighboring_generator.hpp>

// root class : random no connect
class DVS_Context_impl{
public :
	DVS_Context_impl(DLB_INTERNAL_CONTEXT * dlb_ctx);
	virtual ~DVS_Context_impl() {}
	
	
	
	virtual DVS_RANK_TYPE select_target_rank();

	virtual DVS::DVS_MODE get_mode() {return DVS::DVS_MODE_RANDOM;}
	virtual select_target_rank_t get_victim_selection_func();
	
	virtual DVS_RANK_TYPE select_next_hop_to(DVS_RANK_TYPE dest){ return dest; }
	virtual select_next_hop_to_t select_next_hop_func();
	
	friend class DVS_impl;
protected :
	DLB_INTERNAL_CONTEXT * dlb_ctx;
// ------------------------------------------------------
// -------------- GENERIC CONTEXT DATA ------------------
// ------------------------------------------------------
};

// root class : random no connect
class DVS_Context_impl_random : public DVS_Context_impl {
public :
	DVS_Context_impl_random(DLB_INTERNAL_CONTEXT * dlb_ctx);
	virtual ~DVS_Context_impl_random() {}
	
	virtual DVS_RANK_TYPE select_target_rank();

	virtual DVS::DVS_MODE get_mode() {return DVS::DVS_MODE_RANDOM;}
	virtual select_target_rank_t get_victim_selection_func();
	
	virtual DVS_RANK_TYPE select_next_hop_to(DVS_RANK_TYPE dest){ return dest; }
	virtual select_next_hop_to_t select_next_hop_func();

	friend class DVS_impl;
};




class DVS_Context_impl_small_world : public DVS_Context_impl {
public :
	DVS_Context_impl_small_world(DLB_INTERNAL_CONTEXT * dlb_ctx, size_t neighbors_count_ = 0);
	virtual ~DVS_Context_impl_small_world() {}


	virtual DVS::DVS_MODE get_mode() {return DVS::DVS_MODE_SMALL_WORLD;}	
	virtual select_target_rank_t get_victim_selection_func();

	virtual DVS_RANK_TYPE select_next_hop_to(DVS_RANK_TYPE dest);
	virtual select_next_hop_to_t select_next_hop_func();
	
	virtual DVS_RANK_TYPE select_target_rank();
	
private :
	size_t neighbors_count;
	size_t degree;
	d1_kleinberg_neighboring_generator neighbors;
	void init_neighbors();
};

#endif //__DVS_CONTEXT_H__
