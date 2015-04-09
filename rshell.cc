#include <iostream>
#include <string>
#include "boost/tokenizer.hpp"

using namespace std;
using namespace boost;

int main() {
	
	string input;
	
	while(getline(cin, input))
	{
		if(input == "ls")
			cout << "You typed ls" << endl;
		if(input == "exit")
		{
			cout << "Bye" << endl;
			sleep(1);
			break;
		}
	}
}
