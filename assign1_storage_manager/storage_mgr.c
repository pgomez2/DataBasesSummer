#include "storage_mgr.h"
#include <stdio.h>
#include <stdlib.h>

/* manipulating page files */
extern void initStorageManager (void){

    // we this print it is ensure the Storage Manager is inicializated
    printf("initStorageManager has succesfully iniciated\n");

    }


extern RC createPageFile (char *fileName){
    printf("The fileName is %s\n", fileName);
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
    
    // open the file and storage the location in file
    FILE *file = fopen(fileName, "rb+");
    //Check if the file exist, if doesn't exisest return RC_FILE_NOT_FOUND
    if( file == NULL){
       
        return RC_FILE_NOT_FOUND;
    }
   
    //now update the SM_FileHandle with the information
    //first the name of the file

    fHandle -> fileName = fileName;
    printf("fileName in openPage: %s\n", fHandle->fileName);
    
    //now get he number of pages in the file, to this it looks for the end of the file 
    //and divides it by the size of the page.

    //fseek get the pointer of fileName and change the direction to the 
    //direction of memory where the file ends.
    fseek(file, 0, SEEK_END);
    
    //ftell return the distance btw where is the pointer at the moment and 
    //the beggining of the element. It is storage in fileSize
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
    fHandle -> mgmtInfo= file;
    
    return RC_OK;
    }


extern RC closePageFile (SM_FileHandle *fHandle){
    
    //close the file with fclose method
    //fclose requiered a FILE pointer
    if (fclose((FILE*)fHandle->mgmtInfo) != 0) {
        return RC_FILE_NOT_FOUND;
    }
    
    //made null all the pointrs inside of the datastructure 
    fHandle -> fileName = NULL;
    fHandle -> mgmtInfo = NULL;
    fHandle = NULL;


    return RC_OK;
    }
extern RC destroyPageFile (char *fileName){
    
    //remove the file and return and error if the method doesn't work
    if(remove(fileName)==-1){
        return RC_FILE_NOT_FOUND;
    } 

    return RC_OK;
    }

/* reading blocks from disc */
extern RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage){
    
    // Inicializate a File pointer with the direction of the file is gonna be use it.
    FILE* file = (FILE*)fHandle -> mgmtInfo;

    //with fseek get the direction of the pointer, the -1 is because C start to count in zero, so the first block is 0
    if (fseek(file, PAGE_SIZE*(pageNum-1), SEEK_SET) != 0) {
        return RC_FILE_NOT_FOUND; // fseek failed to set position to the beginning
    }

    
    //read the file and copy the elements top, the info is storage in the
    //memory direction of memPage 
    fread(memPage,PAGE_SIZE,1, file);
    


    return RC_OK;
    
    }
extern int getBlockPos (SM_FileHandle *fHandle){
    // Inicializate a File pointer with the direction of the file is gonna be use it.
    FILE* file = (FILE*)fHandle -> mgmtInfo;

    //with fseek get the direction of the pointer, so the first block is 0 
    if (fseek(file, PAGE_SIZE*((fHandle->curPagePos)-1), SEEK_SET) != 0) {
        return RC_FILE_NOT_FOUND; // fseek failed to set position to the beginning
    }
    
    
    return RC_OK;}

extern RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    
    //get the pointer from the metadata 
    FILE* file = (FILE*)fHandle -> mgmtInfo; 
    
    // Seek to the beginning of the file
    if (fseek(file, 0, SEEK_SET) != 0) {
        return RC_FILE_NOT_FOUND; // fseek failed to set position to the beginning
    }

    //read the file and copy the elements top, the info is storage in the
    //memory direction of memPage 
    fread(memPage,PAGE_SIZE,1, file);
    


    return RC_OK;
}
extern RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){        
    // Inicializate a File pointer with the direction of the file is gonna be use it.
    FILE* file = (FILE*)fHandle -> mgmtInfo;

    //with fseek get the direction of the pointer, the -2 is because C start to count in zero, so the first block is 0
    if (fseek(file, PAGE_SIZE*((fHandle->curPagePos)-2), SEEK_SET) != 0) {
        return RC_FILE_NOT_FOUND; // fseek failed to set position to the beginning
    }

    
    //read the file and copy the elements top, the info is storage in the
    //memory direction of memPage 
    fread(memPage,PAGE_SIZE,1, file);
    
    
    
    return RC_OK;
}
extern RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    // Inicializate a File pointer with the direction of the file is gonna be use it.
    FILE* file = (FILE*)fHandle -> mgmtInfo;

    //with fseek get the direction of the pointer, the -1 is because C start to count in zero, so the first block is 0
    if (fseek(file, PAGE_SIZE*((fHandle->curPagePos)-1), SEEK_SET) != 0) {
        return RC_FILE_NOT_FOUND; // fseek failed to set position to the beginning
    }

    
    //read the file and copy the elements top, the info is storage in the
    //memory direction of memPage 
    fread(memPage,PAGE_SIZE,1, file);
    
    
    return RC_OK;
    }
