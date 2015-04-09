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
        
        //Setting up a boost tokenizer. 
        typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
        boost::char_separator<char> sep(" ", ";"); //' ' (space) as delmieter
        tokenizer tok(input, sep);
        
        //ls is a temp list for storing the tokenized value
        list<string> ls;
        //vector for storing a command and its arguments
        vector<char*> args;
        //vector for storing all of the commands that have been chained together
        vector<vector<char*> > commands;
        
        /*
        Check for # and immediatly end parsing since anything after it is a comment.
        Check for ; or || or && to know if a new command needs to be created.
        Put all of these new commands in the commands vector.
        */
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
        
        //Go through all of the commands
        for(int x = 0; x < commands.size(); x++) {
            
            //Get the current command
            vector<char*> com = commands.at(x);
            
            //Using string compare here since they're char * entries
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
                if(result < 0) {
                    char result[100];
                    const char *error = "-rshell: ";
                    strcpy(result, error);
                    strcat(result, com[0]);
                    perror(result);
					exit(1);
                }
                else {
                    exit(0);
                }
            }
            else { //Parent process
                int *status = nullptr;
                waitpid(i, status, 0); //Temp fix just to get child to run properly.
				if(status < 0) {
					perror("Error during child process");
					exit(1);
				}
            }
        }
	}
}
