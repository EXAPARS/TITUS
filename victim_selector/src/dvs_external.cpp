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

#include "dvs_internal.hpp"
#include <cstdio>
#include <unistd.h>

// *********************************************************************
// ********************** DVS WRAPPERS TO PIMPL ************************
// *********************************************************************

DVS_Context * DVS::get_context() {
	return new DVS_Context(DVS_impl::get_context());
}
void DVS::set_context(DVS_Context * arg) {
	DVS_impl::set_context(arg->m_impl);
}
DVS_RANK_TYPE DVS::select_target_rank() {
	DVS_RANK_TYPE r = DVS_impl::select_target_rank();
	return r;
}

DVS_RANK_TYPE DVS::select_next_hop_to(DVS_RANK_TYPE dest){
	DVS_RANK_TYPE r = DVS_impl::select_next_hop_to(dest);
	return r;
}

// *********************************************************************
// ****************** DVS CONTEXT WRAPPERS TO PIMPL ********************
// *********************************************************************

DVS_Context::DVS_Context(DLB_Context * dlb_ctx, DVS::DVS_MODE mode) {
	gaspi_rank_t rank; gaspi_proc_rank(&rank);
	gaspi_rank_t nb_ranks; gaspi_proc_num(&nb_ranks);
	if (mode == DVS::DVS_MODE_SMALL_WORLD && nb_ranks < DVS_SW_SMALLEST_SIZE){
		mode = DVS::DVS_MODE_RANDOM;
		if (rank == 0) TITUS_DBG << "Warning : DVS_Context::DVS_Context : small world victim selection mode falling back to random victim selection mode :"
				<< "nb_ranks(=" << nb_ranks << ") < DVS_SW_SMALLEST_SIZE(=" << DVS_SW_SMALLEST_SIZE << ")" << std::endl; //TITUS_DBG.flush();
	}
	switch(mode) {
		case DVS::DVS_MODE_RANDOM			: m_impl = new  DVS_Context_impl_random(dlb_ctx->m_impl);			break;
		
		case DVS::DVS_MODE_SMALL_WORLD		: m_impl = new  DVS_Context_impl_small_world(dlb_ctx->m_impl); 		break;
		
		default : m_impl = new  DVS_Context_impl(dlb_ctx->m_impl); break;
	}
}


DVS_Context::~DVS_Context() {
	delete m_impl;
}
DVS::DVS_MODE DVS_Context::get_mode(void) {
	return m_impl->get_mode();
}
