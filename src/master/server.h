#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <vector>

#include "client.h"
class Client;

using namespace std;

/*enum class Server_state
{
	NO_CLIENTS,
	FULL
};*/

class Server
{
	private:
	
	string name;
	
	int application_id;
	
	string location_point_of_access;
	
	string location_zone;
	
	//Server_state state;
	
	vector<Client *> active_clients;
	
	public:
	
	Server(string _name, int _application_id, string _location_point_of_access, string _location_zone);
	~Server();
	
	string get_name();
	
	int get_application_id();
	
	string get_location_point_of_access();
	string get_location_zone();
	
	//Client_state get_state();
	
	int assign_client(Client * client);
	
	int remove_client(Client * client);
	
	Client * get_active_client(int index);
	int get_active_client_count();
};

#endif //SERVER_H