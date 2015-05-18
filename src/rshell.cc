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
#include <fstream>
#include <fcntl.h>
#include "boost/tokenizer.hpp"
#include "boost/algorithm/string.hpp"

using namespace std;

//Global used to keep track if the previous command executed successfully
//Used for the || and && connectors to see if they should execute
static bool *pass;

//ls is a temp list for storing the tokenized value
list<string> ls;
//vector for storing a command and its arguments
vector<char*> args;
//vector for storing all of the commands that have been chained together
vector<vector<char*> > commands;
//vector for storing all of the connectors "AND" and "OR"
vector<string> connectors;
//vector for storing files that need to get the output of current command and overwrite/create
vector<char*> out_files_r;
//vector for storing files that need to get the output of current command and overwrite/create
vector<char*> out_files_a;
//vector for storing files that supply input to the current command
vector<char*> in_files;
//int for storing the number of pipes the user has entered
int num_pipes = 0;

void pipe_help(int num_pipes, int pipes[], vector<vector<char*> > &commands, int curr_index)
{
    pid_t i = fork();
    if(i < 0) { //Error
        perror("Failed to create child process");
        *pass = false;
        exit(1);
    }
    if(i == 0) {
        int read_val = 2 * curr_index - 2;
        if(read_val >= 0)
            dup2(pipes[read_val], 0);
        
        int write_val = 2 * curr_index + 1;
        if(write_val < num_pipes*2)
            dup2(pipes[write_val], 1);
        
        for(int pipe_loop = 0; pipe_loop < num_pipes*2; pipe_loop++)
            close(pipes[pipe_loop]);
        
        vector<char*> com = commands[curr_index];
        
        if(in_files.size() > curr_index && in_files.at(curr_index) != 0) {
            int fd0 = open(in_files.at(curr_index), O_RDONLY, 0);
            if(-1 == dup2(fd0, STDIN_FILENO)) {
                perror("Error opening file for writing");
                return;
            }
            close(fd0);
        }
        if(out_files_r.size() > curr_index && out_files_r.at(curr_index) != 0) {
            int fd1 = open(out_files_r.at(curr_index), O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
            if(-1 == dup2(fd1, STDOUT_FILENO)) {
                perror("Error opening file for writing");
                return;
            }
            close(fd1);
        }
        if(out_files_a.size() > curr_index && out_files_a.at(curr_index) != 0) {
            int fd1 = open(out_files_a.at(curr_index), O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
            if(-1 == dup2(fd1, STDOUT_FILENO)) {
                perror("Error opening file for writing");
                return;
            }
            close(fd1);
        }
        //Used for skipping the execution of commands in the event that they're conditional
        //based on the connector used by the user.
        if(curr_index > 0 && connectors.size() > 1 && connectors.at(curr_index-1) == "OR" && *pass)
            exit(0);
        else if(curr_index > 0 && connectors.size() > 1 && connectors.at(curr_index-1) == "AND" && !*pass)
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
    else {
        if(curr_index < num_pipes) {
            pipe_help(num_pipes, pipes, commands, ++curr_index);
        }
    }
}

int main (int argc, char const *argv[])
{
    //Host name can only be 128 characters. If it's more than that, you need to reevaluate your naming choices
    char host[128];
    int h = gethostname(host, sizeof(host));
    if(h != 0) {
        perror("Error retrieving hostname.");
    }
    char * login = getlogin();
    if(!login) {
        perror("Error getting login name.");
    }

    //Input the user puts in
    string input;

    //mmap is used for preserving the variable (pass in this case) through the child processes
    pass = (bool *)mmap(NULL, sizeof *pass, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    if(pass == MAP_FAILED) {
        perror("Failed to save return state of process. rshell will not work properly.");
    }

    //Setting the initial value to false since no command has run

    while(true)
    {
        *pass = false;
        
        ls.clear();
        //vector for storing a command and its arguments
        args.clear();
        //vector for storing all of the commands that have been chained together
        commands.clear();
        //vector for storing all of the connectors "AND" and "OR"
        connectors.clear();
        //vector for storing files that need to get the output of current command and overwrite/create
        out_files_r.clear();
        //vector for storing files that need to get the output of current command and overwrite/create
        out_files_a.clear();
        //vector for storing files that supply input to the current command
        in_files.clear();
        //int for storing the number of pipes the user has entered
        int num_pipes = 0;
        
        bool in_add = false;
        bool out_r_add = false;
        bool out_a_add = false;
        
        fflush(0); //Start with clean stdout and stdin and stderror
        
        //Terminal prompt display
        if(h == 0 && login)
            cout << login << "@" << host << " $ ";
        else
            cout << "$ ";
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

        /*
        Check for # and immediatly end parsing since anything after it is a comment.
        Check for ; or || or && to know if a new command needs to be created.
        Put all of these new commands in the commands vector.
        */
        for(tokenizer::iterator it = tok.begin(); it != tok.end(); ++it) {
            in_add = false;
            out_r_add = false;
            out_a_add = false;
            
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
                    if(!in_add)
                        in_files.push_back(0);
                    if(!out_r_add)
                        out_files_r.push_back(0);
                    if(!out_a_add)
                        out_files_a.push_back(0);
                    args.clear();
                    break;
                }
                else if (*it == "||" ) {
                    if(args.empty()) continue;
                    connectors.push_back("OR");
                    //Adds the connector to the vector and then terminates the current command
                    args.push_back(0);
                    commands.push_back(args);
                    if(!in_add)
                        in_files.push_back(0);
                    if(!out_r_add)
                        out_files_r.push_back(0);
                    if(!out_a_add)
                        out_files_a.push_back(0);
                    args.clear();
                }
                else if (*it == "&&") {
                    if(args.empty()) continue;
                    connectors.push_back("AND");
                    //Adds the connector to the vector and then terminates the current command
                    args.push_back(0);
                    commands.push_back(args);
                    if(!in_add)
                        in_files.push_back(0);
                    if(!out_r_add)
                        out_files_r.push_back(0);
                    if(!out_a_add)
                        out_files_a.push_back(0);
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
                    if(!in_add)
                        in_files.push_back(0);
                    if(!out_r_add)
                        out_files_r.push_back(0);
                    if(!out_a_add)
                        out_files_a.push_back(0);
                    args.clear();
                }
                else if (*it == "|") {
                    if(args.empty()) continue;
                    num_pipes++;
                    //Adds the connector to the vector and then terminates the current command
                    args.push_back(0);
                    commands.push_back(args);
                    if(!in_add)
                        in_files.push_back(0);
                    if(!out_r_add)
                        out_files_r.push_back(0);
                    if(!out_a_add)
                        out_files_a.push_back(0);
                    args.clear();
                }
                else {
                    if(*it == "<") {
                        auto tempit = it;
                        tempit++;
                        if(tempit == tok.end()) {
                            perror("Unexpected newline");
                            continue;
                        }
                        ls.push_back(*tempit);
                        if (in_files.size() == 0) // Replace the in_file for the command if they are strung together `cat < in.txt < in2.txt < in3.txt` would just be `cat < in3.txt`
                            in_files.push_back(const_cast<char*>(ls.back().c_str()));
                        else
                            in_files.at(in_files.size()-1) = const_cast<char*>(ls.back().c_str());
                        it++; //Advance to outfile to skip over that in the command processing
                        if(strncmp(&it->back(), ";", 1) == 0) {
                            args.push_back(0);
                            commands.push_back(args);
                            args.clear();
                            string temp = it->substr(0, it->size()-1);
                            ls.push_back(temp);
                            in_files.at(in_files.size()-1) = const_cast<char*>(ls.back().c_str());
                        }
                        in_add = true;
                        if(!out_r_add)
                            out_files_r.push_back(0);
                        if(!out_a_add)
                            out_files_a.push_back(0);
                        continue;
                    }
                    if(*it == ">") {
                        auto tempit = it;
                        tempit++;
                        if(tempit == tok.end()) {
                            perror("Unexpected newline");
                            continue;
                        }
                        ls.push_back(*tempit);
                        if(out_files_r.size() == 0)
                            out_files_r.push_back(const_cast<char*>(ls.back().c_str()));
                        else
                            out_files_r.at(out_files_r.size()-1) = const_cast<char*>(ls.back().c_str());
                        it++;
                        if(strncmp(&it->back(), ";", 1) == 0) {
                            args.push_back(0);
                            commands.push_back(args);
                            args.clear();
                            string temp = it->substr(0, it->size()-1);
                            ls.push_back(temp);
                            out_files_r.at(out_files_r.size()-1) = const_cast<char*>(ls.back().c_str());
                        }
                        out_r_add = true;
                        if(!in_add)
                            in_files.push_back(0);
                        if(!out_a_add)
                            out_files_a.push_back(0);
                        continue;
                    }
                    if(*it == ">>") {
                        auto tempit = it;
                        tempit++;
                        if(tempit == tok.end()) {
                            perror("Unexpected newline");
                            exit(1);
                        }
                        ls.push_back(*tempit);
                        if(out_files_a.size() == 0)
                            out_files_a.push_back(const_cast<char*>(ls.back().c_str()));
                        else
                            out_files_a.at(out_files_a.size()-1) = const_cast<char*>(ls.back().c_str());
                        it++;
                        if(strncmp(&it->back(), ";", 1) == 0) {
                            args.push_back(0);
                            commands.push_back(args);
                            args.clear();
                            string temp = it->substr(0, it->size()-1);
                            ls.push_back(temp);
                            out_files_a.at(out_files_a.size()-1) = const_cast<char*>(ls.back().c_str());
                        }
                        out_a_add = true;
                        if(!in_add)
                            in_files.push_back(0);
                        if(!out_r_add)
                            out_files_r.push_back(0);
                        continue;
                    }
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
        
/*        for(auto x : in_files) {
            if(x != 0)
                cout << x << endl;
            else
                cout << "0" << endl;
        }
        cout << endl;
        
        for(auto x : out_files_r) {
            if(x != 0)
                cout << x << endl;
            else
                cout << "0" << endl;
        }
        cout << endl;
        
        for(auto x : out_files_a) {
            if(x != 0)
                cout << x << endl;
            else
                cout << "0" << endl;
        }
        cout << endl;
        */

        
        if(num_pipes > 0) {

            int status;
            
            int pipe_size = num_pipes * 2;
            int* pipes;
            try {
                 pipes = new int[pipe_size];   
            }
            catch (std::bad_alloc& ba) {
                std::cerr << "bad_alloc caught: " << ba.what() << endl;
            }
            
            for(int dup_pipes = 0; dup_pipes < num_pipes*2; dup_pipes += 2) {
                pipe(pipes + dup_pipes);
            }

            pipe_help(num_pipes, pipes, commands, 0);
            
            for(int pipe_loop = 0; pipe_loop < pipe_size; pipe_loop++)
                close(pipes[pipe_loop]);
            
            delete [] pipes;
            
            for(int i = 0; i < num_pipes+1; i++) {
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
            continue;
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
                if(in_files.size() > x && in_files.at(x) != 0) {
                    int fd0 = open(in_files.at(x), O_RDONLY, 0);
                    if(-1 == dup2(fd0, STDIN_FILENO)) {
                        perror("Error opening file for writing");
                        continue;
                    }
                    close(fd0);
                }
                if(out_files_r.size() > x && out_files_r.at(x) != 0) {
                    int fd1 = open(out_files_r.at(x), O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
                    if(-1 == dup2(fd1, STDOUT_FILENO)) {
                        perror("Error opening file for writing");
                        continue;
                    }
                    close(fd1);
                }
                if(out_files_a.size() > x && out_files_a.at(x) != 0) {
                    int fd1 = open(out_files_a.at(x), O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
                    if(-1 == dup2(fd1, STDOUT_FILENO)) {
                        perror("Error opening file for writing");
                        continue;
                    }
                    close(fd1);
                }
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
