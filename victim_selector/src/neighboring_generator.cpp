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
//  FILE	: neighboring_generator.cpp
//  CONTENT	:
//
//====================================================================================


#include <neighboring_generator.hpp>
#include "TinyMT/tinymt64.h"
#include <stdint.h>
#include <cstdlib>
#include <cmath>

/*
static inline uint32_t log2(const uint32_t x) {
  uint32_t y;
  asm ( "\tbsr %1, %0\n"
      : "=r"(y)
      : "r" (x)
  );
  return y;
}
*/
#define POW2(X) ( 1 << (X) )


d_reg_neighboring_generator::d_reg_neighboring_generator(size_t nb_rank, size_t degree):
	nb_rank(nb_rank), degree(degree),
	available_half_edges(nb_rank * degree)
{
	if (degree >= nb_rank){
		std::cerr << "assert is going to fail : degree = " << degree << " nb_rank = " << nb_rank << std::endl;
		assert(degree < nb_rank);
	}
	half_edges = new DVS_RANK_TYPE[nb_rank * degree];
	neighbors = new DVS_RANK_TYPE[nb_rank * degree];
	// init tables
	for (size_t i=0;i<degree*nb_rank;i++){
		neighbors[i] = (DVS_RANK_TYPE)-1;
		half_edges[i] = i;
	}
}

void d_reg_neighboring_generator::add_all_neighbors_from_tree(){
	for (size_t i=0;i<nb_rank ; i++){
		add_neighbors_of_rank_in_tree(i);
		//std::cout << "added neighbors of rank" << i << std::endl;
		//print_paired_half_edges(std::cout);
		//print(std::cout);
		//std::cout << std::endl;
	}
	
	//rebuild_neighbors_from_half_edges();
}

void d_reg_neighboring_generator::add_random_neighbors(){
	while(available_half_edges > 1){
		add_random_neighbor();
	}
	rebuild_neighbors_from_half_edges();
}

// **********************
// ****** Printers ******
// **********************

void d_reg_neighboring_generator::print(std::ostream & out)const{
	for (size_t i=0;i<nb_rank;i++){
		out << "rank" << i << ":";
		for (size_t j = 0; j<degree; j++){
			out << "[" << neighbors[i*degree+j] << "] ";
		}
		out << std::endl;
	}
}

void d_reg_neighboring_generator::print_paired_half_edges(std::ostream & out)const{
	out << "print_paired_half_edges" << std::endl;
	for (size_t i=0;i< nb_rank * degree - available_half_edges ; i+=2){
		out << "[" << i/2 << "] (" << half_edges[i] << "," << half_edges[i+1] << ")" << std::endl;
	}
	out << std::endl;
}

void d_reg_neighboring_generator::add_random_neighbor(){
	DVS_RANK_TYPE * p_half_edge_a = half_edges + degree * nb_rank - available_half_edges --;
	DVS_RANK_TYPE * p_half_edge_b = p_half_edge_a + (rand() % available_half_edges) + 1;
	swap(p_half_edge_a + 1 , p_half_edge_b);
	available_half_edges --;
}



// ********************************************
// *** methods for building a infixed btree ***
// ********************************************
std::ostream & operator<<(std::ostream & out, const infixed_b_tree_neighbors &arg){
	out << "rank=" << ((int16_t)arg.rank) << "\tparent=" << ((int16_t)arg.parent) << "\tleft_child=" << ((int16_t)arg.left_child) << "\tright_child=" << ((int16_t)arg.right_child);
	return out;
}

void infixed_b_tree_neighbors::print_full_tree(std::ostream & out, DVS_RANK_TYPE n_ranks){
	for (int i=0 ; i< n_ranks ; i++){
		out << infixed_b_tree_neighbors::get_tree_neighbors(i, n_ranks) << std::endl;
	}
}

DVS_RANK_TYPE infixed_b_tree_neighbors::get_rank_of_root(size_t subtree_size, DVS_RANK_TYPE rank0){
	if (subtree_size == 1) return rank0;
	size_t h = log2(((uint64_t)subtree_size)) + 1;
	DVS_RANK_TYPE root = rank0 + POW2(h-2)-1 + std::min(POW2(h-2),((int)subtree_size)-(POW2(h-1)-1));
	return root;
}

