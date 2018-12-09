
/*
    Author: Likhon D. Gomes
    Professor Jamie Payton
    Fall 2018
    Lab 4 File System
*/


#include <iostream>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>

#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

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


//struct definitions
typedef struct virtualfile{
    int start;
    int current;
    int end;
    string buffer;
} virtualfile;

typedef struct inode{
    int filenum;
    bool isDir;
    time_t createTime;
    time_t editedTime;
    int address;
}inode;

typedef struct inodetable{
    int first;
    int length;
}inodetable;


//functions
void format(int fs);
int makefile(char* path, char* filename);
virtualfile* openfile(char* path, char* filename);
int closefile(virtualfile *f);
int mkdir(char* path, char* dirname);
int rm(char* path, char* name);
int write(virtualfile* f, char* data);
int read(virtualfile* f, char* buf, int size);
int seek(virtualfile* f, int offset);
int getFileNumber(char* path, char* filename);
int addToInodeTable(inode file);
inode readfrominodetable(int filenum);
void seektoempty();
int seektodirectory(char* dirname);

//global variables
int fs;
char* map;
inodetable *iTable;
int nextFD;


int main(){

    fs = open(DISK_NAME, O_RDWR | O_CREAT | O_TRUNC, 0777);

    if(fs == -1){
        cout << "Failed to open the Virtual Disk" << endl;
        perror("Failed to open the virtual disk");
        exit(EXIT_FAILURE);
    } else {
        cout << "Opened Virtual Disk " << DISK_NAME << endl;
    }
    //formatting disk
    format(fs);






}


//Format function starts here
void format(int fs){
    cout << "format() called" << endl;

    int ret = lseek(fs, DISK_SIZE, SEEK_SET);
    if(ret==-1){
        cout << "lseek() failed on Virtual Disk" << endl;
        perror("lseek() failed on Virtual Disk");
        exit(EXIT_FAILURE);
    }
    ret = write(fs, "", 1);
        if(ret != 1){
            perror("writing failed on Virtual Disk");
            exit(EXIT_FAILURE);
        } else {
            cout << "Virtual Disk stretch successful upto " << DISK_SIZE << endl;
        }

    //mapping the file to memory
    mmap(0, DISK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fs, 0);
    
    if(map == MAP_FAILED){
        cout << "Mapping Virtual Disk Failed" << endl;
        perror("Mapping Virtual Disk Failed");
        exit(EXIT_FAILURE);
    } else {
        cout << "Virtual Disk Mapping Successful" << endl;
    }

    nextFD = 1;

    //iTable = malloc(sizeof(inodetable));
    iTable->first = 0;
    iTable->length = INODE_SIZE;
    
    lseek(fs, LOC_ROOT,SEEK_SET);
    ret = write(fs, "1",1);
    //create inode of root directory
    inode root;
    root.filenum = nextFD;
    root.isDir = true;
    time_t current;
    time(&current);
    root.createTime = current;
    root.editedTime = current;
    root.address = LOC_ROOT;

    lseek(fs,0,SEEK_SET);
    addToInodeTable(root);
    if(ret == -1){
        cout << "Error writing inode of root directory" << endl;
        perror("Error writing inode of root directory");
        exit(EXIT_FAILURE);
    } else {
        cout << "format Successful " << endl;
    }
}
//Format function ends here


int addToInodeTable(inode file) {
  //turn inode into a string
  char* rootstring;
  char* spacer = ",";
  char* end = "*";
  char* stringified;
  sprintf(stringified, "%d", file.filenum);
  strcpy(rootstring, stringified);
  strcat(rootstring, spacer);
  sprintf(stringified, "%d", file.isDir);
  strcat(rootstring, stringified);
  strcat(rootstring, spacer);
  sprintf(stringified, "%d", (int)file.createTime);
  strcat(rootstring, stringified);
  strcat(rootstring, spacer);
  sprintf(stringified, "%d", (int) file.editedTime);
  strcat(rootstring, stringified);
  strcat(rootstring, spacer);
  sprintf(stringified, "%d", file.address);
  strcat(rootstring, stringified);
  strcat(rootstring, end);
  //write the inode to the virtualdisk
  lseek(fs, 0, SEEK_SET);
  seektoempty();
  int result = write(fs, rootstring, strlen(rootstring));
  if(result == -1) {
    printf("Error writing inode.\n");
    perror("Error writing inode.");
    return(EXIT_FAILURE);
 }
 return 0;
}

void seektoempty() {
  char* buf;
  do {
    read(fs, buf, 1);
    lseek(fs, 0, SEEK_CUR);
  } while(strcmp(buf, "") != 0);
  lseek(fs, -1, SEEK_CUR);
 }



