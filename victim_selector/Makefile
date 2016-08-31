# 
# This file is part of the TITUS software.
# https://github.com/exapars/
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

#====================================================================================
#
#	AUTHOR	:	UEMURA Seijilo, FONTENAILLE Clément, PETIT Eric
#	FILE	:	Makefile
#	CONTENT	:
#
#====================================================================================


DFLAGS = -g -O0 -W -Wall

CXX = mpicxx -std=c++0x 
CFLAGS = -fPIC -Wall -O2 $(DFLAGS)
LDFLAGS = -fPIC -shared $(DFLAGS)

GPI_INC=$(GASPI_HOME)/include
GPI_LIB=$(GASPI_HOME)/lib64

DLB_INC=$(DLB_HOME)/include
DLB_LIB=$(DLB_HOME)/lib

DVS_HOME=$(DLB_HOME)/victim_selector
DVS_INC=$(DVS_HOME)/include
DVS_LIB=$(DVS_HOME)/lib


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

.PHONY: all run clean
