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


#include <cstdlib>
#include <sstream>
#include "dvs_internal.hpp"
#include "../../src/TITUS_DLB_context.hpp"


// static DVS_impl fields init

select_target_rank_t DVS_impl::fptr_select_target_rank = nullptr;
select_next_hop_to_t DVS_impl::fptr_select_next_hop_to = nullptr;
DVS_Context_impl * DVS_impl::context = nullptr;



// *********************************************************************
// *********************** DVS_Context_impl ****************************
// *********************************************************************	
	
DVS_Context_impl::DVS_Context_impl(TITUS_DLB_INTERNAL_CONTEXT * TITUS_DLB_ctx):
	TITUS_DLB_ctx(TITUS_DLB_ctx)
{
}

static DVS_RANK_TYPE base_victim_selection(DVS_Context_impl *context) {return context->select_target_rank();}

select_target_rank_t DVS_Context_impl::get_victim_selection_func() {
		return &base_victim_selection;
}

DVS_RANK_TYPE DVS_Context_impl::select_target_rank() {
	DVS_RANK_TYPE a = (DVS_RANK_TYPE)rand() % (TITUS_DLB_ctx->get_nb_ranks() -1);
    if ( a >= TITUS_DLB_ctx->get_rank() ) ++a ;
    return (a);
}

static DVS_RANK_TYPE base_select_next_hop_to(DVS_Context_impl * context, DVS_RANK_TYPE dest){
	return ((DVS_Context_impl*)context)->select_next_hop_to(dest);
}
select_next_hop_to_t DVS_Context_impl::select_next_hop_func(){
	return &base_select_next_hop_to;
}




// *********************************************************************
// ******************** DVS_Context_impl_random ************************
// *********************************************************************
	
	
DVS_Context_impl_random::DVS_Context_impl_random(TITUS_DLB_INTERNAL_CONTEXT * TITUS_DLB_ctx):
	DVS_Context_impl(TITUS_DLB_ctx)
{
	TITUS_DBG << "new DVS_Context_impl_random" << std::endl; //TITUS_DBG.flush();

	// init all communication pairs and set register segments to all
	for (DVS_RANK_TYPE i = 0 ; i < TITUS_DLB_ctx->get_nb_ranks(); i++) TITUS_DLB_ctx->register_to_rank(i); //! BAD
}

static DVS_RANK_TYPE random_victim_selection(DVS_Context_impl *context) {return ((DVS_Context_impl_random*)context)->select_target_rank();}

select_target_rank_t DVS_Context_impl_random::get_victim_selection_func() {
		return &random_victim_selection;
}

DVS_RANK_TYPE DVS_Context_impl_random::select_target_rank() {
	DVS_RANK_TYPE a = (DVS_RANK_TYPE)rand() % (TITUS_DLB_ctx->get_nb_ranks() -1);
    if ( a >= TITUS_DLB_ctx->get_rank() ) ++a ;
    return (a);
}

static DVS_RANK_TYPE random_select_next_hop(DVS_Context_impl * context, DVS_RANK_TYPE dest) {return ((DVS_Context_impl_random *)context)->select_next_hop_to(dest);}
select_next_hop_to_t DVS_Context_impl_random::select_next_hop_func(){
	return &random_select_next_hop;
}


// *********************************************************************
// ****************** DVS_Context_impl_small_world *********************
// *********************************************************************

#include <stdint.h>
static inline uint32_t log2(const uint32_t x) {
  uint32_t y;
  asm ( "\tbsr %1, %0\n"
      : "=r"(y)
      : "r" (x)
  );
  return y;
}