infixed_b_tree_neighbors infixed_b_tree_neighbors::get_tree_neighbors(DVS_RANK_TYPE rank, size_t subtree_size, DVS_RANK_TYPE rank0){
	assert(subtree_size != 0);
	
	size_t h = log2(((uint64_t)subtree_size)) + 1;	
	if (h == 1){
		if (rank == rank0){
			infixed_b_tree_neighbors n;
			n.rank = rank;
			n.left_child = n.right_child = n.parent = ((DVS_RANK_TYPE)-1);
			return n;
		}
	}
	if (h == 2){
		if (rank == rank0){
			infixed_b_tree_neighbors n;
			n.rank = rank;
			n.left_child = n.right_child = ((DVS_RANK_TYPE)-1);
			n.parent = rank + 1;
			return n;
		}
		if (rank == rank0 +1){
			infixed_b_tree_neighbors n;
			n.rank = rank;
			n.left_child = rank0;
			if (subtree_size == 3) 	n.right_child = rank + 1;
			else 					n.right_child = ((DVS_RANK_TYPE)-1);
			n.parent = ((DVS_RANK_TYPE)-1);
			return n;
		}
		if (rank == rank0 +2){
			infixed_b_tree_neighbors n;
			n.rank = rank;
			n.left_child = n.right_child = ((DVS_RANK_TYPE)-1);
			n.parent = rank - 1;
			return n;
		}
	}
	
	DVS_RANK_TYPE root = rank0 + POW2(h-2)-1 + std::min(POW2(h-2),((int)subtree_size)-(POW2(h-1)-1));
	
	// rank is in left child subtree
	if (rank < root){
		infixed_b_tree_neighbors n = get_tree_neighbors(rank, root - rank0, rank0);
		if (n.parent == ((DVS_RANK_TYPE)-1)) n.parent = root;
		return n;
	}
	if (rank == root){
		infixed_b_tree_neighbors n;
		n.rank = rank;
		n.left_child =  get_rank_of_root(root - rank0, rank0); // root of left child subtree
		n.right_child = get_rank_of_root(subtree_size - (root - rank0) -1, root + 1); // root of right child subtree
		n.parent = ((DVS_RANK_TYPE)-1); // caller will fill it
		return n;
	}
	// rank is in left child subtree
	//if (rank > root){ // useless if
	infixed_b_tree_neighbors n = get_tree_neighbors(rank, subtree_size - (root - rank0) -1, root +1);
	if (n.parent == ((DVS_RANK_TYPE)-1)) n.parent = root;
	return n;
}
void d_reg_neighboring_generator::add_neighbors_of_rank_in_tree(DVS_RANK_TYPE rank){
	assert (degree >= 3);
	
	infixed_b_tree_neighbors n = infixed_b_tree_neighbors::get_tree_neighbors(rank, nb_rank);
	
	if (n.left_child != ((DVS_RANK_TYPE)-1) )	add_neighboring(rank, n.left_child);
	if (n.left_child != ((DVS_RANK_TYPE)-1) )	add_neighboring(rank, n.right_child);
}
// ************************************
// *** Utils and basic manipulators ***
// ************************************

bool d_reg_neighboring_generator::are_neighbors(DVS_RANK_TYPE rank0, DVS_RANK_TYPE rank1){
	for (size_t i=0;i<degree;i++){
		if (neighbors[rank0 * degree + i] == rank1) return true;
	}
	return false;
}

DVS_RANK_TYPE d_reg_neighboring_generator::find_free_half_edge(DVS_RANK_TYPE rank){
	DVS_RANK_TYPE i;
	for (i=rank * degree;i < (rank + 1) * degree;i++){
		if (neighbors[i] == (DVS_RANK_TYPE)-1)
			return (i);
	}
	assert(i < (rank + 1) * degree);
	return -1;
}

// std::pair a free edge of a with one of b and write this std::pair in neighbors 
void d_reg_neighboring_generator::add_neighboring(DVS_RANK_TYPE rank_a, DVS_RANK_TYPE rank_b){
	// find free slot for a :
	DVS_RANK_TYPE half_edge_a = find_free_half_edge(rank_a);
	DVS_RANK_TYPE half_edge_b = find_free_half_edge(rank_b);
	connect_half_edges(half_edge_a,half_edge_b);

	DVS_RANK_TYPE owner_of_a = half_edge_a / degree;
	DVS_RANK_TYPE index_of_neighbor_in_a = half_edge_a % degree;
	DVS_RANK_TYPE owner_of_b = half_edge_b / degree;
	DVS_RANK_TYPE index_of_neighbor_in_b = half_edge_b % degree;
	
	neighbors[owner_of_a * degree + index_of_neighbor_in_a] = owner_of_b;
	neighbors[owner_of_b * degree + index_of_neighbor_in_b] = owner_of_a;
}

