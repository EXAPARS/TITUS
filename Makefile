# 
# This file is part of the TITUS software.
# https://github.com/exapars/TITUS
# Copyright (c) 2015-2016 University of Versailles UVSQ
# Copyright (c) 2017 Bull SAS
#
# TITUS  is a free software: you can redistribute it and/or modify  
# it under the terms of the GNU Lesser General Public License as   
# published by the Free Software Foundation, version 3.
#
# TITUS is distributed in the hope that it will be useful, but 
# WITHOUT ANY WARRANTY; without even the implied warranty of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#

#================================================================
#       	lib CONFIGURATION
#================================================================
#! TODO : properly handle Clang too
ifeq ($(USE_INTEL_COMPILER),true)
    CC               = mpiicc -D__ICC 
    CXX              = mpiicc -std=c++0x -D__ICC
    MPICC            = mpiicc -D__ICC
    MPICXX           = mpiicc -std=c++0x -D__ICC
    F90              = mpiifort
    #L_SUPC++         = -lsupc++
    L_MATH           = -limf
else
    # user may export GCC_HAS_COLORING=true
    ifeq ($(GCC_HAS_COLORING),true)
        CC       = mpicc -fdiagnostics-color=always 
        CXX      = mpicxx -std=c++0x -fdiagnostics-color=always 
        MPICC    = mpicc -fdiagnostics-color=always 
        MPICXX   = mpicxx -std=c++0x -fdiagnostics-color=always
        F90      = mpif90
    else
        CC       = mpicc 
        CXX      = mpicxx -std=c++0x 
        MPICC    = mpicc 
        MPICXX   = mpicxx -std=c++0x
        F90      = mpif90
    endif
    L_SUPC++     = -lsupc++
    L_MATH       = -lm
endif

TITUS_DLB_INC = -I$(TITUS_DLB_HOME)/include
TITUS_DLB_LIB = -L$(TITUS_DLB_HOME)/lib -lTITUS_DLB

DVS_HOME = $(TITUS_DLB_HOME)/victim_selector
DVS_INC = -I$(DVS_HOME)/include
DVS_LIB = -L$(DVS_HOME)/lib -lDVS

GASPI_INC = -I$(GASPI_HOME)/include
GASPI_LIB = -L$(GASPI_HOME)/lib64 -lGPI2-dbg

UNWIND_INC = -I$(TITUS_DLB_HOME)/libunwind/include
UNWIND_LIB = -L$(TITUS_DLB_HOME)/libunwind/include -lunwind

DFLAGS 		= -g -O0 $(L_SUPC++)
CFLAGS		= -fPIC -W -Wall -O2 -Wno-sign-compare -DMPI $(DFLAGS) $(TITUS_DLB_INC) $(DVS_INC) $(GASPI_INC) $(UNWIND_INC)
IBVERBS_LIB		= -libverbs
# GNU
#CXXFLAGS	= -g -O0 -std=c++11
#INTEL
CXXFLAGS	= -fPIC -O2 -W -Wall -Wno-sign-compare $(DFLAGS) $(TITUS_DLB_INC) $(DVS_INC) $(GASPI_INC) $(UNWIND_INC)
F90FLAGS	= -fPIC -O2 -cpp -free -warn all -stand f08 $(DFLAGS) $(TITUS_DLB_INC) $(DVS_INC) $(GASPI_INC)
LDFLAGS		= -fPIC -shared $(DVS_LIB) $(GASPI_LIB) $(IBVERBS_LIB) $(UNWIND_LIB) -lpthread $(L_MATH) $(DFLAGS)

