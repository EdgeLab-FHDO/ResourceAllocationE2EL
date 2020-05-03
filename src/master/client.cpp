#include "client.h"


Client::Client(string _name, int _application_id, string _location_user_equipment)
{
	name = _name;
	application_id = _application_id;
	location_user_equipment = _location_user_equipment;
	
	node = nullptr;
}


Client::~Client()
{
}


string Client::get_name()
{
	return name;
}


int Client::get_application_id()
{
	return application_id;
}


void Client::set_location_point_of_access(string _location)
{
	location_point_of_access = _location;
}


string Client::get_location_point_of_access()
{
	return location_point_of_access;
}


void Client::set_location_zone(string _location)
{
	location_zone = _location;
}


string Client::get_location_zone()
{
	return location_zone;
}


string Client::get_location_user_equipment()
{
	return location_user_equipment;
}


void Client::set_node(Node * node)
{
	this->node = node;
}


Node * Client::get_node()
{
	return node;
}


int Client::assign_server(Server * server)
{
	active_servers.push_back(server);
	
	return 0;
}


int Client::remove_server(Server * server)
{
	for(auto server_iterator = active_servers.begin(); server_iterator != active_servers.end();)
	{
		if((*server_iterator)->get_name().compare(server->get_name()) == 0)
		{
			previous_servers.push_back(*server_iterator);
			
			server_iterator = active_servers.erase(server_iterator);
			
			return 0;
		}
		else
		{
			++server_iterator;
		}
	}
	
	return -1;
}


Server * Client::get_active_server(int index)
{
	return active_servers.at(index);
}


int Client::get_active_server_count()
{
	return active_servers.size();
}


Server * Client::get_previous_server(int index)
{
	return previous_servers.at(index);
}


int Client::get_previous_server_count()
{
	return previous_servers.size();
}

/*
double Client::get_score(Node * node)
{
	int number_of_disconnects = 0;
	
	for(auto server_iterator = previous_servers.begin(); server_iterator != previous_servers.end(); ++server_iterator)
	{
		if((*server_iterator)->get_application_id() == node->get_application_id()									// same application id
		&& (*server_iterator)->get_location_zone().compare(node->get_location_zone()) == 0 							// same zone
		&& (*server_iterator)->get_location_point_of_access().compare(node->get_location_point_of_access()) == 0)	// same point of access
		{
			number_of_disconnects++;
		}
	}
	
	return number_of_disconnects;
}
*/

void Client :: set_possible_poas_client(vector<Poa*>* possbile_poa_client)
{

	this->possbile_poa_client = possbile_poa_client;

}


vector<Poa*>* Client :: get_possible_poas_client()
{
	return possbile_poa_client ;
}
