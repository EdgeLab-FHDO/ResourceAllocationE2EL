#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <iostream>
#include <errno.h>
#include <atomic>
#include <thread>
#include <chrono>
#include <string>
#include <cstdint>
#include <queue>
#include <deque>
#include <mutex>

#include "clientTCP.h"
#include "serializer.h"
#include "application.h"

using namespace std;
using namespace std::chrono;

ClientTCP connection_TCP_server;
ClientTCP master_TCP;

string server_hostname;
uint16_t server_port;

char * master_hostname;
int master_port;

char * logger_hostname;
int logger_port;

string client_name;
string user_equipment_name;

const int type = 0;	// client

int application_id;

int dummy_data_size;

Serializer serializer;

const int max_buffer_size = 256;
char buffer[max_buffer_size];
int buffer_size;

int connection_score = 0;
int max_connection_score = 20;

int calculate_average_delay(int new_delay)
{
	const int64_t time_window_ms = 60000;
	
	static deque<int> last_delays;
	static deque<time_point<system_clock, milliseconds>> last_delays_time_points;
	
	auto time_now = time_point_cast<milliseconds>(system_clock::now());
	
	last_delays.push_back(new_delay);
	last_delays_time_points.push_back(time_now);
	
	// remove all delays that are older than time_window_ms
	while((time_now - last_delays_time_points.front()).count() > time_window_ms)
	{
		last_delays.pop_front();
		last_delays_time_points.pop_front();
	}
	
	int64_t sum = 0;
	
	for(auto delay_iterator = last_delays.begin(); delay_iterator != last_delays.end(); ++delay_iterator)
	{
		sum += *delay_iterator;
	}
	
	int average = sum / last_delays.size();
	
	cout << "average delay: " << average << " ms over the last " << time_window_ms << " ms, using " << last_delays.size() << " samples" << endl;
	
	return average;
}

void logger_thread(string file_name, queue<int64_t> * output_queue, mutex * output_mutex, ClientTCP * logger_TCP)
{
	int64_t value_to_send;
	
	while(true)
	{
		cout << "connecting to " << logger_hostname << " on " << logger_port << endl;
				
		if(logger_TCP->connect_to(logger_hostname, logger_port, 2000) < 0)
		{
			cout << logger_TCP->get_last_error() << endl;
			sleep(5);
			continue;
		}
		
		serializer.serialize_logger_start(buffer, buffer_size, file_name);
		
		if(logger_TCP->send_data(buffer, buffer_size) < 0)
		{
			cout << logger_TCP->get_last_error() << endl;
			continue;
		}
		
		usleep(10000);	// make sure that packets are separate
		
		while(logger_TCP->is_connected())
		{
			sleep(1);
			while(!output_queue->empty())
			{
				output_mutex->lock();
				value_to_send = output_queue->front();
				output_mutex->unlock();
				
				serializer.serialize_logger_time_point(buffer, buffer_size, value_to_send);
						
				if(logger_TCP->send_data(buffer, buffer_size) < 0)
				{
					cout << logger_TCP->get_last_error() << endl;
					break;
				}
				
				buffer_size = max_buffer_size;
				if(logger_TCP->receive_data(buffer, buffer_size) < 0)
				{
					cout << logger_TCP->get_last_error() << endl;
					break;
				}
				if(buffer[0] != 0)
				{
					cout << "confirmation not recieved, disconnecting" << endl;
					logger_TCP->disconnect();
					break;
				}
				
				output_mutex->lock();
				output_queue->pop();
				output_mutex->unlock();
			}
		}
	}
}

void queue_data(queue<int64_t> * output_queue, mutex * output_mutex, int64_t time_point, int64_t score, int64_t delay, int64_t min_threshold, int64_t max_threshold)
{
	output_mutex->lock();
	output_queue->push(time_point);
	output_queue->push(score);
	output_queue->push(delay);	
	output_queue->push(min_threshold);
	output_queue->push(max_threshold);
	output_mutex->unlock();
}

void queue_data(queue<int64_t> * output_queue, mutex * output_mutex, int64_t time_point, int64_t score)
{
	output_mutex->lock();
	output_queue->push(time_point);
	output_queue->push(score);
	output_mutex->unlock();
}

void connect_to_server()
{
	cout << "connecting to " << master_hostname << " on " << master_port << endl;
			
	if(master_TCP.connect_to(master_hostname, master_port, 2000) < 0)
	{
		cout << master_TCP.get_last_error() << endl;
		return;
	}
	
	if(master_TCP.is_connected())
	{
		serializer.serialize_request(buffer, buffer_size, client_name, application_id, type, user_equipment_name);
		
		if(master_TCP.send_data(buffer, buffer_size) < 0)
		{
			cout << master_TCP.get_last_error() << endl;
			return;
		}
		
		buffer_size = max_buffer_size;
		if(master_TCP.receive_data(buffer, buffer_size) < 0)
		{
			cout << master_TCP.get_last_error() << endl;
			return;
		}
		
		master_TCP.disconnect();
		
		serializer.deserialize_response(buffer, server_hostname, server_port);
		
		cout << "server hostname: " << server_hostname << ", server port: " << server_port << endl;
		
		if(server_port == 0)	// no available servers
		{
			sleep(1);
			
			return;
		}
		
		cout << "connecting to " << server_hostname << " on " << server_port << endl;
	
		if(connection_TCP_server.connect_to(server_hostname.c_str(), server_port, 2000) < 0)
		{
			cout << connection_TCP_server.get_last_error() << endl;
			return;
		}
	}
}

