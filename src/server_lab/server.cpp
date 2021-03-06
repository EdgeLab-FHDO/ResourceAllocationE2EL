#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <string>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/video.hpp>

#include "serverTCP.h"
#include "clientTCP.h"

#include <cpprest/http_client.h>
#include <cpprest/filestream.h>

using namespace utility;                    // Common utilities like string conversions
using namespace web;                        // Common features like URIs.
using namespace web::http;                  // Common HTTP functionality
using namespace web::http::client;          // HTTP client features
//using namespace concurrency::streams;		// Asynchronous streams

using namespace cv;
using namespace std;

void motion_application_thread(ServerTCP * TCP_connection)
{
	int data_buffer_size = 65536;
	char* data_buffer = new char[data_buffer_size];
	
	Size image_size(256, 144);
	
	Mat frame = Mat::zeros(image_size, CV_8UC1);
	Mat base_frame = Mat::zeros(image_size, CV_8UC1);
	Mat modified_frame;
	Mat difference_frame;
	Mat dilated_frame;
	Mat fgMask;
	
	while(TCP_connection->is_connected())
	{
		int received_data_size = frame.total() * frame.elemSize();
		if(TCP_connection->receive_data_exact_size(data_buffer, received_data_size) < 0)
		{
			cout << TCP_connection->get_last_error() << endl;
			break;
		}
		
		int data_index = 0;
		for (int i = 0; i < frame.rows; i++)
		{
			for (int j = 0; j < frame.cols; j++)
			{
				base_frame.at<uchar>(i, j) = data_buffer[data_index];
				data_index++;
			}
		}
		
		received_data_size = frame.total() * frame.elemSize();
		if(TCP_connection->receive_data_exact_size(data_buffer, received_data_size) < 0)
		{
			cout << TCP_connection->get_last_error() << endl;
			break;
		}
		
		//time_point<system_clock, milliseconds> copy_start_time = time_point_cast<milliseconds>(system_clock::now());
		
		data_index = 0;
		for (int i = 0; i < frame.rows; i++)
		{
			for (int j = 0; j < frame.cols; j++)
			{
				frame.at<uchar>(i, j) = data_buffer[data_index];
				data_index++;
			}
		}
		
		/*time_point<system_clock, milliseconds> copy_end_time = time_point_cast<milliseconds>(system_clock::now());
		
		cout << "server copy delay: " << (copy_end_time - copy_start_time).count() << "ms" << endl;
		
		time_point<system_clock, milliseconds> start_time = time_point_cast<milliseconds>(system_clock::now());*/
		
		GaussianBlur(base_frame, base_frame, Size(11, 11), 0);
		
		GaussianBlur(frame, modified_frame, Size(11, 11), 0);

		absdiff(modified_frame, base_frame, difference_frame);

		threshold(difference_frame, fgMask, 12, 255, THRESH_BINARY);

		erode(fgMask, dilated_frame, Mat(), Point(-1, -1), 2);
		dilate(dilated_frame, dilated_frame, Mat(), Point(-1, -1), 2);

		dilate(dilated_frame, dilated_frame, Mat(), Point(-1, -1), 12);
		erode(dilated_frame, dilated_frame, Mat(), Point(-1, -1), 12);


		RNG rng(12345);
		vector<vector<Point> > contours;
		vector<Point> contours_poly;
		Rect boundRect;
		Point2f center;
		float radius;
		vector<Vec4i> hierarchy;
		findContours(dilated_frame, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);
		/*Mat contours_frame = Mat::zeros(dilated_frame.size(), CV_8UC3);
		for (size_t i = 0; i < contours.size(); i++)
		{
			if (contours.at(i).size() > 15)
			{
				Scalar color = Scalar(rng.uniform(0, 256), rng.uniform(0, 256), rng.uniform(0, 256));

				approxPolyDP(Mat(contours[i]), contours_poly, 3, true);
				boundRect = boundingRect(Mat(contours_poly));
				minEnclosingCircle((Mat)contours_poly, center, radius);

				drawContours(contours_frame, contours, (int)i, color, 2, LINE_8, hierarchy, 0);

				rectangle(final_frame, boundRect.tl(), boundRect.br(), color, 2, 8, 0);
				//circle(final_frame, center, (int)radius, color, 2, 8, 0);
			}
		}*/
		
		if(contours.size() >0)
		{	
			approxPolyDP(Mat(contours[0]), contours_poly, 3, true);
			boundRect = boundingRect(Mat(contours_poly));
		}	
		
		data_buffer[0] = boundRect.x;
		data_buffer[1] = boundRect.y;
		data_buffer[2] = boundRect.width;
		data_buffer[3] = boundRect.height;
		
		int sent_data_size = 4;
		
		if(TCP_connection->send_data(data_buffer, sent_data_size) < 0)
		{
			cout << TCP_connection->get_last_error() << endl;
			break;
		}
		
		/*time_point<system_clock, milliseconds> end_time = time_point_cast<milliseconds>(system_clock::now());
		
		int delay_ms = (end_time - start_time).count();
		
		cout << "server side delay: " << delay_ms << "ms" << endl;*/
	}
}

int main(int argc, char *argv[])
{
	if (argc < 10) {
		cout << "usage: " << argv[0] << " master_hostname master_port server_name server_port application_id point_of_access zone data_size load_size" << endl;
		return -1;
	}
	
	char * master_hostname = argv[1];
	int master_port = atoi(argv[2]);
	string server_name(argv[3]);
	int server_port = atoi(argv[4]);
	int application_id = atoi(argv[5]);
	string point_of_access(argv[6]);
	string zone(argv[7]);
	int dummy_data_size = atoi(argv[8]);
	unsigned long long load_size = stoull(argv[9]);
	
	const int type = 1;	// server
	
	/*try
	{
		http_client client(U("http://meep-mg-manager"));
		uri_builder builder(U("/v1/mg/" + server_name + "-srv/app/server"));
		http_response response = client.request(methods::POST, builder.to_string()).get();
		
		cout << "Received response status code: " << response.status_code() << endl;
	}
	catch (const std::exception& e)
	{
		cout << e.what() << endl;
	}*/
	
	ServerTCP server_TCP;
	
	while(true)
	{
		cout << "listening on " << server_port << endl;
		
		if(server_TCP.multithread_listen_on(server_port, 2000, motion_application_thread) < 0)
		{
			cout << server_TCP.get_last_error() << endl;
			continue;
		}
	}
	
	server_TCP.disconnect();
	
	return 0;
}