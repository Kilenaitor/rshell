#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include "errno.h"
#include <string>
#include <vector>
#include <list>
#include "boost/tokenizer.hpp"

using namespace std;

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
        
        typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
        boost::char_separator<char> sep(" ");
        tokenizer tok(input, sep);
        
        string arg;
        list<string> ls;
        vector<char*> args;
        
        for(tokenizer::iterator it = tok.begin(); it != tok.end(); ++it) {
            ls.push_back(*it);
            args.push_back(const_cast<char*>(ls.back().c_str()));
        }
        args.push_back(0);
        
        //TODO: Filter Input
        //TODO: Grab arguments
    
        if(input == "exit") {
            exit(0);
        }
    
        //Main process thread
        pid_t i = fork();
        
        if(i < 0) { //Error
            perror("Failed to create child process");
            break;
        }
        else if(i == 0) { //Child process
            
            int com = execvp(args[0], &args[0]);
            if(com < 0)
                perror("Error executing command");
            else {
                exit(0);
            }
        }
        else { //Parent process
            int *status = nullptr;
            waitpid(i, status, 0); //Temp fix just to get child to run properly.
        }
	}
}
