//////////////////////////////////////////////////////////////////////
// Intentionally flawed system call library that implements          //
// (unfortunately, not) "safe" file I/O, "preventing" writing "MZ"   //
// at the beginning of a file.                                       //
//                                                                   //
// Written by Golden G. Richard III (@nolaforensix), 7/2017          //
//                                                                   //
// Props to Brian Hay for a similar exercise he used in a recent     //
// training.                                                        //
///////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fileio.h"

//
// GLOBALS
//

//fp = file pointer

//instance of struct to be used in all private functions
FSError fserror;

//
// private functions
//

static int seek_file(File file, SeekAnchor start, long offset) {
  //if not file and there is no position get out
  if (! file->fp || (start != BEGINNING_OF_FILE && 
	       start != CURRENT_POSITION && start != END_OF_FILE)) {
    return 0;
  }
  //fseek sets the file position of FILE file->fp to the offset
  //if file=beginning move pointer to begininng. if at end move pointer to end. else move to SEEK_CUR  
  //used geeksforgeeks for reference 
  else {
    if (! fseek(file->fp, offset, start == BEGINNING_OF_FILE ? SEEK_SET : 
		(start == END_OF_FILE ? SEEK_END : SEEK_CUR))) {
      return 1;
    }
    else {
      return 0;
    }
  }
}

//
// public functions
//

// open or create a file with pathname 'name' and return a File
// handle.  The file is always opened with read/write access. If the
// open operation fails, the global 'fserror' is set to OPEN_FAILED,
// otherwise to NONE.
File open_file(char *name) {
  File f;
	//allocates memory 
  f=malloc(sizeof(struct _FileInternal));
  fserror=NONE;
  //if file existd open it, if not create it, if cant give error
  // try to open existing file
  f->fp=fopen(name, "r+");
  if (!f->fp) {
    // fail, fall back to creation
    f->fp=fopen(name, "w+");
    if (!f->fp) {
      fserror=OPEN_FAILED;
      return NULL;
    }
  }
  //initialize because the first two bytes may be empty
  f->mem[0]=0;
  f->mem[1]=0;
  //get the first 2 bytes
  read_file_from(f,f->mem,2L,BEGINNING_OF_FILE,0L);
  seek_file(f,BEGINNING_OF_FILE,0L);
  return f;
}

// close a 'file'.  If the close operation fails, the global 'fserror'
// is set to CLOSE_FAILED, otherwise to NONE.
void close_file(File file) {
  if (file->fp && ! fclose(file->fp)) {
    fserror=NONE;
  }
  else {
    fserror=CLOSE_FAILED;
  }
  if(file){
    //frees the memory reserved in the open function
    free(file);
  }
}

// read at most 'num_bytes' bytes from 'file' into the buffer 'data',
// starting 'offset' bytes from the 'start' position.  The starting
// position is BEGINNING_OF_FILE, CURRENT_POSITION, or END_OF_FILE. If
// the read operation fails, the global 'fserror' is set to READ_FAILED,
// otherwise to NONE.
unsigned long read_file_from(File file, void *data, unsigned long num_bytes, 
			     SeekAnchor start, long offset) {

  unsigned long bytes_read=0L;

  fserror=NONE;
  if (! file->fp || ! seek_file(file, start, offset)) {
    fserror=READ_FAILED;
  }
  //used tutorials point for fread reference
	//reads 2 elements, each of one byte from the file
  else {
    bytes_read=fread(data, 1, num_bytes, file->fp);
    //if there is an error in reading the file give READ_FAILED
    if (ferror(file->fp)) {
      fserror=READ_FAILED;
    }
  }
  return bytes_read;
}
  
