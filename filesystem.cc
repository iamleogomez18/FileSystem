
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
inode readFromInodeTable(int filenum);
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

    iTable = (inodetable*)malloc(sizeof(inodetable));
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


 int makefile(char* path, char* filename) {
  printf("makefile() called");
  //make sure file name is of valid length
  if(strlen(filename) < FILENAME_MIN_SIZE) {
    cout << "ERROR: Filename " << filename << " too short" << endl;
    return 1;
  }
  else if(strlen(filename) > FILENAME_MAX_SIZE) {
    cout << "ERROR: Filename " << filename << " is too long" << endl;
    return 1;
  }
  //find an empty memory spot for the file to live
  int result;
  int loc;
  loc = lseek(fs, LOC_ROOT, SEEK_SET);
  char buf[1];
  //read the first byte of each memory location to see if it's available
  read(fs, buf, 1);
  while(strcmp(buf, "") != 0) {
    loc = lseek(fs, BLOCK_SIZE - 1, SEEK_CUR);
    read(fs, buf, 1);
  }
  printf("Saving file at memory location 0x%x.\n", loc);
  //write a non-zero byte at start of block so it's known that this block is taken
  lseek(fs, loc, SEEK_SET);
  result = write(fs, "1", 1);

  //create inode entry
  inode file;
  time_t current;
  time(&current);
  file.createTime = current;
  file.editedTime = current;
  file.isDir = false;
  file.filenum = ++nextFD;
  file.address = loc;
  printf("Creating file with inode:\n   created = %d\n   edited = %d\n   isDir = %d\n   filenum = %d\n   addr = %d\n", (int)file.createTime, (int)file.editedTime, file.isDir, file.filenum, file.address);

  //add file pair to directory
  //filepair is a string of "filename , #"
  char* filepair = (char*) malloc((FILENAME_MAX_SIZE + 2)*sizeof(char) + sizeof(int));
  char* filepairspacer = ",";
  strcpy(filepair, filepairspacer);
  strcat(filepair, filename);
  strcat(filepair, filepairspacer);
  char* fnumstring = (char*) malloc(16);
  sprintf(fnumstring, "%d", file.filenum);
  strcat(filepair, fnumstring);
  //if the path specified is the root (specified by "/" or just ""), navigate to it
  if(strcmp(path,"/") == 0 || strcmp(path,"") == 0) {
    loc = lseek(fs, LOC_ROOT, SEEK_SET);
  }
  else {
    //the path specified isn't the root, so we have to find the correct directory    
    seektodirectory(path);
  }
  //find next blank byte in the current directory to put the file pair
  seektoempty();
  result = write(fs, filepair, strlen(filepair)*sizeof(char));
  //check if write to directory worked
  if(result == -1) {
    printf("Error writing file %s to virtual filesystem.\n", filename);
    perror("Error writing file in my_mkfile().");
    return(EXIT_FAILURE);
  }
  else {
    printf("Successfully wrote file '%s' to directory '%s'.\n", filename, path);
  }
  //save file inode to inode table in virtual filesystem
  addToInodeTable(file);

  return 0;
}

int seekToDirectory(char* path){
    char* pdup = strdup(path);
    char** pathArray = (char**) malloc(strlen(path)* sizeof(char));
    char* pathElement = strtok(pdup, "/");
    pathArray[0] = ""; //this is the root

    int i;
    for(i = 1; pathElement != NULL; i++){
        pathArray[i] = pathElement;
        pathElement = strtok(NULL, "/");
    }
    i--;

    lseek(fs, LOC_ROOT, SEEK_SET);

    int j, dirNum;
    char* childDir;
    char* currentDir = (char*)malloc(512);

    for(j = 0;j<i; j++){
        read(fs, currentDir, 512);
        childDir = strstr(currentDir, pathArray[j+1]);
        if(childDir == NULL){
            cout << "ERROR: Directory " << pathArray[j+1] << "not found in " << currentDir << endl;
            return(EXIT_FAILURE);
        }

        while(*childDir != ','){
            childDir++;
        }
        childDir++;
        dirNum = *childDir - '0';
        inode next = readFromInodeTable(dirNum);
        int nextAddr = next.address;
        lseek(fs, nextAddr, SEEK_SET);
    }
    return 0;
}

inode readFromInodeTable(int fileNumber){
    inode in;
    lseek(fs,0,SEEK_SET);
    char* iTable = (char*) malloc(2048);
    read(fs, iTable, 2048);
    char** inodesArray = (char**) malloc(strlen(iTable)*sizeof(char));
    char* intok = strtok(iTable, "*");
    int i;
    for(i = 0; intok != NULL && strcmp(intok, "\0") !=0; i++){
        inodesArray[i] = intok;
        intok = strtok(NULL, "*");
    }

    int inodeNum, j;
    for(j =0; j<<i; j++){
        inodeNum = inodesArray[j][0] - '0';
        if(inodeNum == fileNumber){
            break;
        }
    }
    in.filenum = atoi(strtok(inodesArray[j],","));
    //in.isDir = atoi(strtok)
    in.createTime = atoi(strtok(NULL,","));
    in.editedTime = atoi(strtok(NULL,","));
    in.address = atoi(strtok(NULL,","));
    return in;
}
