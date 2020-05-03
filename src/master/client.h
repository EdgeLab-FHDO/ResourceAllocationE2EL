#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <vector>

#include "server.h"
#include "node.h"
#include "Poa.h"
class Server;
class Node;


using namespace std;

/*enum class Client_state
{
	NO_SERVER_NEEDED,
	BACKUP_SERVER_NEEDED,
	MAIN_SERVER_NEEDED
};*/

class Client
{
	private:
	
	string name;
	
	int application_id;
	
	string location_point_of_access;
	
	string location_zone;
	
	string location_user_equipment;
	
	//Client_state state;
	
	vector<Server *> active_servers;
	
	vector<Server *> previous_servers;
	
	vector<Poa*>* possbile_poa_client;

	Node * node;
	
	public:
	
	Client(string _name, int _application_id, string _location_user_equipment);
	~Client();
	
	string get_name();
	
	int get_application_id();
	
	void set_location_point_of_access(string _location);
	string get_location_point_of_access();
	
	void set_location_zone(string _location);
	string get_location_zone();
	
	string get_location_user_equipment();
	
	void set_possible_poas_client(vector<Poa*>* possbile_poa_client);//new change1

	vector<Poa*>* get_possible_poas_client();

	//Client_state get_state();
	
	void set_node(Node * node);
	Node * get_node();
	
	int assign_server(Server * server);
	
	int remove_server(Server * server);
	
	Server * get_active_server(int index);
	int get_active_server_count();
	
	Server * get_previous_server(int index);
	int get_previous_server_count();
	
	//double get_score(Node * node);
};

#endif //CLIENT_H
