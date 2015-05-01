#include <iostream>
#include <fstream>
#include <stdio.h>
#include <unistd.h>
#include "errno.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string>
#include <vector>
#include <list>
#include <iterator>
#include <fcntl.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <cmath>
#include <string.h>
#include <sys/stat.h>
#include <grp.h>
#include <pwd.h>
#include <time.h>
#include <locale.h>
#include <langinfo.h>
#include <algorithm>

using namespace std;

void list_output(vector<char*> &v)
{
	struct passwd *pws;
	pws = getpwuid(geteuid());
	
	struct group *grp;
	grp = getgrgid(getgid());
	
	int width = 0;
	int total = 0;
	
	for(auto x : v) {
		int tempWidth = 0;
		struct stat fileStat;
		if(stat(x, &fileStat) < 0)
			perror("Failed");
		
		total += ceil(fileStat.st_blocks);
		
		for(; fileStat.st_size != 0; fileStat.st_size /= 10, tempWidth++);
		tempWidth > width ? width = tempWidth : width;
	}
	
	cout << "total " << total/2 << endl;
	
    for(auto x : v) {
		struct stat fileStat;
	    if(stat(x, &fileStat) < 0)  
			perror("Failed");
        
        bool directory = (S_ISDIR(fileStat.st_mode));
        bool executable = (fileStat.st_mode > 0) && (S_IEXEC & fileStat.st_mode);
		bool hidden = (x[0] == '.');
        
		printf( directory ? "d" : "-");
	    printf( (fileStat.st_mode & S_IRUSR) ? "r" : "-");
	    printf( (fileStat.st_mode & S_IWUSR) ? "w" : "-");
	    printf( (fileStat.st_mode & S_IXUSR) ? "x" : "-");
	    printf( (fileStat.st_mode & S_IRGRP) ? "r" : "-");
	    printf( (fileStat.st_mode & S_IWGRP) ? "w" : "-");
	    printf( (fileStat.st_mode & S_IXGRP) ? "x" : "-");
	    printf( (fileStat.st_mode & S_IROTH) ? "r" : "-");
	    printf( (fileStat.st_mode & S_IWOTH) ? "w" : "-");
	    printf( (fileStat.st_mode & S_IXOTH) ? "x" : "-");
		printf("  ");
        cout << fileStat.st_nlink << " ";
		printf("%s  ", pws->pw_name);
		printf("%s  ", grp->gr_name);
		cout.width(width);
		cout << std::right << fileStat.st_size << " ";
        
        struct tm *tm;
        char datestring[256];
        tm = localtime(&fileStat.st_mtime);
        strftime(datestring, sizeof(datestring), nl_langinfo(D_T_FMT), tm);
        printf("%s ", datestring);
		
        if(directory) {
            if(hidden)
                cout << "\x1b[34;47m" << x << "\x1b[0m" << endl;
            else
                cout << "\x1b[34;40m" << x << "\x1b[0m" << endl;
        }
        else if(executable) {
            if(hidden)
                cout << "\x1b[1;32;47m" << x << "\x1b[0m" << endl;
            else
                cout << "\x1b[1;32;40m" << x << "\x1b[0m" << endl;
        }
        else if(hidden)
            cout << "\x1b[30;47m" << x << "\x1b[0m" << endl;
        else
            cout << x << endl;
	}
}

void standard_output(vector<char*> &v, int length)
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int check_width = v.size()*length-1;
    int num_rows = ceil((double)check_width/(double)w.ws_col);
    
    for(int i = 0; i < num_rows; ++i)
    {
        for(unsigned a = i; a < v.size(); a += num_rows) {
            
    		struct stat fileStat;
            
            bool directory = (S_ISDIR(fileStat.st_mode));
            bool executable = (fileStat.st_mode > 0) && (S_IEXEC & fileStat.st_mode);
    		bool hidden = ((v.at(a))[0] == '.');
            
            if(directory) {
                if(hidden)
                    cout << "\x1b[34;47m" << v.at(a) << "\x1b[0m";
                else
                    cout << "\x1b[34;40m" << v.at(a) << "\x1b[0m";
            }
            else if(executable) {
                if(hidden)
                    cout << "\x1b[1;32;47m" << v.at(a) << "\x1b[0m";
                else
                    cout << "\x1b[1;32;40m" << v.at(a) << "\x1b[0m";
            }
            else if(hidden)
                cout << "\x1b[30;47m" << v.at(a) << "\x1b[0m";
            else
                cout << v.at(a);
            
            for(int i = strlen(v.at(a)); i < length && a < (v.size() - num_rows); ++i)
                cout << " ";
        }
        cout << endl;
    }
}

bool comparisonFunc(const char *c1, const char *c2) {
    return strcasecmp(c1, c2) < 0;
}

int main (int argc, char const *argv[])
{
    DIR *dirp;
    string direc = "./";
    bool list = false;
    bool all = false;
    bool recursive = false;
    vector<char*> files;
    unsigned max_length = 9;
    
    if(argc == 2) {
        string first = argv[1];
        if(first[0] == '-') {
            if(first.find('a') != string::npos)
                all = true;
            if(first.find('l') != string::npos)
                list = true;
            if(first.find('R') != string::npos)
                recursive = true;
        }
        else
            direc = argv[1];
    }
    
    if(argc > 2) {
        direc = argv[2];
    }
    
    if(NULL == (dirp = opendir(const_cast<char*>(direc.c_str())))) {
        perror("There was an error with opendir(). ");
        exit(1);
    }
    
    
    struct dirent *filespecs;
    errno = 0;
    while(NULL != (filespecs = readdir(dirp))) {
        if(filespecs->d_name[0] == '.' && !all)
            continue;
        files.push_back(filespecs->d_name);
        strlen(filespecs->d_name) > max_length ? max_length = strlen(filespecs->d_name) : max_length;
    }
    if(errno != 0) {
        perror("There was an error with readdir(). ");
        exit(1);
    }
    
    sort(files.begin(), files.end(), comparisonFunc);
    
    if(!list)
        standard_output(files, ++max_length);
    else
        list_output(files);
    
    if(recursive)
        cout << "rec" << endl;
    
    if(-1 == closedir(dirp)) {
        perror("There was an error with closedir(). ");
        exit(1);
    }
    return 0;
}