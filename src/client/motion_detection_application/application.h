#ifndef APPLICATION_H
#define APPLICATION_H

#include <mutex>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <fstream>
#include <vector>
#include <string>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/video.hpp>

#include "clientTCP.h"

using namespace cv;
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
		
		double fps;
		long number_of_frames;
		
		VideoCapture capture;
		
		bool initialized;
		
		char* data;
		int data_size;
		
		Mat frame;
		Mat frame_resized;
		Mat frame_gray;
		Mat first_frame;
		
		Mat * frames;
		
		Size frame_size;
		
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