extern RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    // Inicializate a File pointer with the direction of the file is gonna be use it.
    FILE* file = (FILE*)fHandle -> mgmtInfo;

    //with fseek get the direction of the pointer, so the first block is 0 
    if (fseek(file, PAGE_SIZE*((fHandle->curPagePos)), SEEK_SET) != 0) {
        return RC_FILE_NOT_FOUND; // fseek failed to set position to the beginning
    }

    
    //read the file and copy the elements top, the info is storage in the
    //memory direction of memPage 
    fread(memPage,PAGE_SIZE,1, file);
    
    
    return RC_OK;
}
extern RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
            
        
    // Inicializate a File pointer with the direction of the file is gonna be use it.
    FILE* file = (FILE*)fHandle -> mgmtInfo;

    //with fseek get the direction of the pointer, so the first block is 0 
    if (fseek(file, -PAGE_SIZE, SEEK_END) != 0) {
        return RC_FILE_NOT_FOUND; // fseek failed to set position to the beginning
    }

    
    //read the file and copy the elements top, the info is storage in the
    //memory direction of memPage 
    fread(memPage,PAGE_SIZE,1, file);
    
    
    
    return RC_OK;
    }

/* writing blocks to a page file */
extern RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage){
    
    //get the file pointer that was stored in mgmtInfo
    FILE *file = (FILE*)fHandle -> mgmtInfo;
    
    //find the position of the page that have to be modified
    int seekResult = fseek(file, PAGE_SIZE*pageNum,SEEK_SET);
    if (seekResult != 0) {
        return RC_WRITE_FAILED;  // fseek failed, really useful to debug
    }
    //with the position that have to be modified, modfied it, the information is in memPage
    size_t writeResult = fwrite(memPage,sizeof(char) ,PAGE_SIZE, file); 
    if (writeResult < PAGE_SIZE) {
        return RC_WRITE_FAILED;  // fwrite did not write the expected amount of data
    }
    
    //update the metadata with the 
    fHandle -> curPagePos = pageNum;
    
    //delete the pointer
    file = NULL; 
    
    return RC_OK;
}
extern RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
            
        
    // Inicializate a File pointer with the direction of the file is gonna be use it.
    FILE* file = (FILE*)fHandle -> mgmtInfo;

    //with fseek get the direction of the pointer, so the first block is 0 
    if (fseek(file, PAGE_SIZE*((fHandle->curPagePos)-1), SEEK_SET) != 0) {
        return RC_FILE_NOT_FOUND; // fseek failed to set position to the beginning
    }

    
    //with the position that have to be modified, modfied it, the information is in memPage
    size_t writeResult = fwrite(memPage,sizeof(char) ,PAGE_SIZE, file); 
    if (writeResult < PAGE_SIZE) {
        return RC_WRITE_FAILED;  // fwrite did not write the expected amount of data
    }
    
    
    return RC_OK;
    }
extern RC appendEmptyBlock (SM_FileHandle *fHandle){
    
    //reserve some space in the memory with zeroes
    char *pageWithZeros  = (char*)calloc(PAGE_SIZE, sizeof(char));

    //go the final part of the file 
    fseek(fHandle->mgmtInfo, 0, SEEK_END);

    //add the page (block) with zeros at the end of the file
    fwrite(pageWithZeros, sizeof(char), PAGE_SIZE, fHandle->mgmtInfo);

    //liberate the memory used it for the zeros
    free(pageWithZeros);

    // Change the number of pages of the file in the metadata
    fHandle->totalNumPages ++;


    return RC_OK;
    }
extern RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle){
    
    // Calculate the number of pages to add
    int pagesToAdd = numberOfPages - fHandle->totalNumPages;

    // check if there is enough vapacity, if there are just skip the if
    if(pagesToAdd>0){
    
    // create the number of blocks with zeros to have enough pages
    char *pageWithZeros  = (char*)calloc(PAGE_SIZE*(pagesToAdd) , sizeof(char));

    //go the final part of the file 
    fseek(fHandle->mgmtInfo, 0, SEEK_END);

    //add the page(s) with zeros at the end of the file
    fwrite(pageWithZeros, sizeof(char), PAGE_SIZE, fHandle->mgmtInfo);

    //liberate the memory used it for the zeros
    free(pageWithZeros);

    // Change the number of pages of the file in the metadata
    fHandle->totalNumPages += pagesToAdd;
    
    //return ok

    return RC_OK;
    }
    
    //maybe it was not necesarry to operate, so give back the OK
    return RC_OK;
    }