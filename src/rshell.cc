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
        string separator2(" ;");
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
            if(!string(*it).empty())
                cout << *it << endl;
            /*
            if(*it == "#")
                break;
            //If the current element is the start of a connector, this checks to see if the following index contains the other half of the connector
            else if( (*it == "|" && *next(it, 1) == "|" ) || (*it == "&" && *next(it, 1) == "&") ) {
                if(*next(it, 2) == "#")
                    break;
                it++;
                args.insert(args.begin(), const_cast<char*>(string("sh").c_str()));
                args.insert(++args.begin(), const_cast<char*>(string("-c").c_str()));
                args.push_back(const_cast<char*>(string("||").c_str()));
                args.push_back(const_cast<char*>(string(*next(it,1)).c_str()));
                args.push_back(0);
                commands.push_back(args);
                args.clear();
                it++;it++;
            }
            else if(*it == ";") {
                args.push_back(0);
                commands.push_back(args);
                args.clear();
            }
            else {
                ls.push_back(*it);
                args.push_back(const_cast<char*>(ls.back().c_str()));
            }
            */
        }
        break;
        if(!args.empty()) {
            args.push_back(0);
            commands.push_back(args);
        }
        
        //Go through all of the commands
        for(int x = 0; x < commands.size(); x++) {
            
            //Get the current command
            vector<char*> com = commands.at(x);
            if(com.empty())
                break;
            
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
                cout << com[0] << endl;
                cout << com[1] << endl;
                cout << com[2] << endl;
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
