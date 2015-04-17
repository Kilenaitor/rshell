#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include "errno.h"
#include <string>
#include <vector>
#include <list>
#include <iterator>
#include "boost/tokenizer.hpp"
#include "boost/algorithm/string.hpp"

using namespace std;

int main (int argc, char const *argv[])
{   
    char host[128];
    gethostname(host, sizeof(host));
    char * login = getlogin();
    
	string input;
    vector<string> connectors;
    bool pass = true;
    
    /*char* arg[] = {"sh", "-c", "echo \"This is true\" || echo \"This is false\""};
    int result = execvp(arg[0], arg);
    cout << result << endl;
    exit(0);*/
    
    while(true)
	{
        //Terminal prompt display
        cout << login << "@" << host << " $ ";
        cout.flush();

        //Grab user input for bash prompt
        getline(cin,input);
        boost::trim(input);
        
        //Setting up a boost tokenizer.
        typedef boost::tokenizer<boost::escaped_list_separator<char> > tokenizer; 
//        typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
//        boost::char_separator<char> sep(" ", ";#|&"); //' ' (space) and " as delimiters
        string separator1("\\");
        string separator2(" ");
        string separator3("\"\'");
        boost::escaped_list_separator<char> sep(separator1,separator2,separator3);
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
            if( !it->empty() ) {
                if(*it == "#")
                    break;
                //If the current element is the start of a connector, this checks to see if the following index contains the other half of the connector
                else if (*it == "||" ) {
                    connectors.push_back("OR");
                    args.push_back(0);
                    commands.push_back(args);
                    args.clear();
                }
                else if (*it == "&&") {
                    connectors.push_back("AND");
                    args.push_back(0);
                    commands.push_back(args);
                    args.clear();
                }
                else if(strncmp(&it->back(), ";", 1) == 0) {
                    string temp = it->substr(0, it->size()-1);
                    if(temp.size() > 1) {
                        ls.push_back(temp);
                        args.push_back(const_cast<char*>(ls.back().c_str()));
                    }
                    args.push_back(0);
                    commands.push_back(args);
                    args.clear();
                }

                else {
                    connectors.push_back(" ");
                    ls.push_back(*it);
                    args.push_back(const_cast<char*>(ls.back().c_str()));
                }
            }
        }
        if(!args.empty()) {
            connectors.push_back(" ");
            args.push_back(0);
            commands.push_back(args);
        }
        
        //Go through all of the commands
        for(int x = 0; x < commands.size(); x++) {
            
            //Get the current command
            vector<char*> com = commands.at(x);
            if(com.empty())
                continue;
            
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
/*                
                if(connectors.at(x+1) == "OR" && pass)
                    exit(0);
                if(connectors.at(x+1) == "AND" && !pass)
                    exit(0);
*/
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
                    pass = false;
					exit(1);
				}
                else {
                    pass = true;
                }
            }
        }
	}
}
