# 
# This file is part of the TITUS software.
# https://github.com/exapars/
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
#

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
#is a useful one-liner which will give you the full directory name of the script no matter where it is being called from (from stackoverflow).
#This will work as long as the last component of the path used to find the script is not a symlink (directory links are OK). If you want to also resolve any links to the script itself, you need a multi-line solution

export TITUS_LOGGER_HOME=$DIR
export LD_LIBRARY_PATH=$TITUS_LOGGER_HOME/lib:$LD_LIBRARY_PATH
export LIBRARY_PATH=$TITUS_LOGGER_HOME/lib:$LIBRARY_PATH
export CPLUS_INCLUDE_PATH=$TITUS_LOGGER_HOME/include:$CPLUS_INCLUDE_PATH
export C_INCLUDE_PATH=$TITUS_LOGGER_HOME/include:$C_INCLUDE_PATH
