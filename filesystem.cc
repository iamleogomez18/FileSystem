
/*
    Author: Likhon D. Gomes
    Professor Jamie Payton
    Fall 2018
    Lab 4 File System
*/


#include <iostream>
#include <string.h>
#include <time.h>

using namespace std;

#define DISK_SIZE 1048576
#define DISK_NAME "filesystem"
#define LOC_INODES 64
#define INODE_SIZE 32
#define LOC_ROOT 2048

#define MAX_FILE_SIZE 32768
#define BLOCK_SIZE 512
#define FILENAME_MIN_SIZE 11
#define FILENAME_MAX_SIZE 32


struct virtualfile{
    int start;
    int current;
    int end;
    string buffer;
};

struct inode{
    int filenum;
    bool isDir;
    time_t createTime;
    time_t editedTime;
};

struct inodetable{
    int first;
    int length;
};


void format(int fs);
int mkFile(string path, string filename);
virtualfile *openfile(string path, string filename);
int closefile(virtualfile* f);
int mkdir(string path, string dirName);

int main(){

}

