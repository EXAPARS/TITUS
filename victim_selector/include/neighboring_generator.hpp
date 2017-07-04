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


#ifndef __NEIGHBORING_GENERATOR_H
#define __NEIGHBORING_GENERATOR_H


#include <iostream>
#include <fstream>
#include <cstdlib>
#include <utility>
#include <cassert>
#include <cstdint>
#include <set>
#include <algorithm>


#ifndef DVS_RANK_TYPE
#include <DVS.hpp>
#endif


struct infixed_b_tree_neighbors{
	DVS_RANK_TYPE rank;
	DVS_RANK_TYPE parent;
	DVS_RANK_TYPE left_child;
	DVS_RANK_TYPE right_child;
	
	bool is_left_child(){return rank < parent;}
	bool is_right_child(){return rank > parent;}
	
	static void print_full_tree(std::ostream & out, DVS_RANK_TYPE n_ranks);
	static infixed_b_tree_neighbors get_tree_neighbors(DVS_RANK_TYPE rank, size_t subtree_size, DVS_RANK_TYPE rank0 = 0);
	static DVS_RANK_TYPE get_rank_of_root(size_t subtree_size, DVS_RANK_TYPE rank0 = 0);
};

std::ostream & operator<<(std::ostream & out, const infixed_b_tree_neighbors &arg);

class d_reg_neighboring_generator{
	public :
	d_reg_neighboring_generator(size_t nb_rank, size_t degree);
	
	void add_all_neighbors_from_tree();
	void add_random_neighbors();

	bool are_neighbors(DVS_RANK_TYPE rank0, DVS_RANK_TYPE rank1);
	DVS_RANK_TYPE get_neighbor(DVS_RANK_TYPE rank, size_t index){
		assert(rank < nb_rank);
		assert(index < degree);
		return neighbors[rank * degree + index];
	}
	
	// **********************
	// ****** Printers ******
	// **********************
	
	void print(std::ostream & out)const;	
	void print_paired_half_edges(std::ostream & out)const;	
	
	size_t get_degree(){ return degree; }
	DVS_RANK_TYPE get_nb_ranks(){ return nb_rank; }
	
	protected :
	
	DVS_RANK_TYPE nb_rank;
	size_t degree;
	
	DVS_RANK_TYPE * half_edges;
	size_t available_half_edges; // available_half_edges are at the end of the half_edges table
	DVS_RANK_TYPE * neighbors;
	
	void add_random_neighbor();
	
	// ********************************************
	// *** methods for building a infixed btree ***
	// ********************************************
	
	void add_neighbors_of_rank_in_tree(DVS_RANK_TYPE rank);
	DVS_RANK_TYPE get_rank_of_root(size_t subtree_size);
	
	// recursively searches the parent of rank in a tree nb_ranks wide
	// returns the rank of the parent of rank and the size of the subtree of which rank is the root
	std::pair<DVS_RANK_TYPE,size_t> get_parent_of_rank(DVS_RANK_TYPE rank, size_t nb_ranks);
	
	// ************************************
	// *** Utils and basic manipulators ***
	// ************************************
	
	DVS_RANK_TYPE find_free_half_edge(DVS_RANK_TYPE rank);
	// std::pair a free edge of a with one of b and write this std::pair in neighbors 
	void add_neighboring(DVS_RANK_TYPE rank_a, DVS_RANK_TYPE rank_b);
	
	void connect_half_edges(DVS_RANK_TYPE half_edge_a, DVS_RANK_TYPE half_edge_b);
	inline void swap(DVS_RANK_TYPE * a , DVS_RANK_TYPE * b);	
	void rebuild_neighbors_from_half_edges();
};

std::ostream & operator << (std::ostream & out, d_reg_neighboring_generator const & arg);


class d1_kleinberg_neighboring_generator{

protected :
	size_t nb_rank;
	size_t degree;
	size_t local_reach;

	void add_local_neighbors();
	void add_random_neighbors(uint64_t seed = 145785213);
	
	std::set<DVS_RANK_TYPE> * neighbors;

public :
	d1_kleinberg_neighboring_generator(size_t nb_rank, size_t degree, size_t local_reach = 1);
	
	size_t get_neighbors_count(DVS_RANK_TYPE rank)const;
	bool are_neighbors(DVS_RANK_TYPE a,DVS_RANK_TYPE b)const;
	
	std::set<DVS_RANK_TYPE>::iterator begin_of(DVS_RANK_TYPE rank){ return neighbors[rank].begin(); }
	std::set<DVS_RANK_TYPE>::iterator end_of(DVS_RANK_TYPE rank){ return neighbors[rank].end(); };
	
	DVS_RANK_TYPE get_neighbor(DVS_RANK_TYPE rank, size_t index)const;
	
	// returns the distance between two ranks in the underlying topology
	int get_distance(DVS_RANK_TYPE a,DVS_RANK_TYPE b)const;
	
	// returns the closest neighbor of src to dest
	DVS_RANK_TYPE get_closest_neighbor(DVS_RANK_TYPE src, DVS_RANK_TYPE dest)const;

	void print(std::ostream & out)const;
	
	void check_graph(std::ostream & out = std::cerr)const; // checks that there is no unidirectionnal edge in the graph
	
	size_t get_nb_ranks()const{return nb_rank;}
	size_t get_degree()const{return degree;}
	size_t get_local_reach()const{return local_reach;}
};
std::ostream & operator << (std::ostream & out, d1_kleinberg_neighboring_generator const & arg);

#endif //__NEIGHBORING_GENERATOR_H
