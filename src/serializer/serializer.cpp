#include "serializer.h"

void Serializer::serialize_request(char * buffer, int& size, string name, uint16_t application_id, char type, string user_equipment_name)
{
	buffer[0] = type;
	
	buffer[1] = (application_id / 256) % 256;
	buffer[2] = application_id % 256;
	
	strcpy(buffer + 3, name.c_str());
	
	strcpy(buffer + 3 + name.size() + 1, user_equipment_name.c_str());
	
	size = name.size() + 1 + user_equipment_name.size() + 1 + 1 + 2;	// name content + null + user_equipment_name content + null + type + id
}


void Serializer::deserialize_request(const char * buffer, string& name, uint16_t& application_id, char& type, string& user_equipment_name)
{
	type = buffer[0];
	
	application_id = ((uint8_t)buffer[1]) * 256 + ((uint8_t)buffer[2]);
	
	name.clear();
	name += (buffer + 3);
	
	user_equipment_name.clear();
	user_equipment_name += (buffer + 3 + name.size() + 1);
}


void Serializer::serialize_location(char * buffer, int& size, string location_point_of_access, string location_zone)
{
	strcpy(buffer, location_point_of_access.c_str());
	
	strcpy(buffer + location_point_of_access.size() + 1, location_zone.c_str());
	
	size = location_point_of_access.size() + 1 + location_zone.size() + 1;
}


void Serializer::deserialize_location(const char * buffer, string& location_point_of_access, string& location_zone)
{
	location_point_of_access.clear();
	location_point_of_access = buffer;
	
	location_zone.clear();
	location_zone = buffer + location_point_of_access.size() + 1;
}
	

void Serializer::serialize_response(char * buffer, int& size, string hostname, uint16_t port)
{
	buffer[0] = (port / 256) % 256;
	buffer[1] = port % 256;
	
	strcpy(buffer + 2, hostname.c_str());
	
	size = hostname.size() + 1 + 2;	// string content + null + port
}


void Serializer::deserialize_response(const char * buffer, string& hostname, uint16_t& port)
{
	port = ((uint8_t)buffer[0]) * (uint16_t)256 + ((uint8_t)buffer[1]);
	
	hostname.clear();
	hostname += (buffer + 2);
}


void Serializer::serialize_logger_start(char * buffer, int& size, string name)
{
	strcpy(buffer + 0, name.c_str());
	
	size = name.size() + 1;	// string content + null
}


void Serializer::deserialize_logger_start(const char * buffer, string& name)
{
	name.clear();
	name += (buffer + 0);
}	


void Serializer::serialize_logger_time_point(char * buffer, int& size, int64_t time_point)
{
	/*buffer[0] = (time_point / 256 / 256 / 256 / 256 / 256 / 256 / 256) % 256;
	buffer[1] = (time_point / 256 / 256 / 256 / 256 / 256 / 256) % 256;
	buffer[2] = (time_point / 256 / 256 / 256 / 256 / 256) % 256;
	buffer[3] = (time_point / 256 / 256 / 256 / 256) % 256;
	buffer[4] = (time_point / 256 / 256 / 256) % 256;
	buffer[5] = (time_point / 256 / 256) % 256;
	buffer[6] = (time_point / 256) % 256;
	buffer[7] = time_point % 256;*/
	
	buffer[0] = (time_point >> 56) & 0x00000000000000FF;
	buffer[1] = (time_point >> 48) & 0x00000000000000FF;
	buffer[2] = (time_point >> 40) & 0x00000000000000FF;
	buffer[3] = (time_point >> 32) & 0x00000000000000FF;
	buffer[4] = (time_point >> 24) & 0x00000000000000FF;
	buffer[5] = (time_point >> 16) & 0x00000000000000FF;
	buffer[6] = (time_point >> 8) & 0x00000000000000FF;
	buffer[7] = (time_point >> 0) & 0x00000000000000FF;
	
	size = 8;	// string content time_point
}


void Serializer::deserialize_logger_time_point(const char * buffer, int64_t& time_point)
{
	time_point = (
				 + ((uint64_t)buffer[0]) << 56
				 + ((uint64_t)buffer[1]) << 48
				 + ((uint64_t)buffer[2]) << 40
				 + ((uint64_t)buffer[3]) << 32
				 + ((uint64_t)buffer[4]) << 24
				 + ((uint64_t)buffer[5]) << 16
				 + ((uint64_t)buffer[6]) << 8
				 + ((uint64_t)buffer[7])
				 );
}