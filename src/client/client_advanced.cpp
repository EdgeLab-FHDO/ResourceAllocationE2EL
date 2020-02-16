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

#include "clientTCP.h"
#include "serializer.h"

using namespace std;
using namespace std::chrono;

char state;

int delay_threshold_min = 100;
int delay_threshold_max = 200;

char* dummy_data;
int dummy_data_size;

time_point<system_clock, milliseconds> next_start_time;
time_point<system_clock, milliseconds> next_end_time;
time_point<system_clock, milliseconds> next_sync_time;

milliseconds period(1000);
milliseconds end_time_shift(1000);
milliseconds sync_time_shift(250);
milliseconds start_time_shift(500);

void connection_thread(ClientTCP * connection_TCP, char * output, bool * output_ready, string * server_name)
{
	time_point<system_clock, milliseconds> start_time = time_point_cast<milliseconds>(system_clock::now());
	time_point<system_clock, milliseconds> end_time = time_point_cast<milliseconds>(system_clock::now());
	
	time_point<system_clock, milliseconds> local_next_time = next_start_time;
	
	int connection_score = 0;
	while(true)
	{
		//cout << "next_start_time: " << next_start_time.time_since_epoch().count() << ", time_now: " << time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count() << endl;
		local_next_time = next_start_time;
		this_thread::sleep_until(local_next_time);
		
		if(!connection_TCP->is_connected())
		{
			connection_score = 0;
		}
		
		if(connection_TCP->is_connected() && !*output_ready)
		{
			start_time = time_point_cast<milliseconds>(system_clock::now());
			
			int state_size = 1;
			/*if(connection_TCP->send_data(&state, state_size) < 0)
			{
				cout << connection_TCP->get_last_error() << endl;
				continue;
			}*/
			
			dummy_data[0] = state;
			
			//cout << "dummy_data_size: " << dummy_data_size << endl;
			
			if(connection_TCP->send_data(dummy_data, dummy_data_size) < 0)
			{
				cout << connection_TCP->get_last_error() << endl;
				continue;
			}

			if(connection_TCP->receive_data(output, state_size) < 0)
			{
				cout << connection_TCP->get_last_error() << endl;
				continue;
			}
			
			end_time = time_point_cast<milliseconds>(system_clock::now());
			
			int delay_ms = (end_time - start_time).count();
								
			int delay_threshold;
			if(delay_threshold_max == delay_threshold_min)
			{
				delay_threshold = delay_threshold_max;
			}
			else
			{
				delay_threshold = rand() % (delay_threshold_max - delay_threshold_min) + delay_threshold_min;
			}
			
			if(delay_ms > delay_threshold)
			{
				connection_score--;
			}
			else
			{
				connection_score++;
			}
			
			if(connection_score >= 5)
			{
				connection_score = 5;
			}
			else if(connection_score <= -5)
			{
				connection_score = -5;
			}
			
			cout << *server_name << " delay: " << delay_ms << ", threshold: " << delay_threshold << ", score: " << connection_score << endl;
			
			if(connection_score <= -5)
			{	
				cout << *server_name << " delay is too large, disconnecting" << endl;
				
				connection_TCP->disconnect();
			}
			else if(connection_score >= 5)
			{
				*output_ready = true;
			}	
		}
	}
}


void output_thread(char * output_1, bool * output_ready_1, char * output_2, bool * output_ready_2, string * server_1_name, string * server_2_name)
{
	auto local_next_time = next_end_time;
	while(true)
	{
		local_next_time = next_end_time;
		this_thread::sleep_until(local_next_time);
		
		if(*output_ready_1)
		{
			state = *output_1;
			cout << "state from " << *server_1_name << ": " << (int)state << endl << endl;
		}
		else if(*output_ready_2)
		{
			state = *output_2;
			cout << "state from " << *server_2_name << ": " << (int)state << endl << endl;
		}
		else
		{
			state++;
			cout << "state from local server: " << (int)state << endl << endl;
		}
		
		*output_ready_1 = false;
		*output_ready_2 = false;
	}
}


void synchronizer_thread()
{
	while(true)
	{	
		next_sync_time = next_sync_time + period;
		
		next_start_time = next_start_time + period;
		
		next_end_time = next_end_time + period;
		
		//cout << "waiting until " << next_start_time.time_since_epoch().count() << endl;
		
		this_thread::sleep_until(next_sync_time);
	}
}


