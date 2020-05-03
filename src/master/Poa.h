#ifndef POA_H
#define POA_H


#include <string>

using namespace std;

class Poa
{
	private :

	string point_of_access_name ;

    string location_zone ;

	public :

    Poa(string _location_point_of_access , string _location_zone);
    
    string get_poa_name();

    string get_zone_of_poa();
};
#endif