// write 'num_bytes' to 'file' from the buffer 'data', starting
// 'offset' bytes from the 'start' position.  The starting position is
// BEGINNING_OF_FILE, CURRENT_POSITION, or END_OF_FILE.  If an attempt
// is made to modify a file such that "MZ" appears in the first two
// bytes of the file, the write operation fails and ILLEGAL_MZ is
// stored in the global 'fserror'.  If the write fails for any other
// reason, the global 'fserror' is set to WRITE_FAILED, otherwise to
// NONE.
unsigned long write_file_at(File file, void *data, unsigned long num_bytes, 
			     SeekAnchor start, long offset) {
  
  //need this for golden 3 and two file test beacuse they to update the data in mem
  char *temp=(char *)data;
  unsigned long firstbyte = 0L;
  unsigned long secondbyte = 1L;
  //store data into mem and then do the
  unsigned long bytes_written=0L;
  fserror=NONE;
  if (! file->fp || ! seek_file(file, start, offset)) {
    fserror=WRITE_FAILED;
  }
  //if at the start and the first 2 bytes are equal "MZ" returns !0 and gives error
  //works for readwrite.c
  else if (offset == 0L &&  !strncmp(data, "MZ", 2)) {
    // don't let MZ ever appear at the beginning of the file!
    // (it can't be this easy, can it?)
    fserror=ILLEGAL_MZ;
  }
  else if(file->mem[0]=='M'&& !strncmp(data, "Z", 1)){
    fserror=ILLEGAL_MZ;
  }
  else if(file->mem[1]=='Z'&& !strncmp(data, "M", 1)){
    fserror=ILLEGAL_MZ;
  }
  //works for golden3 and two file as it first checks where you are in the file and if you are 
  //at the first or second byte it checks mem along with data by storing data in temp and then 
  //checks if an illegal action would occur, then if not sends the data to mem and writes to the 
  //file
  else{
    unsigned long here = ftell(file->fp);//check of file pointer to see if at first byte or second byte 
    //if at beggining with Z in memory and trying to push in M
    if(here==firstbyte && temp[0]=='M' && (file->mem[0]=='Z'|| file->mem[1]=='Z')){
      fserror=ILLEGAL_MZ;
    }
    //in the first position trying to write an M in front of the Z
    else if(here == firstbyte && (file->mem[1]=='Z'&& temp[0]=='M')){
      fserror= ILLEGAL_MZ;
    }
    //if in the second position with M and trying to write Z behind it
    else if(here==secondbyte && temp[1]=='Z' && (file->mem[0]=='M'|| file->mem[1]=='M')){
      fserror=ILLEGAL_MZ;
    }
    // in the second position trying to add a Z to the M
    else if(here == secondbyte && (file->mem[0]=='M'&& temp[0]=='Z')){
      fserror= ILLEGAL_MZ;
    }
    else{
        //gets past the check then it updates mem and writes to file
        //writes M to mem[0]
        if(here == firstbyte && temp[0]=='M'){
          file->mem[0] = 'M';
        }
        //writes M to mem[0]
        else if(here == firstbyte && (temp[1]=='M' || temp[0]=='M')){
          file->mem[0]='M';
        }
        //writes Z to mem[1]
        else if(here == firstbyte && temp[1]=='Z'){
          file->mem[1]='Z';
        }
        //writes Z to mem[1]
        else if(here == secondbyte && (temp[0]=='Z' || temp[1]=='Z')){
          file->mem[1]='Z';
        }
        //if passes check writes the characters to the file
        bytes_written = fwrite(data,1,num_bytes,file->fp);
        if(bytes_written < num_bytes){
        fserror= WRITE_FAILED;
      }
    }
  }
  return bytes_written;
}
// print a string representation of the error indicated by the global
// 'fserror'.
void fs_print_error(void) {
  printf("FS ERROR: ");
  switch (fserror) {
  case NONE:
    puts("NONE");
    break;
  case OPEN_FAILED:
    puts("OPEN_FAILED");
    break;
  case CLOSE_FAILED:
    puts("CLOSE_FAILED");
    break;
  case READ_FAILED:
    puts("READ_FAILED");
    break;
  case WRITE_FAILED:
    puts("WRITE_FAILED");
    break;
  case ILLEGAL_MZ:
    puts("ILLEGAL_MZ: SHAME ON YOU!");
  break;
  default:
    puts("** UNKNOWN ERROR **");
  }
}




