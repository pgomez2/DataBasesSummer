GROUP 3.
    Pablo Gomez Hidalgo
    Akula Himavanth
    Aswin Reddy Manthena


In this assigment we take serioulsy not only be able to execute all the text without problesm but also not have any data leaks, we use 
valgrind to check there is not any dataleaks. 


HOW TO EXECUTE 

    1) Use the command "make clean" to clean previous files.

    2) Use the command "make" to compile the files 

    3) Use the command "make run_test1" to run all the test

    4) Use the command "make valgrind" to run valgrind and detect data leaks.



MODIFICATIONS OF THE test_assign1_1

    We added at "TEST_CHECK(closePageFile (&fh))" and "free(ph);" to free all dynamic memory in the test 
    "testSinglePageContent(void)"

    Also we added some printf to debbug some errors and track where the code stop to execute 



MANIPULATING PAGE FILES 

    extern void initStorageManager (void)
        -> The function prints "initStorageManager has succesfully iniciated" to ensure it works

    extern RC createPageFile (char *fileName)
        -> the functions creates the file using fopen method. To made the file full of zeros is used the method calloc.

    extern RC openPageFile (char *fileName, SM_FileHandle *fHandle)
        -> this function open the file and create a datastructure with all the metadata of the file. The metadata should be in the begining of
        the file but the test made necessary everything zeros. Also checks if the file exists, if not, return erro.

    extern RC closePageFile (SM_FileHandle *fHandle)
        -> The function close the file and made null all the pointers in the medatada. To close the file it gets the pointer previously
        storage in the function openPageFile and converted as FILE pointer.

    extern RC destroyPageFile (char *fileName)
        -> The function destroy the file using the method remove.

READING BLOCKS FROM THE DISC

    extern RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage);
    extern int getBlockPos (SM_FileHandle *fHandle);
    extern RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage);
    extern RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage);
    extern RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage);
    extern RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage);
    extern RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage);

        -> to develope this functions we follow a really similar strategy, basically we get the pointer of the file where the data is storage
        the with the help of the function fseek get the position of the block we want to read. We taked care that the first file start in the 
        "0" position, and with this just update the metadata of the file. 




WRITING BLOCKS TO A PAGE FILE 

    extern RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
    extern RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage);

        ->This two functions are pretty similar between them and with the read blocks, the first part is find the place where we want to write 
        the block and the with the pointer use the funtion fwrite to add the contenct we want.
    
    extern RC appendEmptyBlock (SM_FileHandle *fHandle);
        -> This function is similar to openPageFile. First we find the last location in memory of the file. And them, as we did in openPageFile we create a new 
        empyt bloxk and using fwriting we add to the file.

        extern RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle);
        -> This function is really similar than appendEmptyBlock, it could be just a recursive call to appendEmptyBlock. First take the diference between the 
        current number of pages and the asked number of pages, if this differences is positive. Creates the number of empty blocks give by the difference and
        are added with fwriting.