void d_reg_neighboring_generator::connect_half_edges(DVS_RANK_TYPE half_edge_a, DVS_RANK_TYPE half_edge_b){
	DVS_RANK_TYPE index_of_a = half_edge_a;
	DVS_RANK_TYPE index_of_b = half_edge_b;
	
	while (half_edges[index_of_a] != half_edge_a) index_of_a = half_edges[index_of_a];
	while (half_edges[index_of_b] != half_edge_b) index_of_b = half_edges[index_of_b];
	
	//std::cout << "connect_half_edges : connecting (" << index_of_a << "," << index_of_b << ")" << std::endl;
	
	swap(&half_edges[nb_rank * degree - available_half_edges --], &half_edges[index_of_a]);
	swap(&half_edges[nb_rank * degree - available_half_edges --], &half_edges[index_of_b]);
	
}

inline void d_reg_neighboring_generator::swap(DVS_RANK_TYPE * a , DVS_RANK_TYPE * b){
	DVS_RANK_TYPE temp = *a;
	*a = *b;
	*b = temp;
}

void d_reg_neighboring_generator::rebuild_neighbors_from_half_edges(){
	int count = 0;
	for (size_t i = 0 ; i +1 < degree * nb_rank - available_half_edges; i+=2){
		// add that new edge to the neighbors table
		DVS_RANK_TYPE owner_of_a = half_edges[i] / degree;
		DVS_RANK_TYPE index_of_neighbor_in_a = half_edges[i] % degree;
		DVS_RANK_TYPE owner_of_b = half_edges[i+1] / degree;
		DVS_RANK_TYPE index_of_neighbor_in_b = half_edges[i+1] % degree;
		
		if (neighbors[owner_of_a * degree + index_of_neighbor_in_a] != owner_of_b) count++;
		neighbors[owner_of_a * degree + index_of_neighbor_in_a] = owner_of_b;
		if (neighbors[owner_of_b * degree + index_of_neighbor_in_b] != owner_of_a) count++;
		neighbors[owner_of_b * degree + index_of_neighbor_in_b] = owner_of_a;
	}
	//std::cout << "rebuild_neighbors_from_half_edges : updated " << count << " neighbors" << std::endl;
}

std::ostream & operator << (std::ostream & out, d_reg_neighboring_generator const & arg){
	arg.print(out);
	return out;
}





// *****************************************************
// ******* d1_kleinberg_neighboring_generator **********
// *****************************************************

int d1_kleinberg_neighboring_generator::get_distance(DVS_RANK_TYPE a,DVS_RANK_TYPE b)const{
	return std::min(((DVS_RANK_TYPE)abs(a-b)) , ((DVS_RANK_TYPE)(nb_rank - abs(a-b))));
}

size_t d1_kleinberg_neighboring_generator::get_neighbors_count(DVS_RANK_TYPE rank)const{
	assert(rank < nb_rank);
	return neighbors[rank].size(); 
}
DVS_RANK_TYPE d1_kleinberg_neighboring_generator::get_neighbor(DVS_RANK_TYPE rank, size_t index)const{
	assert(rank < nb_rank);
	assert(index < neighbors[rank].size());
	//! TODO : optimize : copy neighbors in an array for index based access. might not be worth the effort.
	auto it = neighbors[rank].begin();
	for (size_t i = 0 ; i < index ; i ++) it ++;
	return *it;
}

size_t d1_kleinberg_neighboring_generator::are_neighbors(DVS_RANK_TYPE a, DVS_RANK_TYPE b)const{
	return (neighbors[a].find(b) != neighbors[a].end());
}

void d1_kleinberg_neighboring_generator::print(std::ostream & out)const{	
	for (DVS_RANK_TYPE i=0; i < nb_rank ; i++){
		out << "rank " << i << " :";
		for (auto it = neighbors[i].begin() ; it != neighbors[i].end() ; it ++){
			out << " [" << *it << "]";
		}
		out << std::endl;
	}
}


