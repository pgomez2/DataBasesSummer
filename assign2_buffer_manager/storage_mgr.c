#include<stdio.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>
#include<string.h>
#include<math.h>

#include "storage_mgr.h"

FILE *pageFile;

/* manipulating page files */
extern void initStorageManager (void){

    // we this print it is ensure the Storage Manager is inicializated
    printf("initStorageManager has succesfully iniciated\n");
}



RC createPageFile(char *fileName) {
    FILE *file = fopen(fileName, "w+");
    if (!file) return RC_FILE_NOT_FOUND;

    char *emptyPage = calloc(PAGE_SIZE, sizeof(char));
    if (!emptyPage) {
        fclose(file);
        return RC_MEMORY_ALLOCATION_ERROR;
    }

    size_t writeResult = fwrite(emptyPage, sizeof(char), PAGE_SIZE, file);
    free(emptyPage);
    fclose(file);

    return (writeResult == PAGE_SIZE) ? RC_OK : RC_WRITE_FAILED;
}


RC openPageFile(char *fileName, SM_FileHandle *fHandle) {
    // Attempt to open the specified file in read-only mode
    FILE *fileDescriptor = fopen(fileName, "r");

    // Verify the file was opened successfully
    if (!fileDescriptor) {
        return RC_FILE_NOT_FOUND;
    } else {
        // Initialize file handle details with the file name and reset the page position
        fHandle->fileName = fileName;
        fHandle->curPagePos = 0;

        // Utilize fstat to fetch the file's statistics for size calculation
        struct stat fileStats;
        if (fstat(fileno(fileDescriptor), &fileStats) < 0) {
            return RC_ERROR;
        }

        // Determine the number of pages based on the file size and predefined page size
        fHandle->totalNumPages = fileStats.st_size / PAGE_SIZE;

        // Close the file to ensure the integrity of file operations
        fclose(fileDescriptor);
        return RC_OK;
    }
}


RC destroyPageFile(char *newFileName) {

    //This function tries to delete a specified page file
    // and returns success if successful, or a file not found error otherwise.



    if (remove(newFileName) == 0) {
        return RC_OK;
    } else {  
        return RC_FILE_NOT_FOUND;
    }
}

RC readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Validate the given page number is within the file's range
    if (pageNum < 0 || pageNum >= fHandle->totalNumPages)
        return RC_READ_NON_EXISTING_PAGE;

    // Attempt to open the file associated with the file handle for reading
    FILE *file = fopen(fHandle->fileName, "r");
    if (!file)  // Ensure the file was opened successfully
        return RC_FILE_NOT_FOUND;
    
    // Move the file pointer to the start of the specified page
    if (fseek(file, pageNum * PAGE_SIZE, SEEK_SET) != 0) {
        fclose(file);  // Close the file if seeking failed
        return RC_READ_NON_EXISTING_PAGE;
    }

    // Read the page into the provided buffer
    size_t readBytes = fread(memPage, sizeof(char), PAGE_SIZE, file);
    if (readBytes < PAGE_SIZE) {  // Verify that a full page was read
        fclose(file);  // Close the file if reading was incomplete
        return RC_ERROR;
    }

    // Update the file handle's current page position
    fHandle->curPagePos = ftell(file);

    // Close the file to ensure data consistency
    fclose(file);

    return RC_OK;
}


int getBlockPos(SM_FileHandle *fileHandle) {
   
    // Return the current page position.

    return fileHandle->curPagePos;
}

RC readFirstBlock(SM_FileHandle *fileHandle, SM_PageHandle pageData) {
 
    // Reads the first block of a file, updates current page position in file handle, and handles file not found errors.
    FILE *filePointer;
    filePointer = fileHandle->mgmtInfo;
    if (filePointer == NULL) {
       
        return RC_FILE_NOT_FOUND;

    } else {
        fseek(filePointer, 0, SEEK_SET);
        fread(pageData, PAGE_SIZE, 1, filePointer);
        fileHandle->curPagePos = 0;
       
        return RC_OK;
    }
}


RC readPreviousBlock(SM_FileHandle *fileHandle, SM_PageHandle pageData) {
   //Reads the previous block in a file, updates the current position,
   // and manages errors for start of file or file not found.


    FILE *filePointer;
    filePointer = fileHandle->mgmtInfo;
    if (filePointer == NULL) {
       
        return RC_FILE_NOT_FOUND;
    } else {
        if (fileHandle->curPagePos == 0) {
            return RC_READ_NON_EXISTING_PAGE;
        } else {
           
            fseek(filePointer, (fileHandle->curPagePos - 1) * PAGE_SIZE, SEEK_SET);
            fread(pageData, PAGE_SIZE, 1, filePointer);
            fileHandle->curPagePos = fileHandle->curPagePos - 1;
           
           
            return RC_OK;
        }
    }
}


RC readCurrentBlock(SM_FileHandle *fileHandle, SM_PageHandle pageData) {
   
    //Reads the current block from a file based on the file handle's position and
    //handles cases where the file is not found.
   
   
    FILE *filePointer;
    filePointer = fileHandle->mgmtInfo;
    if (filePointer == NULL) {
        return RC_FILE_NOT_FOUND;
    } else {
       
        fseek(filePointer, fileHandle->curPagePos * PAGE_SIZE, SEEK_SET);
        fread(pageData, PAGE_SIZE, 1, filePointer);
        return RC_OK;
    }
}


