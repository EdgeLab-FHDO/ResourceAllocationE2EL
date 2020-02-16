#include<iostream>
#include <unistd.h>

int main(int argc, char const* argv[])
{
	while(true)
	{
		std::cout << "test 1" << std::endl;
		sleep(1);
	}
	return 0;
}