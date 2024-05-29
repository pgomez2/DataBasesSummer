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



