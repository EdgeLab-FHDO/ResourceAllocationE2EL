#include "application.h"


Application::Application()
{
	initialized = false;
	
	terminating = false;
}


Application::~Application()
{
	terminating = true;
	
	cycles.join();
	
	delete[] dummy_data;
}


void Application::run_one_cycle()
{
	if(connection_TCP->is_connected())
	{
		dummy_data[0] = state;
		
		if(connection_TCP->send_data(dummy_data, dummy_data_size) < 0)
		{
			cout << connection_TCP->get_last_error() << endl;
			return;
		}

		if(connection_TCP->receive_data(dummy_data, state_size) < 0)
		{
			cout << connection_TCP->get_last_error() << endl;
			return;
		}
		
		state = dummy_data[0];
	}
}


void Application::cycles_thread()
{
	while(~terminating)
	{
		if(should_start)
		{
			should_start_mutex.lock();
			should_start = false;
			should_start_mutex.unlock();
			
			run_one_cycle();
			
			finished_mutex.lock();
			finished = true;
			finished_mutex.unlock();
		}
		
		usleep(1000);
	}
}


void Application::start()
{
	should_start_mutex.lock();
	should_start = true;
	should_start_mutex.unlock();
	
	finished_mutex.lock();
	finished = false;
	finished_mutex.unlock();
}
	
	
void Application::initialize(ClientTCP * connection_TCP, int dummy_data_size)
{
	if(!initialized)
	{
		initialized = true;
		
		should_start = false;
		
		finished = true;
		
		threshold_current_index = 0;
		
		string file_name = "data.txt";
		
		ifstream file;
		
		do 
		{
			file.open (file_name, ifstream::in);
			if(!file.is_open())
			{
				cout << "ERROR can not open file " << file_name << " for reading" << endl;
				sleep(5);
			}
		} while (!file.is_open());
		
		int min_threshold;
		int max_threshold;
		long threshold_end_time;
		
		char first_comma;
		char second_comma;
		
		while (file >> min_threshold >> first_comma >> max_threshold >> second_comma >> threshold_end_time)
		{
			if(first_comma == ',' && second_comma == ',')
			{
				min_threshold_list.push_back(min_threshold);
				max_threshold_list.push_back(max_threshold);
				threshold_end_time_list.push_back(threshold_end_time);
			}
		}
		
		file.close();
		
		state_size = 1;
		state = 0;
		
		this->dummy_data_size = dummy_data_size;
		dummy_data = new char[dummy_data_size];
		
		this->connection_TCP = connection_TCP;
		
		start_time = time_point_cast<milliseconds>(system_clock::now());
		
		cycles = thread(&Application::cycles_thread, this);
	}
}


bool Application::is_finished()
{
	return finished;
}


int Application::get_min_threshold()
{
	correct_threshold_index();
	return min_threshold_list[threshold_current_index];
}


int Application::get_max_threshold()
{
	correct_threshold_index();
	return max_threshold_list[threshold_current_index];
}

void Application::correct_threshold_index()
{
	auto time_now = time_point_cast<milliseconds>(system_clock::now());
	long relative_time_duration = (time_now - start_time).count() % threshold_end_time_list.back();
	
	if(threshold_current_index == threshold_end_time_list.size() - 1)	// if it is the last element, then check for relative_time_duration overflow
	{
		if(relative_time_duration < threshold_end_time_list[threshold_current_index - 1])
		{
			threshold_current_index = 0;
		}
	}
	
	while(relative_time_duration > threshold_end_time_list[threshold_current_index])
	{
		threshold_current_index++;
	}
}