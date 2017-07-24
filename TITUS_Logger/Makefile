# This file is part of the TITUS software.
# https://github.com/exapars/TITUS
# Copyright (c) 2015-2017 University of Versailles UVSQ
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

# include Makefile.in from parent directory
ifndef $(TITUS_DLB_HOME)
TITUS_DLB_HOME = ..
endif

include ../Makefile.in

#================================================================
#       	lib build CONFIGURATION
#================================================================

#~ CXX = clang -std=c++11
#~ MPICXX = clang -std=c++11

SRC	:= $(wildcard src/*.cpp)
OBJ	:= $(patsubst src/%.cpp, obj/%.o, $(SRC))
FSRC := $(wildcard src/*.f90)
FOBJ := $(patsubst src/%.f90, obj/%.f90.o, $(FSRC))

CFLAGS += $(TITUS_LOGGER_INC)
DFLAGS += 
CXXFLAGS += $(TITUS_LOGGER_INC)
F90FLAGS += $(TITUS_LOGGER_INC)


#================================================================
#       	test build CONFIGURATION
#================================================================

TEST_LDFLAGS	= -O2 $(L_MATH) $(DFLAGS) $(TITUS_LOGGER_LIB)
TEST_CFLAGS     = -W -Wall $(DFLAGS) $(TITUS_LOGGER_INC)
TEST_CXXFLAGS   = $(CXXFLAGS) -W -Wall
TEST_EXEC	    = 

#================================================================
#       	Targets
#================================================================
lib = lib/libTITUS_Logger.so
tests_src = $(wildcard tests/*.cpp)
tests = $(patsubst %.cpp, %, $(tests_src))
MOD := $(patsubst src/%.f90, include/%.mod, $(FSRC))

all : $(lib) $(tests) $(MOD)

#================================================================
#       	make lib
#================================================================
lib : $(lib)

rebuild_lib:
	make clean
	make -C victim_selector clean
	make

lib/%.so : $(OBJ) $(FOBJ)
	@mkdir -p lib
	$(CXX) -o $@ $^ $(LDFLAGS)

lib/%.a : $(OBJ) $(FOBJ)
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
#       	mate tests
#================================================================

tests : $(tests)

rebuild_tests: rebuild_lib
	make tests

libDVS:
	+make -C victim_selector/ -j$(J)

tests/% : tests/%.cpp $(lib)
	$(MPICXX) -o $@ $< $(TEST_CFLAGS) $(TEST_LDFLAGS)

run_tests:
	for test in tests ; do $(TEST_EXEC) $test; done;

	
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
	rm -rf *~ tests/bin/ tests/obj/ obj/ lib/ */*.mod

cleanall: clean
	rm -rvf  doxygen/html/ doxygen/latex/

mrproper:
	make cleanall
	make -C victim_selector/ mrproper

tarball: mrproper
	rm -f TITUS_Logger.tar.gz
	tar -czf TITUS_Logger.tar.gz .

.PHONY: all lib tests run clean cleanall mrproper tarball
