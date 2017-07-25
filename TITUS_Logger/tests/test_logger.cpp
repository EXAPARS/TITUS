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

#include <TITUS_Logger.hpp>
#include <unistd.h>
#include <sstream>

int main(){
	
	
	TITUS_Logger Logger;
	Logger.start_session();
	
	TITUS_Logger_Entry_counter<int> counter1 (& Logger, "test1");
	counter1 ++;
	
	TITUS_Logger_Entry_counter<int> counter2 (& Logger, "test2");
	counter2 += 2;
	
	TITUS_Logger_Entry_Timer_ns timer1 (& Logger,"timer0");
	
	timer1.start();
	timer1.stop();
	
	TITUS_Logger_Entry_Timer_ns_pow_histogram<10> h(& Logger,"histogram1", 10, 50000);
	
	// lets measure the precision of usleep
	for ( int i = 0 ; i < 40 ; i ++){
		std::stringstream tmpname;
		tmpname << "timer" << std::setfill('0') << std::setw(3) << i;
		TITUS_Logger_Entry_Timer_ns * itertimer = new TITUS_Logger_Entry_Timer_ns(& Logger, tmpname.str());
		h.start();
		itertimer->start();
		usleep(1);
		h.stop();
		itertimer->stop();
	}
	
	Logger.end_session();
	Logger.start_session();
	Logger.end_session();

	
	std::cout << Logger;
}