int main(int argc, char *argv[])
{
	if (argc < 8) {
	   cout << "usage " << argv[0] << " master_hostname master_port client_name application_id delay_threshold_min delay_threshold_max data_size" << endl;
       return -1;
    }
	
	char * master_hostname = argv[1];
	int master_port = atoi(argv[2]);
	string client_name(argv[3]);
	int application_id = atoi(argv[4]);
	delay_threshold_min = atoi(argv[5]);
	delay_threshold_max = atoi(argv[6]);
	dummy_data_size = atoi(argv[7]);
	
	if(delay_threshold_max < delay_threshold_min)
	{
		cout << "delay_threshold_max must be larger than or equal delay_threshold_min" << endl;
		return -1;
	}
	
	dummy_data = new char[dummy_data_size];
	
	const int type = 0;	// client
	
	ClientTCP connection_TCP_server_1;
	ClientTCP connection_TCP_server_2;
	
	ClientTCP master_TCP;
	
	bool output_ready_1 = false;
	bool output_ready_2 = false;
	
	char output_1 = 0;
	char output_2 = 0;
	
	string server_hostname;
	uint16_t server_port;
	
	string connection_1_server_name = "server name not yet assigned";
	string connection_2_server_name = "server name not yet assigned";
	
	Serializer serializer;
	
	const int max_buffer_size = 256;
	char buffer[max_buffer_size];
	int buffer_size;
	
	state = 0;
	
	auto time_now = time_point_cast<milliseconds>(system_clock::now());
	
	next_sync_time = time_now + sync_time_shift;
	next_start_time = time_now + start_time_shift;
	next_end_time = time_now + end_time_shift;
	
	thread synchronizer (synchronizer_thread);
	thread connection_1 (connection_thread, &connection_TCP_server_1, &output_1, &output_ready_1, &connection_1_server_name);
	thread connection_2 (connection_thread, &connection_TCP_server_2, &output_2, &output_ready_2, &connection_2_server_name);
	thread output (output_thread, &output_1, &output_ready_1, &output_2, &output_ready_2, &connection_1_server_name, &connection_2_server_name);
	
	bool socket_ready = false;
	
	auto local_next_time = next_end_time;
	
	while(true)
	{
		local_next_time = next_end_time;
		this_thread::sleep_until(local_next_time);
		
		usleep(100000);	// just to make sure that  the output thread is finished
		
		if(!connection_TCP_server_1.is_connected() || !connection_TCP_server_2.is_connected())
		{
			cout << "connecting to " << master_hostname << " on " << master_port << endl;
			
			if(master_TCP.connect_to(master_hostname, master_port, 2000) < 0)
			{
				cout << master_TCP.get_last_error() << endl;
				continue;
			}
			
			if(master_TCP.is_connected())
			{
				serializer.serialize_request(buffer, buffer_size, client_name, application_id, type);
				
				if(master_TCP.send_data(buffer, buffer_size) < 0)
				{
					cout << master_TCP.get_last_error() << endl;
					continue;
				}
				
				buffer_size = max_buffer_size;
				if(master_TCP.receive_data(buffer, buffer_size) < 0)
				{
					cout << master_TCP.get_last_error() << endl;
					continue;
				}
				
				master_TCP.disconnect();
				
				serializer.deserialize_response(buffer, server_hostname, server_port);
				
				cout << "server hostname: " << server_hostname << ", server port: " << server_port << endl;
				
				if(server_port == 0)	// no available servers
				{
					sleep(5);
					
					continue;
				}
				
				if(!connection_TCP_server_1.is_connected())
				{
					connection_1_server_name = server_hostname;
					
					cout << "connecting to " << connection_1_server_name << " on " << server_port << " as server 1" << endl;
				
					if(connection_TCP_server_1.connect_to(server_hostname.c_str(), server_port, 2000) < 0)
					{
						cout << connection_TCP_server_1.get_last_error() << endl;
						continue;
					}
				}
				else if(!connection_TCP_server_2.is_connected())
				{
					connection_2_server_name = server_hostname;
					
					cout << "connecting to " << connection_2_server_name << " on " << server_port << " as server 2" << endl;
					
					if(connection_TCP_server_2.connect_to(server_hostname.c_str(), server_port, 2000) < 0)
					{
						cout << connection_TCP_server_2.get_last_error() << endl;
						continue;
					}
				}
			}
		}
	}
    return 0;
}