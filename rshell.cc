#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include "errno.h"
#include <string>
#include "boost/tokenizer.hpp"

using namespace std;
using namespace boost;

int main (int argc, char const *argv[])
{   
    char host[128];
    gethostname(host, sizeof(host));
    char * login = getlogin();
    
	string input;
    
    while(true)
	{
        //Terminal prompt display
        cout << login << "@" << host << " $ ";
        cout.flush();

        //Grab user input for bash prompt
        getline(cin,input);
    
        if(argv[1] == "exit")
            break;
    
        //Main process thread
        pid_t i = fork();
        if(i == 0) 
        {
            char* arg[] = {"ls", "-l", NULL};
            /*int com = execvp(arg[0], arg);
            if(com < 0)
                perror("Error executing command.");
            */
            cout << "Made it here" << endl;
        }
    
	}
}