#include "storage_mgr.h"
#include <stdio.h>
#include <stdlib.h>>

/* manipulating page files */
extern void initStorageManager (void){

    // we this print it is ensure the Storage Manager is inicializated
    printf("initStorageManager has succesfully iniciated");

    //return ok to ensure everything is correct 
    return RC_OK;
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