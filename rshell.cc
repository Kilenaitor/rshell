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
        boost::char_separator<char> sep(" ", ";");
        tokenizer tok(input, sep);
        
        list<string> ls;
        vector<char*> args;
        vector<vector<char*> > commands;
        
        for(tokenizer::iterator it = tok.begin(); it != tok.end(); ++it) {
            if(*it == "#")
                break;
            if(*it == ";" || *it == "||" || *it == "&&") {
                args.push_back(0);
                commands.push_back(args);
                args.clear();
            }
            else {
                ls.push_back(*it);
                args.push_back(const_cast<char*>(ls.back().c_str()));
            }
        }
        args.push_back(0);
        commands.push_back(args);
        
        for(int x = 0; x < commands.size(); x++) {
            
            vector<char*> com = commands.at(x);
            
            if(strncmp(com[0], "exit", 4) == 0) {
                exit(0);
            }
    
            //Main process thread
            pid_t i = fork();
        
            if(i < 0) { //Error
                perror("Failed to create child process");
                break;
            }
            else if(i == 0) { //Child process
            
                int result = execvp(com[0], &com[0]);
                if(result < 0)
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
}