int main(int argc, char *argv[])
{
	if (argc < 9) {
	   cout << "usage " << argv[0] << " master_hostname master_port logger_hostname logger_port client_name application_id dummy_data_size user_equipment_name" << endl;
       return -1;
    }
	
	master_hostname = argv[1];
	master_port = atoi(argv[2]);
	logger_hostname = argv[3];
	logger_port = atoi(argv[4]);
	client_name = argv[5];
	application_id = atoi(argv[6]);
	dummy_data_size = atoi(argv[7]);
	user_equipment_name = argv[8];
		
	auto time_now = time_point_cast<milliseconds>(system_clock::now());
	auto app_start_time = time_point_cast<milliseconds>(system_clock::now());
	auto app_end_time = time_point_cast<milliseconds>(system_clock::now());
	auto app_earliest_end_time = time_point_cast<milliseconds>(system_clock::now());
	auto app_latest_end_time = time_point_cast<milliseconds>(system_clock::now());
	
	milliseconds min_app_period(0);
	milliseconds max_app_period(0);
	
	bool missed_latest_end_time = false;
	bool connection_first_loop = true;
	bool disconnection_first_loop = true;
	
	ClientTCP logger_TCP_time_points;
	queue<int64_t> output_time_points;
	mutex output_time_points_mutex;
	
	thread logger_time_points(logger_thread, client_name + "_time", &output_time_points, &output_time_points_mutex, &logger_TCP_time_points);
	
	Application application;
	application.initialize(&connection_TCP_server, dummy_data_size);
	
	while(true)
	{	
		if(!connection_TCP_server.is_connected())
		{
			connection_first_loop = true;
			
			if(disconnection_first_loop)
			{
				disconnection_first_loop = false;
				
				connection_score = 0;
				
				time_now = time_point_cast<milliseconds>(system_clock::now());
				//queue_data(&output_time_points, &output_time_points_mutex, time_now.time_since_epoch().count(), connection_score, -1, -1, -1);
				queue_data(&output_time_points, &output_time_points_mutex, time_now.time_since_epoch().count(), connection_score);
			}
			
			sleep(1);
			
			connect_to_server();
		}
		else
		{
			app_start_time = time_point_cast<milliseconds>(system_clock::now());
			application.start();
			
			disconnection_first_loop = true;
			
			if(connection_first_loop)
			{
				connection_first_loop = false;
				connection_score = max_connection_score / 2;
				
				time_now = time_point_cast<milliseconds>(system_clock::now());
				//queue_data(&output_time_points, &output_time_points_mutex, time_now.time_since_epoch().count(), connection_score, -1, -1, -1);
				queue_data(&output_time_points, &output_time_points_mutex, time_now.time_since_epoch().count(), connection_score);
			}
			
			min_app_period = milliseconds(application.get_min_threshold());
			max_app_period = milliseconds(application.get_max_threshold());
			
			app_earliest_end_time = app_start_time + min_app_period;
			app_latest_end_time = app_start_time + max_app_period;
			
			//cout << "max_app_period = " << max_app_period.count() << " min_app_period = " << min_app_period.count() << endl;
			//cout << "app_start_time = " << app_start_time.time_since_epoch().count() << ", app_earliest_end_time = " << app_earliest_end_time.time_since_epoch().count() << ", app_latest_end_time = " << app_latest_end_time.time_since_epoch().count() << endl;
			
			this_thread::sleep_until(app_earliest_end_time);
			
			missed_latest_end_time = false;
			
			while(true)
			{
				if(application.is_finished())
				{
					if(connection_TCP_server.is_connected())	// if application finished successfully
					{
						app_end_time = time_point_cast<milliseconds>(system_clock::now());
						
						int delay_ms = (app_end_time - app_start_time).count();
						
						cout << "delay = " << delay_ms << endl;
						
						calculate_average_delay(delay_ms);
						
						//cout << "output ready at " << app_end_time.time_since_epoch().count() << ", delay = " << delay_ms << endl;
						
						if(!missed_latest_end_time)	// only increase score if no latest end time is missed (score was not decreased this cycle)
						{
							if(connection_score < max_connection_score)
							{
								connection_score++;
								
								queue_data(&output_time_points, &output_time_points_mutex, time_now.time_since_epoch().count(), connection_score);
																
								//cout << "score increased to " << connection_score << endl;
							}
						}
						
						time_now = time_point_cast<milliseconds>(system_clock::now());
						//queue_data(&output_time_points, &output_time_points_mutex, time_now.time_since_epoch().count(), connection_score, delay_ms, min_app_period.count(), max_app_period.count());
					}
					
					break;
				}
				
				time_now = time_point_cast<milliseconds>(system_clock::now());
				if(time_now > app_latest_end_time)
				{
					connection_score--;
					
					time_now = time_point_cast<milliseconds>(system_clock::now());
					//queue_data(&output_time_points, &output_time_points_mutex, time_now.time_since_epoch().count(), connection_score, -1, min_app_period.count(), max_app_period.count());
					queue_data(&output_time_points, &output_time_points_mutex, time_now.time_since_epoch().count(), connection_score);
								
					//cout << "score decreased to " << connection_score << endl;
					
					missed_latest_end_time = true;
					
					if(connection_score <= 0)
					{
						connection_TCP_server.disconnect();
						
						//cout << "delay is too large, disconnecting" << endl;
						
						break;
					}
					
					max_app_period = milliseconds(application.get_max_threshold());
					app_latest_end_time += max_app_period;
					
					//cout << "max_app_period = " << max_app_period.count() << endl;
					//cout << "app_latest_end_time pushed to " << app_latest_end_time.time_since_epoch().count() << endl;
				}
				
				usleep(1000);
			}
		}
	}
    return 0;
}