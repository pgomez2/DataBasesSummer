HOW TO EXECUTE 

    1) Use the command "make clean" to clean previous files.

    2) Use the command "make" to compile the files 

    3) Use the command "make run_test1" to run all the test

    4) Use the command "make valgrind" to run valgrind and detect data leaks.


MODIFICATIONS OF THE test_assign1_1

    We added at "TEST_CHECK(closePageFile (&fh))" and "free(ph);" to free all dynamic memory in the test 
    "testSinglePageContent(void)"



MANIPULATING PAGE FILES 

    extern void initStorageManager (void)
        -> The function prints "initStorageManager has succesfully iniciated" to ensure it works

    extern RC createPageFile (char *fileName)
        -> the functions creates the file using fopen method. To made the file full of zeros is used the method calloc.

    extern RC openPageFile (char *fileName, SM_FileHandle *fHandle)
        -> this function open the file and create a datastructure with all the metadata of the file. The metadata should be in the begining of
        the file but the test made necessary everything zeros.

    extern RC closePageFile (SM_FileHandle *fHandle)
        -> The function close the file and made null all the pointers in the medatada. To close the file it gets the pointer previously
        storage in the function openPageFile and converted as FILE pointer.

    extern RC destroyPageFile (char *fileName)
        -> The function destroy the file using the method remove.

READING BLOCKS FROM THE DISC

    extern RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage);
    extern int getBlockPos (SM_FileHandle *fHandle);
    extern RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage);
    ->

    extern RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage);
    extern RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage);
    extern RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage);
    extern RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage);





WRITING BLOCKS TO A PAGE FILE 

    extern RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
        -> The function is desingned using fseek and fwrite. fseek put the pointer in the position by pageNum and fwrite 
        writes in the fHandle the information that comes from memPage.

extern RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage);
extern RC appendEmptyBlock (SM_FileHandle *fHandle);
extern RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle);

