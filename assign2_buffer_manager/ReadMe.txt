GROUP 3.
    Pablo Gomez Hidalgo
    Akula Himavanth
    Aswin Reddy Manthena

In this assigment we take serioulsy not only be able to execute all the text without problesm but also not have any data leaks, we use 
valgrind to check there is not any dataleaks. 

We tried to implement the test2_2 with the LFU, CLOCK and LRU-k but we fail doind that. Still there are some references to the code we tried.  

Also to handle the functions aslked to do we created more functions not asked directly in the assigment.  

HOW TO EXECUTE 

    1) Use the command "make clean" to clean previous files.

    2) Use the command "make" to compile the files 

    3) Use the command "make run_test1" to run all the test

    4) Use the command "make valgrind" to run valgrind and detect data leaks.


MODIFICATIONS OF THE test_assign2_1

    On some points of the test_assign2_1 there were some dynamic memory allocation that they were not close so we added the 
    free memroy to not have dataleaks.

    Also we added some printf to debbug some errors and track where the code stop to execute 

0. Functions no asked for 

bool writeBlockToDisk(BM_BufferPool *const bm, PageFrame *pageFrame, int pageFrameIndex)
-->  The function use the functions previously defined in storage_mgr to come back the pages from the buffer 
     to the memory    

bool setNewPageToPageFrame(PageFrame *pageFrame, PageFrame *page, int pageFrameIndex)
-->  This functioncreate a page frame really usefull to handle the pages from the memory tu the buffer.

void FIFO(BM_BufferPool *const bm, PageFrame *page) 
-->  Algorithm to implment FIFO

void LRU(BM_BufferPool *const bm, PageFrame *page)
-->  Algorithm to implemet LRU



1. Buffer Manager Interface Pool Handling


These functions are the skeleton of the Buffer. They allow the creation of a Buffer Pool when there are pages on the disk. The buffer pool is created in memory (most volatile component). 
With the help of storage_mgr from the previous assignment, the start and close operations of the BufferPool are carried out.

This functions are the 

initBufferPool(...)
--> Setup: Assigns the buffer pool's file, number of pages, and replacement strategy.
--> Memory Allocation: Allocates memory for page frames in the buffer pool, returning RC_MEMORY_ALLOCATION_ERROR if allocation fails.
--> Initialization: Initializes each page frame in the buffer pool with default values.
--> Finalization: Sets the buffer pool's management data and resets counters like totalDiskWriteCount, clockPointer, and lfuPointer.

shutdownBufferPool(...)
--> Validation: Checks if the buffer pool is properly initialized.
--> Flush Dirty Pages: Forces all dirty pages in the buffer pool to be written back to disk.
--> Resource Cleanup: Frees memory allocated to page frames and associated data, ensuring no memory leaks.
--> Error Handling: Returns appropriate error codes if issues are encountered during shutdown.

forceFlushPool(...)
--> Validation: Checks if the buffer pool is initialized.
--> Page Processing: Iterates through each page frame in the buffer.
--> Write Dirty Pages: Writes back modified pages (dirty pages) to the disk if they are not currently being used.
--> Error Handling: Handles any errors encountered while opening or writing to the page file.

2. PAGE MANAGEMENT FUNCTIONS

They load pages from disk into the buffer pool (known as pinning pages), release a page frame from the buffer pool (unpinning a page), designate a page as modified or 'dirty', and ensure a modified page frame is written back to the disk.


markDirty(BM_BufferPool *const bm, BM_PageHandle *const page)
--> Input Validation: Checks if the buffer pool (bm), the page handle (page), and the buffer pool management data are initialized. If not, returns RC_ERROR.
--> Marking Process: Iterates through the buffer pool to find the specified page and marks it as dirty by setting the dirtyBit to 1.
--> Error Handling: Returns RC_ERROR if the page is not found in the buffer pool.
--> Status Update: If the page is successfully marked as dirty, returns RC_OK.



unpinPage(BM_BufferPool *const bm, BM_PageHandle *const page)
--> Input Validation: Validates that the buffer pool (bm), the page handle (page), and the buffer pool management data are initialized. If not, returns RC_INVALID_INPUT_PARAMS.
--> Unpinning Process: Iterates through the buffer pool to find the specified page. If found, decrements the fixCount if it's greater than 0.
--> Error Handling: Returns RC_PAGE_NOT_FOUND if the page is not found in the buffer pool.
--> Status Update: If the page is successfully unpinned, returns RC_OK.


forcePage(BM_BufferPool *const bm, BM_PageHandle *const page)
--> Input Validation: Validates that the buffer pool (bm), the page handle (page), and the buffer pool management data are initialized. If not, returns RC_INVALID_INPUT_PARAMS.
--> Writing Process: Iterates through the buffer pool to find the specified page. Opens the page file and writes the page's data back to disk.
--> Error Handling: Includes error checks for file opening and writing failures, returning appropriate error codes such as RC_FILE_NOT_FOUND and RC_WRITE_FAILED.
--> Status Update: Clears the dirtyBit of the page after writing and increments the disk write count. If the page is successfully written, returns RC_OK. If the page is not found, returns RC_PAGE_NOT_FOUND.



pinPage(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum)
--> Input Validation: Validates that the buffer pool (bm) and its management data are initialized.
--> Pinning Process:
    Empty Buffer Pool: If the buffer pool is empty, reads the page from disk, initializes the page frame, and updates the buffer pool.

    Buffer Pool with Pages:
        Checks if the page is already in memory. If found, increments the fixCount and updates hit counters based on the replacement strategy.
        If an empty frame is found, reads the page from disk, initializes the page frame, and updates the buffer pool.
        If the buffer pool is full, allocates a new page frame, reads the page from disk, and applies the appropriate page replacement strategy.

--> Error Handling: Includes error handling for invalid inputs, file opening failures, and other potential issues during the pinning process.
--> Status Update: Updates the buffer pool, increments counters, and applies the replacement strategy. Returns RC_OK upon successful pinning.