DVS_Context_impl_small_world::DVS_Context_impl_small_world(TITUS_DLB_INTERNAL_CONTEXT * TITUS_DLB_ctx, size_t neighbors_count_):
	DVS_Context_impl(TITUS_DLB_ctx),
	neighbors_count(TITUS_DLB_ctx->get_nb_ranks()),
	degree(neighbors_count_ != (size_t)-1 ? neighbors_count_ : 2*log2( (uint32_t)TITUS_DLB_ctx->get_nb_ranks()-1 )),
	//~ degree(neighbors_count_ != (size_t)-1 ? neighbors_count_ : 45),
	neighbors(neighbors_count, degree)
{
	if (TITUS_DLB_ctx->get_rank() == 0){
		TITUS_DBG << "new DVS_Context_impl_small_world (average degree = " << degree*2 << ")" << std::endl; //TITUS_DBG.flush();
		std::stringstream filename("");
		filename << "small_world_" << TITUS_DLB_ctx->get_rank() << ".txt";
		std::ostream * out = new std::ofstream(filename.str().c_str());
		*out << neighbors;
		delete out;
	}
	//! TODO : cleanup after debug
//	neighbors.check_graph();
	
	for (auto it = neighbors.begin_of(TITUS_DLB_ctx->get_rank()); it != neighbors.end_of(TITUS_DLB_ctx->get_rank()) ; it ++)
	TITUS_DLB_ctx->register_to_rank(*it); //! BAD move to TITUS_DLB context and add public getters for n_neighbors and index based neighbors access

}


static DVS_RANK_TYPE small_world_victim_selection(DVS_Context_impl * context){
	return ((DVS_Context_impl_small_world*)context)->select_target_rank();
}
static DVS_RANK_TYPE small_world_select_next_hop(DVS_Context_impl * context, DVS_RANK_TYPE dest){
	return ((DVS_Context_impl_small_world *)context)->select_next_hop_to(dest);
}
select_target_rank_t DVS_Context_impl_small_world::get_victim_selection_func(){
	return &small_world_victim_selection;
}
select_next_hop_to_t DVS_Context_impl_small_world::select_next_hop_func(){
	return &small_world_select_next_hop;
}


DVS_RANK_TYPE DVS_Context_impl_small_world::select_target_rank() {
	int your_rand = rand();
	DVS_RANK_TYPE r = neighbors.get_neighbor(TITUS_DLB_ctx->get_rank(),your_rand % neighbors.get_neighbors_count(TITUS_DLB_ctx->get_rank()));
//	TITUS_DBG << "DVS_Context_impl_small_world::select_target_rank() : selected target rank " << r << std::endl; //TITUS_DBG.flush();
	assert(neighbors.are_neighbors(TITUS_DLB_ctx->get_rank(), r));
	return r;
}

DVS_RANK_TYPE DVS_Context_impl_small_world::select_next_hop_to(DVS_RANK_TYPE dest) {
	DVS_RANK_TYPE r = neighbors.get_closest_neighbor(TITUS_DLB_ctx->get_rank(), dest);
	//TITUS_DBG << "DVS_Context_impl_small_world::select_next_hop_to : asked a route to rank " << dest << ", returned " << r << std::endl; //TITUS_DBG.flush();
	return r;
}


// *********************************************************************
// ***************** DVS_Context_impl_weighted_tree ********************
// *********************************************************************

DVS_Context_impl_weighted_tree::DVS_Context_impl_weighted_tree(TITUS_DLB_INTERNAL_CONTEXT * TITUS_DLB_ctx):
	DVS_Context_impl(TITUS_DLB_ctx)
{
	//! TODO
}


static DVS_RANK_TYPE weighted_tree_victim_selection(DVS_Context_impl * context) {return ((DVS_Context_impl_weighted_tree*)context)->select_target_rank();}

select_target_rank_t DVS_Context_impl_weighted_tree::get_victim_selection_func() {
	return &weighted_tree_victim_selection;
}

DVS_RANK_TYPE DVS_Context_impl_weighted_tree::select_target_rank() {
	//! TODO
	return TITUS_DLB_ctx->get_rank();
}

static DVS_RANK_TYPE weighted_tree_select_next_hop(DVS_Context_impl * context, DVS_RANK_TYPE dest) {return ((DVS_Context_impl_weighted_tree *)context)->select_next_hop_to(dest);}
select_next_hop_to_t DVS_Context_impl_weighted_tree::select_next_hop_func(){
	return &weighted_tree_select_next_hop;
}
