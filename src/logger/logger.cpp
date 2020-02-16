#include <iostream>
#include <string>
#include <cstdint>
#include <fstream>
#include <vector>

#include "serverTCP.h"
#include "serializer.h"

using namespace std;

Serializer serializer;

ServerTCP TCP_server_connection;

bool should_end = false;

void file_writer_thread(ServerTCP * TCP_connection)
{
	const int max_buffer_size = 256;
	char buffer[max_buffer_size];
	int buffer_size;
	
	int64_t value_received;
	int value_size = sizeof(int64_t);
	
	string file_name;
	
	fstream file_stream;
	
	buffer_size = max_buffer_size;
	if(TCP_connection->receive_data(buffer, buffer_size) < 0)
	{
		cout << TCP_connection->get_last_error() << endl;
		return;
	}
	
	serializer.deserialize_logger_start(buffer, file_name);
	
	cout << "client name received = " << file_name << endl;
	
	file_stream.open(file_name, fstream::out | fstream::app | fstream::binary);
	if(!file_stream.is_open())
	{
		cout << "ERROR can not open file " << file_name << " for writing" << endl;
		TCP_connection->disconnect();
		return;
	}
		
	while(TCP_connection->is_connected() && should_end == false)
	{
		buffer_size = value_size;
		int result = TCP_connection->receive_data_exact_size_timeout_without_error(buffer, buffer_size);
		if(result == -2)	// timeout
		{
			continue;
		}
		else if(result < 0)
		{
			cout << TCP_connection->get_last_error() << endl;
			file_stream.close();
			return;
		}
		
		// not needed here since writing to a file is in binary mode
		//serializer.deserialize_logger_time_point(buffer, value_received);
		//cout << "value_received = " << value_received << endl;
		
		file_stream.write(buffer, buffer_size);
		if(file_stream.fail())
		{
			cout << "ERROR can not write to file " << file_name << endl;
			TCP_connection->disconnect();
			file_stream.close();
			return;
		}
		
		buffer_size = 1;
		buffer[0] = 0;
		if(TCP_connection->send_data(buffer, buffer_size) < 0)
		{
			cout << TCP_connection->get_last_error() << endl;
			file_stream.close();
			return;
		}
	}
	
	file_stream.close();
}

void server_thread(int port)
{
	while(true)
	{
		cout << "listening on " << port << endl;
		
		if(TCP_server_connection.multithread_listen_on(port, 2000, file_writer_thread) < 0)
		{
			cout << TCP_server_connection.get_last_error() << endl;
			continue;
		}
	}
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
	   cout << "usage " << argv[0] << " port" << endl;
       return -1;
    }
	
	int port = atoi(argv[1]);

	thread server(server_thread, port);
	
	cin.get();
	
	should_end = true;
	
	TCP_server_connection.join_threads();
	
	//sleep(10);
	
	return 0;
}