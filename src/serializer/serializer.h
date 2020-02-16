#ifndef SERIALIZER_H
#define SERIALIZER_H

#include <string>
#include <cstring>
#include <cstdint>

using namespace std;

class Serializer
{
	private:
	
	public:
	
	void serialize_request(char * buffer, int& size, string name, uint16_t application_id, char type, string user_equipment_name);
	void deserialize_request(const char * buffer, string& name, uint16_t& application_id, char& type, string& user_equipment_name);
	
	void serialize_location(char * buffer, int& size, string location_point_of_access, string location_zone);
	void deserialize_location(const char * buffer, string& location_point_of_access, string& location_zone);
	
	void serialize_response(char * buffer, int& size, string hostname, uint16_t port);
	void deserialize_response(const char * buffer, string& hostname, uint16_t& port);
	
	void serialize_logger_start(char * buffer, int& size, string name);
	void deserialize_logger_start(const char * buffer, string& name);
	
	void serialize_logger_time_point(char * buffer, int& size, int64_t time_point);
	void deserialize_logger_time_point(const char * buffer, int64_t& time_point);
};

#endif //SERIALIZER_H