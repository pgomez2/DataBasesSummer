#include "storage_mgr.h"
#include <stdio.h>
#include <stdlib.h>

/* manipulating page files */
extern void initStorageManager (void){

    // we this print it is ensure the Storage Manager is inicializated
    printf("initStorageManager has succesfully iniciated");

    }


extern RC createPageFile (char *fileName){
    
    //create a file, use the name that come from the argument of the function
    //and store the direction of the file in a pointer called file
    FILE *file = fopen(fileName, "w+"); 

    // this is zeros in a reserve part of the memory with calloc 
    char *fileWithZeros  = (char*)calloc(PAGE_SIZE, sizeof(char));

    //write in the files the zeros in the file 
    fwrite(fileWithZeros, sizeof(char), PAGE_SIZE, file);

    //libeerate the space in the reserve part of the memory
    free(fileWithZeros);
    //close the file 
    fclose(file);
    return RC_OK;
    }
extern RC openPageFile (char *fileName, SM_FileHandle *fHandle){
    
    //Check if the file exists, if doesn't exisest return RC_FILE_NOT_FOUND
    if (fileName == NULL){
        return RC_FILE_NOT_FOUND;
    }
    FILE *file = fopen(fileName, "rb+");
    //now update the SM_FileHandle with the information
    //first the name of the file
    fHandle -> fileName = fileName;
    
    //now get he number of pages in the file, to this it looks for the end of the file 
    //and divides it by the size of the page.

    //fseek get the pointer of fileName and change the direction to the 
    //direction of memeroy where the file ends
    fseek(file, 0, SEEK_END);
    
    //ftell return the distance btw where is the pointer at the moment and 
    //the beggining of the element
    long fileSize = ftell(file);

    //if there is a error, fileSize returns -1
    if(fileSize == -1){
        return RC_FILE_NOT_FOUND; 
    }

    //The result is divide by the side of a page, and get the number of pages 
    fHandle -> totalNumPages = fileSize / PAGE_SIZE; 
    
    // Initially set the current page position to 0
    fHandle -> curPagePos=0;

    // Store the file pointer in mgmtInfo for further operations
    fHandle -> mgmtInfo= fileName;
    
    return RC_OK;
    }


extern RC closePageFile (SM_FileHandle *fHandle){
    
    //fclose(fHandle);
    return RC_OK;
    }
extern RC destroyPageFile (char *fileName){

    remove(fileName); 

    return RC_OK;
    }

/* reading blocks from disc */
extern RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage){return RC_OK;}
extern int getBlockPos (SM_FileHandle *fHandle){return RC_OK;}
extern RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){return RC_OK;}
extern RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){return RC_OK;}
extern RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){return RC_OK;}
extern RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){return RC_OK;}
extern RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){return RC_OK;}

/* writing blocks to a page file */
extern RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage){return RC_OK;}
extern RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){return RC_OK;}
extern RC appendEmptyBlock (SM_FileHandle *fHandle){return RC_OK;}
extern RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle){return RC_OK;}