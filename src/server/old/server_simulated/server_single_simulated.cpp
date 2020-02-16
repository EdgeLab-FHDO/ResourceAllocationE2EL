#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <string>

#include "serverTCP.h"
#include "clientTCP.h"
#include "serializer.h"

#include <cpprest/http_client.h>
#include <cpprest/filestream.h>

using namespace utility;                    // Common utilities like string conversions
using namespace web;                        // Common features like URIs.
using namespace web::http;                  // Common HTTP functionality
using namespace web::http::client;          // HTTP client features
//using namespace concurrency::streams;		// Asynchronous streams

using namespace std;

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
	ClientTCP master_TCP;
	
	Serializer serializer;
	
	char counter = 0;
	int counter_size = 1;
	
	bool status_advertised = false;
	
	const int max_buffer_size = 256;
	char buffer[max_buffer_size];
	int buffer_size;
	
	int data_buffer_size = 65536;
	char* data_buffer = new char[data_buffer_size];
	
	while(true)
	{
		while(!status_advertised)
		{
			cout << "connecting to " << master_hostname << " on " << master_port << endl;
			
			if(master_TCP.connect_to(master_hostname, master_port, 2000) < 0)
			{
				cout << master_TCP.get_last_error() << endl;
				sleep(1);
			}
			
			if(master_TCP.is_connected())
			{
				serializer.serialize_request(buffer, buffer_size, server_name, application_id, type, "");
				
				if(master_TCP.send_data(buffer, buffer_size) < 0)
				{
					cout << master_TCP.get_last_error() << endl;
					sleep(1);
					continue;
				}
				
				serializer.serialize_location(buffer, buffer_size, point_of_access, zone);
				
				cout << "point of access: " << buffer << ", zone: " << buffer + point_of_access.size() + 1 << ", buffer size: " << buffer_size << endl;
				
				usleep(20000);	// wait a little bit before sending data again
				
				if(master_TCP.send_data(buffer, buffer_size) < 0)
				{
					cout << master_TCP.get_last_error() << endl;
					sleep(1);
					continue;
				}
				
				status_advertised = true;
				master_TCP.disconnect();
			}
		}
		
		cout << "listening on " << server_port << endl;
		
		if(server_TCP.listen_on(server_port, 10000, 2000) < 0)
		{
			cout << server_TCP.get_last_error() << endl;
			sleep(1);
		}
		
		while(server_TCP.is_connected())
		{
			int received_data_size = dummy_data_size;
			if(server_TCP.receive_data_exact_size(data_buffer, received_data_size) < 0)
			{
				cout << server_TCP.get_last_error() << endl;
				sleep(1);
				break;
			}
			
			time_point<system_clock, milliseconds> start_time = time_point_cast<milliseconds>(system_clock::now());
			
			cout << "received data size: " << received_data_size << endl;
			
			if(received_data_size != dummy_data_size)
			{
				cout << "expected data size: " << dummy_data_size << endl;
				server_TCP.disconnect();
				sleep(1);
				break;
			}
			
			for(unsigned long long i = 0; i < load_size; i++)
			{
				for(unsigned long long j = 0; j < 1000000; j++);
			}
			
			counter = data_buffer[0];
			counter++;
			
			int counter_size = 1;
			if(server_TCP.send_data(&counter, counter_size) < 0)
			{
				cout << server_TCP.get_last_error() << endl;
				sleep(1);
				break;
			}
			
			time_point<system_clock, milliseconds> end_time = time_point_cast<milliseconds>(system_clock::now());
			
			int delay_ms = (end_time - start_time).count();
			
			cout << "server side delay: " << delay_ms << "ms" << endl;
			
			cout << "counter: " << (int)counter << endl;
		}
		
		status_advertised = false;
	}
	
	server_TCP.disconnect();
	
	return 0;
}