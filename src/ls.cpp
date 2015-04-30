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
		
		total += ceil(fileStat.st_size/512);
		
		for(; fileStat.st_size != 0; fileStat.st_size /= 10, tempWidth++);
		tempWidth > width ? width = tempWidth : width;
	}
	
	cout << "total " << total << endl;
	
    for(auto x : v) {
		struct stat fileStat;
	    if(stat(x, &fileStat) < 0)  
			perror("Failed");
		
		printf( (S_ISDIR(fileStat.st_mode)) ? "d" : "-");
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
		printf("%d ",fileStat.st_nlink);
		printf("%s  ", pws->pw_name);
		printf("%s ", grp->gr_name);
		std::cout.width(width);
		cout << std::right << fileStat.st_size << " ";
		struct tm * timeinfo = localtime(&fileStat.st_ctime);
		printf("%s", asctime(timeinfo));
		
		cout << x << endl;
	}
}

void standard_output(vector<char*> &v, int length)
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int check_width = v.size()*length;
    int num_rows = ceil((double)check_width/(double)w.ws_col);
    
    for(int i = 0; i < num_rows; ++i)
    {
        for(int a = i; a < v.size(); a += num_rows) {
            cout << v.at(a);
            for(int i = strlen(v.at(a)); i < length; ++i)
                cout << " ";
        }
        cout << endl;
    }
}

int main (int argc, char const *argv[])
{
    DIR *dirp;
    string direc = "./";
    bool list = false;
    bool all = false;
    bool recursive = false;
    vector<char*> files;
    int max_length = 9;
    
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
    
    if(!list)
        standard_output(files, ++max_length);
    else
        list_output(files);
    
    if(-1 == closedir(dirp)) {
        perror("There was an error with closedir(). ");
        exit(1);
    }
    return 0;
}