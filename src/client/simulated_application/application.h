#ifndef APPLICATION_H
#define APPLICATION_H

#include <mutex>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <fstream>
#include <vector>
#include <string>

#include "clientTCP.h"

using namespace std;
using namespace std::chrono;

class Application
{
	private:
	
		bool should_start;
		mutex should_start_mutex;
		
		bool finished;
		mutex finished_mutex;
		
		vector<int> min_threshold_list;
		vector<int> max_threshold_list;
		vector<long> threshold_end_time_list;
		int threshold_current_index;
		
		time_point<system_clock, milliseconds> start_time;
		
		int state;
		int state_size;
		
		bool initialized;
		
		char* dummy_data;
		int dummy_data_size;
		
		ClientTCP * connection_TCP;
		
		bool terminating;
		
		thread cycles;
		
		void run_one_cycle();
		
		void cycles_thread();
		
		void correct_threshold_index();
	
	public:
	
		Application();
		~Application();
		
		void start();
		
		void initialize(ClientTCP * connection_TCP, int dummy_data_size);
		
		bool is_finished();
		
		int get_min_threshold();
		int get_max_threshold();
};

#endif //APPLICATION_H