d1_kleinberg_neighboring_generator::d1_kleinberg_neighboring_generator(size_t nb_rank, size_t degree, size_t local_reach):
	nb_rank(nb_rank),
	degree(degree),
	local_reach(local_reach)
{
	neighbors = new std::set<DVS_RANK_TYPE> [nb_rank];
	
	add_local_neighbors();
	add_random_neighbors();
}	

void d1_kleinberg_neighboring_generator::add_local_neighbors(){
	
	for (DVS_RANK_TYPE i=0;i<nb_rank;i++){
		for (DVS_RANK_TYPE j=1 ; j <= local_reach;j++){
			neighbors[i].insert((i+j) % nb_rank);
			neighbors[(i+j) % nb_rank].insert(i);
		}
	}
}

void d1_kleinberg_neighboring_generator::add_random_neighbors(uint64_t seed){
	tinymt64_t rand_ctx;
	// seed all the things
	rand_ctx.mat1 = seed + 1;
    rand_ctx.mat2 = seed - 1;
    rand_ctx.tmat = 1 - seed;
	tinymt64_init(& rand_ctx, seed);
	//std::cout << "nb_rank=" << nb_rank << ", degree=" << degree << ", first rand=" << tinymt64_generate_uint64(& rand_ctx) << std::endl;
	
	DVS_RANK_TYPE a,b;
	for (a = 0 ; a < nb_rank ; a++){
		for (uint i = 0 ; i < degree && neighbors[a].size() < nb_rank-1 ; i ++){
			double r = 	((double)tinymt64_generate_uint64(& rand_ctx)) / ((double)((uint64_t)-1));
			static const double x_max = ((double) nb_rank);
			//double x = 1 / (1 - r * ( 1 - 1 / x_max)); // distribution inverse carrée
			static const double l_x_max = log(x_max);
			double x = exp(l_x_max * r); // distribution inverse
			//double x = r * nb_rank +1; // distribution uniforme
			uint64_t distance = x;
			
			int sign = (tinymt64_generate_uint64(& rand_ctx) > ((uint64_t)-1)/2);
			if (sign == 0) sign = -1;
			
			b = (a + (distance * sign)) % nb_rank;
			
			neighbors[a].insert(b);
			auto p = neighbors[b].insert(a);
			if (p.second == false) i--;
		}
	}
}

DVS_RANK_TYPE d1_kleinberg_neighboring_generator::get_closest_neighbor(DVS_RANK_TYPE src, DVS_RANK_TYPE dest)const{
	
	auto it = neighbors[src].lower_bound(dest);
	DVS_RANK_TYPE upper_match = *it;

	if (upper_match == dest) return dest;
	
	
	DVS_RANK_TYPE lower_match;
	if (it != neighbors[src].end())
		lower_match = ( *it==*(neighbors[src].begin()) )? *it:*(--it);
	else
		lower_match = upper_match = *(neighbors[src].rbegin());

	DVS_RANK_TYPE lowest_neighbor = * neighbors[src].begin();
	DVS_RANK_TYPE highest_neighbor = * neighbors[src].rbegin();
	
	DVS_RANK_TYPE d_upper = get_distance(dest , upper_match);
	DVS_RANK_TYPE d_lower = get_distance(dest , lower_match);
	DVS_RANK_TYPE d_low   = get_distance(dest , lowest_neighbor);
	DVS_RANK_TYPE d_high  = get_distance(dest , highest_neighbor);
	
	DVS_RANK_TYPE minimum = std::min(std::min(d_upper,d_lower),std::min(d_low,d_high));
	
	if (minimum == d_upper) return upper_match;
	if (minimum == d_lower) return lower_match;
	if (minimum == d_low)   return lowest_neighbor;
	if (minimum == d_high)  return highest_neighbor;
	return -1;
}

// checks that there is no unidirectionnal edge in the graph
void d1_kleinberg_neighboring_generator::check_graph(std::ostream & out)const{
	for (size_t i = 0 ; i < nb_rank ; i++){
		for (auto it = neighbors[i].begin() ; it != neighbors[i].end() ; it ++){
			if (neighbors[*it].find(i) == neighbors[*it].end()){
				out << "error in graph : unidirectionnal edge : [" << i << "] -> [" << *it << "]" << std::endl;
			}
		}
	}
}

std::ostream & operator << (std::ostream & out, d1_kleinberg_neighboring_generator const & arg){
	arg.print(out);
	return out;
}
