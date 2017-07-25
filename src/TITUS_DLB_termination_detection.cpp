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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GASPI.h>
#include <cstdint>
#include <assert.h>

#include <TITUS_DLB_gaspi_tools.hpp>
#include "TITUS_DLB_internal.hpp"
#include "TITUS_DLB_context.hpp"

TITUS_DLB_int TITUS_DLB_impl::termination_detection()
{
	// print on local termination (= first call)
    static bool local_termination_reached = false;
    if (local_termination_reached == false){
		local_termination_reached = true;
		TITUS_DBG << "local_termination_reached" << std::endl;
	}
    
    //DEBUG_PRINT("=================TITUS_DLB::termination_detection================\n");
    gaspi_rank_t parent, left_child, right_child;
    MetadataResult *r = context->get_metadata_result();
    int offset_termination = DIFF_PTR(&(r->termination_status.global_termination_detected) , r); // the offset of global global_termination_detected detection value.
    int offset_child; // the offset of terminaison detection value of my subtree in my parent's segment.
    
    static bool subtree_termintion_reached = false;
    
    if(context->get_rank() != 0)                             parent       = (context->get_rank() - 1)/2;
    else                                                     parent       = -1;
    if(2*context->get_rank()+1 < context->get_nb_ranks())    left_child   = 2*context->get_rank()+1;
    else                                                     left_child   = 0;// no left chid
    if(2*context->get_rank()+2 < context->get_nb_ranks())    right_child  = 2*context->get_rank()+2;
    else                                                     right_child  = 0;// no right child
    if(context->get_rank()%2 == 1)                           offset_child = DIFF_PTR(& r->termination_status.left_child_subtree_termination, r);
    if(context->get_rank()%2 == 0)                           offset_child = DIFF_PTR(& r->termination_status.right_child_subtree_termination, r);

    //if(2*context->get_rank()+1 < context->get_nb_ranks() && (2*context->get_rank()+2 >= context->get_nb_ranks()))
    //    std::cout << "rank " << context->get_rank() << " has only 1 child : termination_status.left_child_subtree_termination = " << r->termination_status.left_child_subtree_termination << " termination_status.right_child_subtree_termination = " << r->termination_status.right_child_subtree_termination << std::endl;
	
	
// ******** NOTIFY TOWARD ROOT *******
    // if not root and children have reached termination detection
    if(		context->get_rank() != 0 && 
			r->termination_status.left_child_subtree_termination == 1L && r->termination_status.right_child_subtree_termination == 1L && 
			subtree_termintion_reached == false)
    {
		TITUS_DBG << "TITUS_DLB_impl::termination_detection : subtree_termination_detected" << std::endl;
        subtree_termintion_reached = true;
        gaspi_atomic_value_t value_old;
    	//~ TITUS_DBG << "SUCCESS_OR_DIE( gaspi_atomic_fetch_add ("<<context->segment_result<<", "<<offset_child<<", "<<parent<<", "<<(gaspi_atomic_value_t)1<<", "<<&value_old<<", "<<TITUS_DLB_GASPI_TIMEOUT<<"));"<<std::endl; //TITUS_DBG.flush();
    	//~ gaspi_printf(msg.str().c_str());
    	SUCCESS_OR_DIE( gaspi_atomic_fetch_add (context->segment_result, offset_child, parent, (gaspi_atomic_value_t)1, &value_old, TITUS_DLB_GASPI_TIMEOUT));
        return FAILED;
    }

// ******* NOTIFY TOWARD LEAFS *******
	// if root && children have reached subtree termination
    if(		context->get_rank() == 0 && 
			r->termination_status.left_child_subtree_termination == 1L && r->termination_status.right_child_subtree_termination == 1L )
    {
		assert(subtree_termintion_reached == false); // root does not change this value
		TITUS_DBG << "TITUS_DLB_impl::termination_detection : subtree_termination_detected" << std::endl;
		TITUS_DBG << "TITUS_DLB_impl::termination_detection : end_notification_received" << std::endl;
        if(left_child>0){
			gaspi_atomic_value_t value_old;
    		SUCCESS_OR_DIE( gaspi_atomic_fetch_add (context->segment_result, offset_termination, left_child, (gaspi_atomic_value_t)1, &value_old, TITUS_DLB_GASPI_TIMEOUT));
			ASSERT(value_old == 0);
        }
        if(right_child>0){
			gaspi_atomic_value_t value_old;
    		SUCCESS_OR_DIE( gaspi_atomic_fetch_add (context->segment_result, offset_termination, right_child, (gaspi_atomic_value_t)1, &value_old, TITUS_DLB_GASPI_TIMEOUT));
			ASSERT(value_old == 0);
        }
        local_termination_reached = false;
        return SUCCESS;
    }
    // else if session termination reached
    if(subtree_termintion_reached==1 && r->termination_status.global_termination_detected == 1L)
    {
		TITUS_DBG << "TITUS_DLB_impl::termination_detection : end_notification_received" << std::endl;
        subtree_termintion_reached=false;  //reset
        if(left_child>0){
			gaspi_atomic_value_t value_old;
    		SUCCESS_OR_DIE( gaspi_atomic_fetch_add (context->segment_result, offset_termination, left_child, (gaspi_atomic_value_t)1, &value_old, TITUS_DLB_GASPI_TIMEOUT));
			ASSERT(value_old == 0);
        }
        if(right_child>0){
			gaspi_atomic_value_t value_old;
    		SUCCESS_OR_DIE( gaspi_atomic_fetch_add (context->segment_result, offset_termination, right_child, (gaspi_atomic_value_t)1, &value_old, TITUS_DLB_GASPI_TIMEOUT));
			ASSERT(value_old == 0);
        }
        local_termination_reached = false;
        return SUCCESS;
    }
    return FAILED;
}

