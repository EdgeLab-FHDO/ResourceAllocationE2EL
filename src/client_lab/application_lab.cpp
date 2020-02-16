#include "application_lab.h"


Application::Application() 
{
	initialized = false;
	
	terminating = false;
}


Application::~Application()
{
	terminating = true;
	
	cycles.join();
	
	delete[] data;
	
	delete[] frames;
}


void Application::run_one_cycle()
{
	if(connection_TCP->is_connected())
	{
		time_point<system_clock, milliseconds> time_now = time_point_cast<milliseconds, system_clock>(system_clock::now());

		long frame_number = ((long)((time_now - start_time).count() * fps / 1000))  % number_of_frames;

/*
		time_point<system_clock, milliseconds> processing_start_time = time_point_cast<milliseconds, system_clock>(system_clock::now());

		capture.set(CAP_PROP_POS_MSEC, current_video_time);
		
		capture >> frame;
		if (frame.empty())
		{
			cout << "ERROR can not read from video file " << endl;
			return;
		}

		resize(frame, frame_resized, frame_size);

		cvtColor(frame_resized, frame_gray, COLOR_BGR2GRAY);
		
		frame_gray = frame_gray.reshape(0, 1);
		
		time_point<system_clock, milliseconds> processing_end_time = time_point_cast<milliseconds, system_clock>(system_clock::now());
		
		cout << "client side processing delay: " << (processing_end_time - processing_start_time).count() << endl;*/
		
		//time_point<system_clock, milliseconds> transmission_start_time = time_point_cast<milliseconds, system_clock>(system_clock::now());
		
		int image_size = frames[0].total() * frames[0].elemSize();
		
		if(connection_TCP->send_data((const char *)frames[0].data, image_size) < 0)
		{
			cout << connection_TCP->get_last_error() << endl;
			return;
		}
		
		if(connection_TCP->send_data((const char *)frames[frame_number].data, image_size) < 0)
		{
			cout << connection_TCP->get_last_error() << endl;
			return;
		}
		
		int rectangle_size = 4;
		if(connection_TCP->receive_data(data, rectangle_size) < 0)
		{
			cout << connection_TCP->get_last_error() << endl;
			return;
		}
		
		//time_point<system_clock, milliseconds> transmission_end_time = time_point_cast<milliseconds, system_clock>(system_clock::now());
		
		//cout << "client side transmission delay: " << (transmission_end_time - transmission_start_time).count() << endl;
		
		rectangle_recieved_mutex.lock();
		rectangle_recieved.x = data[0];
		rectangle_recieved.y = data[1];
		rectangle_recieved.width = data[2];
		rectangle_recieved.height = data[3];
		rectangle_recieved_mutex.unlock();
		
		cout << "x = " << (int)data[0] << ", y = " << (int)data[1] << ", width = " << (int)data[2] << ", height = " << (int)data[3] << endl;
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
			
			run_one_cycle();	// TODO:  add a way to know if there was an error in this function
			
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
	
	
void Application::initialize(ClientTCP * connection_TCP, time_point<system_clock, milliseconds> start_time)
{
	if(!initialized)
	{
		initialized = true;
		
		should_start = false;
		
		finished = true;
		
		threshold_current_index = 0;
		
		string file_name_threshold = "data.txt";
		
		ifstream file;
		
		do 
		{
			file.open (file_name_threshold, ifstream::in);
			if(!file.is_open())
			{
				cout << "ERROR can not open file " << file_name_threshold << " for reading" << endl;
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
		
		string file_name_video = "1.mp4";
		
		do 
		{
			capture.open("1.mp4");
			if (!capture.isOpened()) 
			{
				cout << "ERROR can not open file " << file_name_video << " for reading" << endl;
				sleep(5);
			}
		} while (!capture.isOpened());
		
		fps = capture.get(CAP_PROP_FPS);
		number_of_frames = capture.get(CAP_PROP_FRAME_COUNT);
		//video_duration = (number_of_frames / fps) * 1000;
		
		frame_size = Size(256, 144);
		
		frames = new Mat[number_of_frames];
			
		for(long frame_number = 0; frame_number < number_of_frames; frame_number++)
		{
			capture >> frame;
			if (frame.empty())
			{
				cout << "ERROR can not read from file " << file_name_video << endl;
				sleep(5);
			}
			
			resize(frame, frame, frame_size);
		
			cvtColor(frame, frame, COLOR_BGR2GRAY);
			
			frames[frame_number] = frame.clone();
			
			frames[frame_number] = frames[frame_number].reshape(0, 1);
		}
		
		capture.release();
		
		this->data_size = 1024;
		data = new char[dummy_data_size];
		
		this->connection_TCP = connection_TCP;
		
		this->start_time = start_time;
		
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