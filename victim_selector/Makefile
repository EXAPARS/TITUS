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


ifeq ($(USE_INTEL_COMPILER),true)
    CC               = mpiicc -D__ICC 
    CXX              = mpiicc -std=c++0x -D__ICC
    MPICC            = mpiicc -D__ICC
    MPICXX           = mpiicc -std=c++0x -D__ICC
    #L_SUPC++         = -lsupc++
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
    L_SUPC++     = -lsupc++
    L_MATH       = -lm
endif
	

DFLAGS = -g -O0 -W -Wall
CFLAGS = -fPIC -Wall -O2 $(DFLAGS)
LDFLAGS = -fPIC -shared $(DFLAGS)



SRC	:= $(wildcard src/*.cpp) $(wildcard src/*.cpp) $(wildcard src/TinyMT/*.c)
OBJ	:= $(patsubst src/%.cpp, obj/%.o, $(SRC))

ifeq ($(USE_INTEL_COMPILER),true)
	L_MATH= -limf
else
	L_MATH= -lm
endif

all : lib


#================================================================
#       	GENERATE libDVS.so
#================================================================

lib : lib/libDVS.so

# generate the library
lib/libDVS.so : $(OBJ)
	@mkdir -p lib
	$(CXX) $(LDFLAGS) -o $@ $^ $(L_MATH)

obj/%.o : src/%.cpp
	@mkdir -p obj
	$(CXX) $(CFLAGS) -o $@ -c $< $(L_MATH)

#================================================================
#       	CLEANING
#================================================================

clean:
	rm -rvf lib/ obj/

cleanall: clean
	rm -rvf  doxygen/html/ doxygen/latex/
mrproper: cleanall
.PHONY: all run clean