SRC	:= $(wildcard src/*.cpp)
OBJ	:= $(patsubst src/%.cpp, obj/%.o, $(SRC))
FSRC := $(wildcard src/*.f90)
FOBJ := $(patsubst src/%.f90, obj/%.f90.o, $(FSRC))
MOD := $(patsubst src/%.f90, include/%.mod, $(FSRC))

#================================================================
#       	test CONFIGURATION
#================================================================
TEST_LDFLAGS	= -O2 $(TITUS_DLB_LIB) $(DVS_LIB) $(GASPI_LIB) $(IBVERBS_LIB) $(UNWIND_LIB) -lpthread $(L_MATH) $(DFLAGS)
TEST_CFLAGS     = -W -Wall $(DFLAGS) $(TITUS_DLB_INC) $(DVS_INC) $(GASPI_INC)
TEST_CXXFLAGS   = $(CXXFLAGS) -W -Wall
TEST_EXEC	    = mpirun -n 2

lib = lib/libTITUS_DLB.a
tests = DLB_Bench

all : $(lib) $(tests) $(MOD)
#================================================================
#       	GENERATE libTITUS_DLB.so
#================================================================
lib : $(lib)

# generate the library
lib/libTITUS_DLB.so : $(OBJ) $(FOBJ)
	@mkdir -p lib
	$(CXX) -o $@ $^ $(LDFLAGS)

lib/libTITUS_DLB.a : $(OBJ) $(FOBJ)
	@mkdir -p lib
	ar  rcs $@ $^ 

obj/%.o : src/%.cpp
	@mkdir -p obj
	$(CXX) $(CXXFLAGS) -o $@ -c $<

obj/%.f90.o : src/%.f90
	$(F90) $(F90FLAGS) -o $@ -c $<

include/%.mod : obj/%.f90.o src/%.f90
	@cp *.mod include/
	@mkdir -p lib
	@cp *.mod lib/
	@rm *.mod

#================================================================
#       	GENERATE TESTS
#================================================================

tests : $(tests)

rebuild_lib:
	make clean
	make -C victim_selector clean
	make -j 4
	make -j 4 -C victim_selector
	
rebuild_tests: rebuild_lib
	make tests

DLB_Bench: tests/bin/DLB_Bench.bin

libDVS:
	+make -C victim_selector/ -j$(J)

tests/bin/DLB_Bench.bin: tests/obj/DLB_Bench.o $(lib) libDVS
	@mkdir -p tests/bin
	$(MPICXX) -o $@ $<  $(TEST_LDFLAGS)

tests/obj/DLB_Bench.o : tests/src/DLB_Bench.cpp
	@mkdir -p tests/obj
	$(MPICXX) -o $@ -c $<  $(TEST_CXXFLAGS)

run_DLB_Bench:
	$(TEST_EXEC) ./tests/bin/DLB_Bench.bin



test_gaspi_group_numbers: tests/bin/test_gaspi_group_numbers.bin

tests/bin/test_gaspi_group_numbers.bin : tests/obj/test_gaspi_group_numbers.o
	@mkdir -p tests/bin
	$(MPICXX) -o $@ $< $(TEST_LDFLAGS)

tests/obj/test_gaspi_group_numbers.o : tests/src/test_gaspi_group_numbers.cpp
	@mkdir -p tests/obj
	$(MPICXX) -o $@ -c $< $(TEST_CXXFLAGS)


	
test_small_world_generation: tests/bin/test_small_world_generation.bin

tests/bin/test_small_world_generation.bin : tests/obj/test_small_world_generation.o
	@mkdir -p tests/bin
	$(MPICXX) -o $@ $< $(TEST_LDFLAGS)

tests/obj/test_small_world_generation.o : tests/src/test_small_world_generation.cpp
	@mkdir -p tests/obj
	$(MPICXX) -o $@ -c $< $(TEST_CXXFLAGS)



	
#TODO doc part is nor ready for release!

##================================================================
##       	DOXYGEN DOCUMENTATION
##================================================================
#
#doc: doxygen/Doxyfile
#	doxygen doxygen/Doxyfile
#



#================================================================
#       	CLEANING
#================================================================

clean:
	make -C victim_selector/ clean
	rm -rf *~ tests/bin/ tests/obj/ obj/ lib/ */*.mod

cleanall: clean
	rm -rvf  doxygen/html/ doxygen/latex/

mrproper:
	make cleanall
	make -C victim_selector/ mrproper

tarball: mrproper
	rm -f TITUS_DLB_Library.tar.gz
	tar -czf TITUS_DLB_Library.tar.gz .

.PHONY: all lib tests run clean cleanall mrproper tarball libDVS
