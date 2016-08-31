# 
# This file is part of the TITUS software.
# https://github.com/exapars/TITUS
# Copyright (c) 2015-2016 University of Versailles UVSQ
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
	CC               = mpicc -D__ICC 
	CXX              = mpicxx -std=c++0x -D__ICC
	MPICC            = mpicc -D__ICC
	MPICXX           = mpicxx -std=c++0x -D__ICC
	DFLAGS_OPT = -O0
	L_MATH           = -limf
else
	# user may export GCC_HAS_COLORING=true
	ifeq ($(GCC_HAS_COLORING),true)
		CC       = mpicc -fdiagnostics-color=always 
		CXX      = mpicxx -std=c++0x -fdiagnostics-color=always 
		MPICC    = mpicc -fdiagnostics-color=always 
		MPICXX   = mpicxx -std=c++0x -fdiagnostics-color=always 
	else
		CC       = mpicc 
		CXX      = mpicxx -std=c++0x 
		MPICC    = mpicc 
		MPICXX   = mpicxx -std=c++0x
	endif
	DFLAGS_OPT = -Og
	L_MATH = -lm
endif

# dependencies
GPI_INC=$(GASPI_HOME)/include
GPI_LIB=$(GASPI_HOME)/lib64

DLB_INC=$(DLB_HOME)/include
DLB_LIB=$(DLB_HOME)/lib

DVS_HOME=$(DLB_HOME)/victim_selector
DVS_INC=$(DVS_HOME)/include
DVS_LIB=$(DVS_HOME)/lib

# DFLAGS does not defines DEBUG as it has not been maintained
#! TODO : make debug behaviors -DDEBUG dependant and add it here
DFLAGS 		= $(DFLAGS_OPT) -g -fkeep-inline-functions -lsupc++
# always include DFLAGS for debug symbols
CFLAGS		= -fPIC -W -Wall -O2 -Wno-sign-compare $(DFLAGS) -I$(DLB_INC) -I$(GPI_INC) -I$(DVS_INC)
CXXFLAGS	= -fPIC -W -Wall -O2 -Wno-sign-compare $(DFLAGS)
LDFLAGS		= -fPIC -shared -L$(DVS_LIB) -lDVS -L$(GASPI_LIB) -lGPI2-dbg -libverbs -L$DLB_HOME/libunwind/lib -lunwind -lpthread $(L_MATH) $(DFLAGS_OPT)

SRC	:= $(wildcard src/*.cpp)
OBJ	:= $(patsubst src/%.cpp, obj/%.o, $(SRC))

#================================================================
#       	test CONFIGURATION
#================================================================
TEST_LDFLAGS	= -O2 -L$(DLB_LIB) -lDLB -L$(DVS_LIB) -lDVS -L$(GASPI_LIB) -lGPI2-dbg -libverbs -L$DLB_HOME/libunwind/lib -lunwind -lpthread $(L_MATH) $(DFLAGS_OPT)
TEST_CFLAGS     = -W -Wall $(DFLAGS)
TEST_CXXFLAGS   = $(CXXFLAGS) -W -Wall $(DFLAGS)
TEST_EXEC	    = mpirun -n 2

lib = lib/libDLB.a
tests = DLB_Bench

all : $(lib) $(tests)
#================================================================
#       	GENERATE libDLB.so
#================================================================
lib : $(lib)

# generate the library
lib/libDLB.so : $(OBJ)
	@mkdir -p lib
	$(CXX) -o $@ $^ $(LDFLAGS)

lib/libDLB.a : $(OBJ)
	@mkdir -p lib
	ar  rcs $@ $^ 

obj/%.o : src/%.cpp
	@mkdir -p obj
	$(CXX) $(CXXFLAGS) -o $@ -c $<

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


#================================================================
#       	DOC
#================================================================

# placeholder for future documentation generation

#================================================================
#       	CLEANING
#================================================================

clean:
	rm -rf *~ tests/bin/ tests/obj/ obj/ lib/

cleanall: clean
	rm -rvf  doxygen/html/ doxygen/latex/

mrproper:
	make cleanall
	make -C victim_selector/ clean

tarball: mrproper
	rm -f DLB_Library.tar.gz
	tar -czf DLB_Library.tar.gz .

.PHONY: all lib tests run clean cleanall mrproper tarball libDVS
