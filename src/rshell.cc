#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include "errno.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string>
#include <vector>
#include <list>
#include <iterator>
#include "boost/tokenizer.hpp"
#include "boost/algorithm/string.hpp"

using namespace std;

//Global used to keep track if the previous command executed successfully
//Used for the || and && connectors to see if they should execute
static bool *pass;

int main (int argc, char const *argv[])
{
    //Host name can only be 128 characters. If it's more than that, you need to reevaluate your naming choices
    char host[128];
    gethostname(host, sizeof(host));
    char * login = getlogin();

    //Input the user puts in
	string input;

    //mmap is used for preserving the variable (pass in this case) through the child processes
    pass = (bool *)mmap(NULL, sizeof *pass, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);

    //Setting the initial value to false since no command has run
    *pass = false;

    while(true)
	{
        //Terminal prompt display
        cout << login << "@" << host << " $ ";
        cout.flush(); //Have to flush since no std::endl;

        //Grab user input for bash prompt
        getline(cin,input);
        boost::trim(input); //Remove trailing and leading space

        //Setting up a boost tokenizer.
        typedef boost::tokenizer<boost::escaped_list_separator<char> > tokenizer;
        string separator1("");
        string separator2(" ");
        string separator3("\"\'"); //Used to count quoted text as a single token rather than delimit them separately
        boost::escaped_list_separator<char> sep(separator1,separator2,separator3);
        tokenizer tok(input, sep);

        //ls is a temp list for storing the tokenized value
        list<string> ls;
        //vector for storing a command and its arguments
        vector<char*> args;
        //vector for storing all of the commands that have been chained together
        vector<vector<char*> > commands;
        //vector for storing all of the connectors "AND" and "OR"
        vector<string> connectors;

        /*
        Check for # and immediatly end parsing since anything after it is a comment.
        Check for ; or || or && to know if a new command needs to be created.
        Put all of these new commands in the commands vector.
        */
        for(tokenizer::iterator it = tok.begin(); it != tok.end(); ++it) {
            if( !it->empty() ) {
                if(*it == "#" || strncmp(&it->at(0),"#",1) == 0)
                    break;
                else if(strncmp(&it->back(), "#", 1) == 0) {
                    string temp = it->substr(0, it->size()-1);
                    if(temp.size() > 1) {
                        connectors.push_back(" ");
                        ls.push_back(temp);
                        args.push_back(const_cast<char*>(ls.back().c_str()));
                    }
                    args.push_back(0);
                    commands.push_back(args);
                    args.clear();
                    break;
                }
                else if (*it == "||" ) {
                    if(args.empty()) continue;
                    connectors.push_back("OR");
                    //Adds the connector to the vector and then terminates the current command
                    args.push_back(0);
                    commands.push_back(args);
                    args.clear();
                }
                else if (*it == "&&") {
                    if(args.empty()) continue;
                    connectors.push_back("AND");
                    //Adds the connector to the vector and then terminates the current command
                    args.push_back(0);
                    commands.push_back(args);
                    args.clear();
                }
                //Checks to see if the last character (or only character) on the token is a semicolon
                else if(strncmp(&it->back(), ";", 1) == 0) {
                    if(args.empty()) continue;
                    string temp = it->substr(0, it->size()-1);
                    if(temp.size() > 1) {
                        //We only want to count this is a command if there is actually some command being executed
                        //So we check if its size is larger then 1 to check if there's something else other than
                        //  just the semicolon itself.
                        connectors.push_back(" ");
                        ls.push_back(temp);
                        args.push_back(const_cast<char*>(ls.back().c_str()));
                    }
                    args.push_back(0);
                    commands.push_back(args);
                    args.clear();
                }
                else {
                    //If the current token is not a comment, 'and', 'or', or a semicolon, add the token to the current command
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
        for(unsigned x = 0; x < commands.size(); x++) {

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
                *pass = false;
                break;
            }
            else if(i == 0) { //Child process
                //Used for skipping the execution of commands in the event that they're conditional
                //based on the connector used by the user.
                if(x > 0 && connectors.size() > 1 && connectors.at(x-1) == "OR" && *pass)
                    exit(0);
                else if(x > 0 && connectors.size() > 1 && connectors.at(x-1) == "AND" && !*pass)
                    exit(0);
                else {
                    int result = execvp(com[0], &com[0]);
                    if(result < 0) {
                        *pass = false;
                        //This is just the standard format that bash outputs error notices
                        char result[100];
                        const char *error = "-rshell: ";
                        strcpy(result, error);
                        strcat(result, com[0]);
                        perror(result);
                        //Exiting with 1 since the command failed
                        exit(1);
                    }
                    else {
                        *pass = true;
                        exit(0);
                    }
                }
            }
            else { //Parent process
                int status;
				if(wait(&status) < 0) {
					perror("Error during child process");
                    *pass = false;
					exit(1);
				}
                else {
                    //Used to get the exit code of the child process
                    int estat = WEXITSTATUS(status);

                    if(estat == 0) {
                        *pass = true;
                    }
                    if(estat == 1) {
                        *pass = false;
                    }

                    //Check to see if the child terminated by a signal
                    else if (WIFSIGNALED(status)) {
                        printf("%ld terminated because it didn't catch signal number %d\n", (long)i, WTERMSIG(status));
                        *pass = false;
                    }
                }
            }
        }
	}
}
