! This file is part of the TITUS software.
! https://github.com/exapars/TITUS
! Copyright (c) 2015-2017 University of Versailles UVSQ - Exascale Computing Research
! Copyright (c) 2017 Bull SAS

! TITUS  is a free software: you can redistribute it and/or modify  
! it under the terms of the GNU Lesser General Public License as   
! published by the Free Software Foundation, version 3.

! TITUS is distributed in the hope that it will be useful, but 
! WITHOUT ANY WARRANTY; without even the implied warranty of 
! MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
! Lesser General Public License for more details.

! You should have received a copy of the GNU Lesser General Public License
! along with this program. If not, see <http://www.gnu.org/licenses/>.

module TITUS_DLB_C_BINDINGS
  use, intrinsic :: ISO_C_BINDING
  use gaspi_types
  
  implicit none
  
  interface ! void gaspi_return_t c_TITUS_DLB_gaspi_proc_init(gaspi_timeout_t timeout);
    function c_TITUS_DLB_gaspi_proc_init(timeout_ms) result (res) &
    bind (C, name="c_TITUS_DLB_gaspi_proc_init")
    import
    integer (gaspi_timeout_t), value :: timeout_ms
    integer (gaspi_return_t) :: res
    end function c_TITUS_DLB_gaspi_proc_init
  end interface
  
  interface ! void c_TITUS_DLB_parallel_work()
    subroutine c_TITUS_DLB_parallel_work() &
    bind (C, name="c_TITUS_DLB_parallel_work")
    end subroutine c_TITUS_DLB_parallel_work
  end interface
    
  interface ! void c_TITUS_DLB_set_context(void* arg)
    subroutine c_TITUS_DLB_set_context(ctx) &
    bind (C, name="c_TITUS_DLB_set_context")
    use ISO_C_BINDING
    type (C_PTR), value :: ctx
    end subroutine c_TITUS_DLB_set_context
  end interface

  interface ! void c_TITUS_DLB_get_context(void **ret_val)
    subroutine c_TITUS_DLB_get_context(ret_val) &
    bind (C, name="c_TITUS_DLB_get_context")
    use ISO_C_BINDING
    type (C_PTR) :: ret_val
    end subroutine c_TITUS_DLB_get_context
  end interface

  enum, bind(C)
    enumerator :: WORK_STEALING=0
    enumerator :: WORK_REQUESTING=1 
  end enum

  interface ! void c_TITUS_DLB_new_context(void **ret_val, int shared_task_segment_size, int algorithm)
    subroutine c_TITUS_DLB_new_context(ret_val, shared_task_segment_size, algorithm) &
    bind (C, name="c_TITUS_DLB_new_context")
    use ISO_C_BINDING
    type (C_PTR) :: ret_val
    integer (C_INT), value :: shared_task_segment_size
    integer (C_INT), value :: algorithm
    end subroutine c_TITUS_DLB_new_context
  end interface

  interface ! void c_TITUS_DLB_set_problem(
            !     void *context, void *problem_array, int task_size,
            !     int nb_task, void *result_array, int result_size,
            !     void *ptr_task_function, void *params)
    subroutine c_TITUS_DLB_set_problem( &
             context, problem_array, task_size, &
             nb_task, result_array, result_size, &
             ptr_task_function, params) &
    bind (C, name="c_TITUS_DLB_set_problem")
    use ISO_C_BINDING
    type (C_PTR), value :: context
    type (C_PTR), value :: problem_array
    integer (C_INT), value :: task_size
    integer (C_INT), value :: nb_task
    type (C_PTR), value :: result_array
    integer (C_INT), value :: result_size
    type (C_FUNPTR), value :: ptr_task_function
    type (C_PTR), value :: params
    end subroutine c_TITUS_DLB_set_problem
  end interface

  interface ! void c_TITUS_DLB_delete_context(void *context );
    subroutine c_TITUS_DLB_delete_context(context) &
    bind (C, name="c_TITUS_DLB_delete_context")
    use ISO_C_BINDING
    type (C_PTR), value :: context
    end subroutine c_TITUS_DLB_delete_context
  end interface


  interface ! void c_TITUS_DLB_get_logger(void **ret_val, void *context)
    subroutine c_TITUS_DLB_get_logger(ret_val, context) &
    bind (C, name="c_TITUS_DLB_get_logger")
    use ISO_C_BINDING
    type (C_PTR) :: ret_val
    type (C_PTR), value :: context
    end subroutine c_TITUS_DLB_get_logger
  end interface

  
  ! c_TITUS_DLB_LOGGER :
  
  interface ! void c_TITUS_Logger_print_current_session(void *logger, char *filename)
    subroutine c_TITUS_Logger_print_current_session(logger, filename) &
    bind (C, name="c_TITUS_Logger_print_current_session")
    use ISO_C_BINDING
    type (C_PTR), value :: logger
    type (C_PTR), value :: filename
    end subroutine c_TITUS_Logger_print_current_session
  end interface

  interface ! void c_TITUS_Logger_print_all_sessions(void *logger, char *filename)
    subroutine c_TITUS_Logger_print_all_sessions(logger, filename) &
    bind (C, name="c_TITUS_Logger_print_all_sessions")
    use ISO_C_BINDING
    type (C_PTR), value :: logger
    type (C_PTR), value :: filename
    end subroutine c_TITUS_Logger_print_all_sessions
  end interface


  interface ! void c_TITUS_Logger_reset_all_entries(void *logger)
    subroutine c_TITUS_Logger_reset_all_entries(logger) &
    bind (C, name="c_TITUS_Logger_reset_all_entries")
    use ISO_C_BINDING
    type (C_PTR), value :: logger
    end subroutine c_TITUS_Logger_reset_all_entries
  end interface

  ! DVS
  
  interface ! void c_TITUS_DLB_get_DVS_context(void **ret_val, void *context)
    subroutine c_TITUS_DLB_get_DVS_context(ret_val, context) &
    bind (C, name="c_TITUS_DLB_get_DVS_context")
    use ISO_C_BINDING
    type (C_PTR):: ret_val
    type (C_PTR), value :: context
    end subroutine c_TITUS_DLB_get_DVS_context
  end interface

  interface ! void c_TITUS_DLB_set_DVS_context(void *context, void *arg)
    subroutine c_TITUS_DLB_set_DVS_context(context, arg) &
    bind (C, name="c_TITUS_DLB_set_DVS_context")
    use ISO_C_BINDING
    type (C_PTR), value :: context
    type (C_PTR), value :: arg
    end subroutine c_TITUS_DLB_set_DVS_context
  end interface

end module TITUS_DLB_C_BINDINGS