RC readNextBlock(SM_FileHandle *fileHandle, SM_PageHandle pageData) {
    //Reads the next block in a file, updates the file handle's current position, and manages errors for end of file or file not found.
    //
   
   
    FILE *filePointer;
   
    filePointer = fileHandle->mgmtInfo;
    if (filePointer == NULL) {
        return RC_FILE_NOT_FOUND;
    } else {
       
        if (fileHandle->curPagePos == fileHandle->totalNumPages) {
            return RC_READ_NON_EXISTING_PAGE;
        } else {
           
            fseek(filePointer, (fileHandle->curPagePos + 1) * PAGE_SIZE, SEEK_SET);
            fread(pageData, PAGE_SIZE, 1, filePointer);
            fileHandle->curPagePos = fileHandle->curPagePos + 1;
         
            return RC_OK;
        }
    }
}

RC readLastBlock(SM_FileHandle *fileHandle, SM_PageHandle pageData) {
   
    //Reads the last block from a file, updates the file handle's current page position
    //, and handles cases where the file is not found.


    FILE *filePointer;
    filePointer = fileHandle->mgmtInfo;
    if ( filePointer == NULL) {
        return RC_FILE_NOT_FOUND;
    } else {
        fseek(filePointer, fileHandle->totalNumPages * PAGE_SIZE, SEEK_SET);
        fread(pageData, PAGE_SIZE, 1, filePointer  );
        fileHandle->curPagePos = fileHandle->totalNumPages;
     
     
        return RC_OK;
    }
}

RC writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Validate page number to be within the file's page range
    if (pageNum > fHandle->totalNumPages || pageNum < 0)
        return RC_WRITE_FAILED;
    
    // Open the file in read-write mode to allow modifications
    pageFile = fopen(fHandle->fileName, "r+");
    
    // Ensure the file is accessible and opened correctly
    if (pageFile == NULL)
        return RC_FILE_NOT_FOUND;

    int startPosition = pageNum * PAGE_SIZE;

    if (pageNum == 0) {
        // Position file pointer for writing at the calculated offset
        fseek(pageFile, startPosition, SEEK_SET);
        int i;
        for (i = 0; i < PAGE_SIZE; i++) {
            // Append an empty block if EOF is reached prematurely
            if (feof(pageFile))
                appendEmptyBlock(fHandle);
            // Output each byte from the memory page to the file
            fputc(memPage[i], pageFile);
        }

        // Update file handle to reflect new position after writing
        fHandle->curPagePos = ftell(pageFile);

        // Close the file to commit changes and clear buffers
        fclose(pageFile);
    } else {
        // Prepare to write to the first page by setting the current position
        fHandle->curPagePos = startPosition;
        fclose(pageFile);  // Close the file before proceeding to write
        writeCurrentBlock(fHandle, memPage);  // Delegate writing to dedicated function
    }
    return RC_OK;
}


RC writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    // Attempt to access the file with read-write privileges to update its contents
    pageFile = fopen(fHandle->fileName, "r+");

    // Validate the file's availability for operations
    if (pageFile == NULL)
        return RC_FILE_NOT_FOUND;
    
    // Increase file size by adding a block, preparing space for new data
    appendEmptyBlock(fHandle);

    // Position the file pointer to the designated location for data writing
    fseek(pageFile, fHandle->curPagePos, SEEK_SET);
    
    // Transfer the data from memory to the specified file position
    fwrite(memPage, sizeof(char), strlen(memPage), pageFile);
    
    // Update the file handle to reflect the new position after the write operation
    fHandle->curPagePos = ftell(pageFile);

    // Ensure all changes are saved and the file is properly closed
    fclose(pageFile);
    return RC_OK;
}


RC appendEmptyBlock(SM_FileHandle *fHandle) {
    // Define an empty page with static allocation filled with zeros
    char emptyPage[PAGE_SIZE] = {0};

    // Move the file pointer to the end of the file to append the new block
    if (fseek(pageFile, 0, SEEK_END) != 0) {
        return RC_ERROR; // Return error if unable to set the pointer
    }

    // Write the empty block to the file, expanding its size
    size_t result = fwrite(emptyPage, sizeof(char), PAGE_SIZE, pageFile);
    if (result < PAGE_SIZE) {
        return RC_WRITE_FAILED; // Return error if the write operation did not complete successfully
    }

    // Increase the count of total pages in the file handle after appending
    fHandle->totalNumPages++;

    return RC_OK; // Indicate successful append operation
}


RC ensureCapacity(int numberOfPages, SM_FileHandle *fHandle) {
    if (fHandle == NULL || fHandle->mgmtInfo == NULL) {
        return RC_FILE_HANDLE_NOT_INIT;
    }

    FILE *filePointer = (FILE *)fHandle->mgmtInfo;
    int currentPageCount = fHandle->totalNumPages;

    if (numberOfPages <= currentPageCount) {
        return RC_OK;
    }

    // Posicionarse al final del archivo
    if (fseek(filePointer, 0, SEEK_END) != 0) {
        return RC_WRITE_FAILED; // Si fseek falla, se asume un error de escritura
    }

    char *emptyPage = (char *)calloc(PAGE_SIZE, sizeof(char));
    if (emptyPage == NULL) {
        return RC_MEMORY_ALLOCATION_FAIL;
    }

    for (int i = currentPageCount; i < numberOfPages; i++) {
        if (fwrite(emptyPage, sizeof(char), PAGE_SIZE, filePointer) < PAGE_SIZE) {
            free(emptyPage); // Liberar la memoria en caso de error de escritura
            return RC_WRITE_FAILED;
        }
    }

    free(emptyPage); // Liberar la memoria asignada después de su uso

    // Actualizar el total de páginas solo después de que todas las nuevas páginas se hayan escrito con éxito
    fHandle->totalNumPages = numberOfPages;

    return RC_OK;
}