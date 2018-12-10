
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
int closeFile(virtualfile *vf);
int mkdir(char* path, char* dirname);
int rm(char* path, char* name);
int write(virtualfile* f, char* data);
int read(virtualfile* f, char* buf, int size);
int seek(virtualfile* f, int offset);
int getFileNumber(char* path, char* filename);
int addToInodeTable(inode file);
inode readFromInodeTable(int filenum);
void seekEmpty();
int seekDirectory(char* dirname);

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
  seekEmpty();
  int result = write(fs, rootstring, strlen(rootstring));
  if(result == -1) {
    printf("Error writing inode.\n");
    perror("Error writing inode.");
    return(EXIT_FAILURE);
 }
 return 0;
}

void seekEmpty() {
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
    seekDirectory(path);
  }
  //find next blank byte in the current directory to put the file pair
  seekEmpty();
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

int seekDirectory(char* path){
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

virtualfile* openFile(char* path, char* filename) {
  cout << "openFile() called" << endl;
  //get the file's address from its inode
  int filenumber = getFileNumber(path, filename);
  inode in = readFromInodeTable(filenumber);
  int addr = in.address;
  //navigate to file in myfs
  int loc = lseek(fs, addr, SEEK_SET);
  virtualfile *vf = (virtualfile*)malloc(sizeof(virtualfile));
  vf->current = loc;
  vf->start = loc;
  vf->end = addr + BLOCK_SIZE -1;
  printf("Opened virtualfile with vf.start = 0x%x,  vf.current = 0x%x and vf.end = 0x%x\n", vf->start, vf->current, vf->end);
  return vf;
}


int closeFile(virtualfile* vf) {
  cout << "closeFile() was called" << endl;
  if(vf != NULL) {  
    cout << "closed file at " << vf->start << endl;
    vf = NULL;
    return 0;
  }
  else {
    cout << "could not close since no file is open " << endl;
    return(EXIT_FAILURE);
  }
}

//function to make directory
int makeDir(char* path, char* dirname) {
    cout << "makeDir() called";
  //make sure file name is of valid length
  if(strlen(dirname) < 3) {
    cout << "Directory name " << dirname << " too short" << endl;
    return 1;
  }
  else if(strlen(dirname) > FILENAME_MAX_SIZE) {
    cout << "Directory name " << dirname << " too long" << endl;
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
  cout << "Creating directory at " << loc << endl; 
  //write a non-zero byte at start of block so it's known that this block is taken
  lseek(fs, loc, SEEK_SET);
  result = write(fs, "1", 1);

  //create inode entry
  inode file;
  time_t current;
  time(&current);
  file.createTime = current;
  file.editedTime = current;
  file.isDir = 1;
  file.filenum = ++nextFD;
  file.address = loc;

    cout << "Creating directory with inode: Created Time: " << file.createTime <<" editedTime " << file.editedTime << " isDir " << file.isDir << " File Number: " << file.filenum << " Address " << file.address << endl;
  //add file pair to directory
  //filepair is a string of "filename , #"
  char* filepair = (char*)malloc((FILENAME_MAX_SIZE + 2)*sizeof(char) + sizeof(int));
  char* filepairspacer = ",";
  strcpy(filepair, filepairspacer);
  strcat(filepair, dirname);
  strcat(filepair, filepairspacer);
  char* fnumstring = (char*)malloc(16);
  sprintf(fnumstring, "%d", file.filenum);
  strcat(filepair, fnumstring);
  //if the path specified is the root (specified by "/" or just ""), navigate to it
  if(strcmp(path,"/") == 0 || strcmp(path,"") == 0) {
    loc = lseek(fs, LOC_ROOT, SEEK_SET);
  }
  else {
    //the path specified isn't the root, so we have to find the correct directory    
    seekDirectory(path);
  }
  //find next blank byte in the current directory to put the file pair
  seekEmpty();
  result = write(fs, filepair, strlen(filepair)*sizeof(char));
  //check if write to directory worked
  if(result == -1) {
    cout << "Error writing directory " << dirname << " to virtual filysystem" << endl;
    perror("Error writing directory in my_mkdir().");
    return(EXIT_FAILURE);
  }
  else {
    cout << "Successfully wrote directory " << dirname << " to " << path << endl;
  }
  //save directory inode to inode table in virtual filesystem
  addToInodeTable(file);
  return 0;
}

int rm(char* path, char* name) {
  printf("\n---FUNCTION: my_rm()---\n");
  seekDirectory(path);
  char* currentdir = (char*)malloc(512);
  read(fs, currentdir, 512);
  //find (file, filenumber) pair so we can remove it 
  char* filepair = strstr(currentdir, name);
  if(filepair == NULL) {
      cout << "Could not find " << name << " in " << currentdir;

    return(EXIT_FAILURE);
   }
  //overwrite file name & number with \0
  while(*filepair != ',') {
    *filepair = '\0';
    filepair++;
  }
  printf("Deleted entry for file '%s' in directory '%s'.\n", name, path);
  return 0;
}

int write(virtualfile* f, char* data) {
    cout << "write() called" << endl;
  //navigate to current location within file
  lseek(fs, f->current, SEEK_SET);
  //make sure we aren't going to overflow into the next file
  int bytesrem = f->end - f->current;
  if(strlen(data) > bytesrem) {
    cout << "Not enough memory for the file to write " << endl;
    return(EXIT_FAILURE);
  }
  int result;
  result = write(fs, data, strlen(data));  
  //check if write to file worked
  if(result == -1) {
    cout << "Error writing " << data << " to file" << endl;
    return(EXIT_FAILURE);
  }
  else {
    cout << "successfully wrote data " << data << " to file" << endl;
  }
 
  //update current location in virtualfile
  f->current = f->current + strlen(data);

  return 0;
}

int read(virtualfile* f, char* buf, int size) {
  cout << "read() called" << endl;
  //navigate to current location within file
  lseek(fs, f->current, SEEK_SET);
  //check if we're reading past end of file
  if(size > (f->end - f->current)) {
    cout << "Error reading past end of file" << endl;
    return(EXIT_FAILURE);
  }
  //read data into buf
  cout << "Reading data from memory locations " << f->current << " to " << f->current + size;
  read(fs, buf, size);
  cout << " successfully read content " << buf << " from file" << endl;
  //update f->current
  f->current = f->current + size;
  return 0;
}
// seeking to a location within a file, at a specified offset from the file's first byte
int seek(virtualfile* f, int offset) {
  lseek(fs, f->start, SEEK_SET);
  int loc = lseek(fs, offset, SEEK_CUR);
  //update f->current
  f->current = loc;
  return 0;
}


int getFileNumber(char* path, char* filename) {
  seekDirectory(path);
  char* currentdir = (char*)malloc(512);
  read(fs, currentdir, 512);
  //find (file, filenumber) pair so we can navigate to next level
  char* filepair = strstr(currentdir, filename);
  if(filepair == NULL) {
    cout << "Error: Could not find " << filename << " in " << currentdir;
    return(EXIT_FAILURE);
   }
  //get the associated file # of that file
  while(*filepair != ',') {
    filepair++;
  }
  filepair++;
  int filenum = *filepair - '0';
  return filenum;
}
