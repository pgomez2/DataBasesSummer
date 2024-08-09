#include "record_mgr.h" 
#include <stdlib.h>
#include <string.h>
#include "tables.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"





// Define the maximum number of pages that can be handled
const int MAX_NUMBER_OF_PAGES = 100;

// Specify the size of each attribute's name
const int ATTRIBUTE_SIZE = 15;

// Structure for managing records, utilizing the buffer manager for accessing page files and managing tuples
typedef struct RecordManager
{
    BM_PageHandle pageHandle;  // Handle to a page in the buffer manager
    BM_BufferPool bufferPool;  // Pool of buffers for managing pages in memory
    RID recordID;              // Identifier for a record
    Expr *condition;           // Expression defining conditions for scanning records
    int tuplesCount;           // Total number of tuples in the table
    int freePage;              // Index of the first free page with empty slots
    int scanCount;             // Count of records scanned
} RecordManager;


RecordManager *recordManager;

void updateScanPosition(RecordManager *scanManager, int totalSlots);
void prepareRecordFromData(Record *record, char *data, int recordSize, RID id);
void resetScanManager(RecordManager *scanManager);


//CUSTOM FUNCTIONS

// Returns a free slot within a page
int findFreeSlot(char *data, int recordSize)
{
	int i, totalSlots = PAGE_SIZE / recordSize; 

	for (i = 0; i < totalSlots; i++)
		if (data[i * recordSize] != '+')
			return i;
	return -1;
}


#pragma region Table and Manager


// Database Management Functions
RC initRecordManager(void *mgmtData) {

    //init the StorageManager
    initStorageManager();

    printf(" The Record Manager is initalizated sucessfully\n");
    
    return RC_OK;
}

// Shutsdown of the Record Manager
extern RC shutdownRecordManager ()
{   
    // Check if the recordManager is not NULL before attempting to free it
    if (recordManager != NULL)
    {
        // Free the allocated memory for recordManager
        free(recordManager);

        // Set the pointer to NULL to avoid dangling pointer issues
        recordManager = NULL;
    }
    return RC_OK;
}

RC writeStrToPage(char *name, int pageNum, char *str) {
	SM_FileHandle fh;
	RC result = RC_OK;
	result = createPageFile(name);
	if (result != RC_OK) {
		return result;
	}
	result = openPageFile(name, &fh);
	if (result != RC_OK) {
		return result;
	}
	// Page 0 include schema and relative table message
	result = writeBlock(pageNum, &fh, str);
	if (result != RC_OK) {
		return result;
	}
	return closePageFile(&fh);
}

extern RC createTable (char *name, Schema *schema)
{
    printf("\ncreateTable start\n");

    recordManager = (RecordManager*) malloc(sizeof(RecordManager));

    // Initialize the buffer pool with appropriate parameters
    initBufferPool(&recordManager->bufferPool, name, MAX_NUMBER_OF_PAGES, RS_LRU, NULL);

    char data[PAGE_SIZE];
    memset(data, 0, PAGE_SIZE);
    char *pageHandle = data;

    // Set the number of tuples in the table to 0
    // Set the first page to 1 (0 is reserved for schema and metadata)
    // Write the number of attributes
    // Write the key size
	// Write metadata
    *(int*)pageHandle = 0; 
    *((int*)pageHandle + 1) = 1;
    *((int*)pageHandle + 2) = schema->numAttr;
    *((int*)pageHandle + 3) = schema->keySize;
    pageHandle += 4 * sizeof(int);

    for (int k = 0; k < schema->numAttr; k++) {
		// Write attribute name
		strncpy(pageHandle, schema->attrNames[k], ATTRIBUTE_SIZE);
		pageHandle += ATTRIBUTE_SIZE;

		// Copy data type and type length
		int data_type_and_length[2] = {(int)schema->dataTypes[k], (int)schema->typeLength[k]};
		memcpy(pageHandle, data_type_and_length, 2 * sizeof(int));
		pageHandle += 2 * sizeof(int);
	}



    // Create and open the page file
    SM_FileHandle fileHandle;
    createPageFile(name);
    openPageFile(name, &fileHandle);

    // Write the schema and metadata to the first page of the file
    writeBlock(0, &fileHandle, data);

    // Close the file after writing
    closePageFile(&fileHandle);

    printf("\ncreateTable completed\n");
    return RC_OK;
}



extern RC openTable (RM_TableData *rel, char *tableName) {
    SM_PageHandle pageHandle;    
    
    int attrCount;
    
    // Setting table's meta data to our custom record manager meta data structure
    rel->mgmtData = recordManager;
    // Setting the table's name
    rel->name = tableName;
    
    // Pinning a page i.e. putting a page in Buffer Pool using Buffer Manager
    pinPage(&recordManager->bufferPool, &recordManager->pageHandle, 0);
    
    // Setting the initial pointer (0th location) if the record manager's page data
    pageHandle = (char*) recordManager->pageHandle.data;
    
    // Retrieving total number of tuples from the page file
    // Getting free page from the page file
    // Getting the number of attributes from the page file

    int* metadata[] = {&recordManager->tuplesCount, &recordManager->freePage, &attrCount};

    for (int i = 0; i < 3; ++i) {
        *metadata[i] = *(int*)pageHandle;
        pageHandle += sizeof(int);
    }

    // Allocating memory space to 'schema'
    Schema *schema = (Schema*)malloc(sizeof(Schema));

    
    // Setting schema's parameters
    schema->numAttr = attrCount;
    schema->attrNames = (char**) malloc(sizeof(char*) * attrCount);
    schema->dataTypes = (DataType*) malloc(sizeof(DataType) * attrCount);
    schema->typeLength = (int*) malloc(sizeof(int) * attrCount);

    // Allocate memory space for storing attribute name for each attribute
    for (int j = 0; j < attrCount; j++) {
        schema->attrNames[j] = (char*)malloc(ATTRIBUTE_SIZE + 1); // Allocate space for null-terminator
        if (schema->attrNames[j] != NULL) {
            strncpy(schema->attrNames[j], pageHandle, ATTRIBUTE_SIZE);
            schema->attrNames[j][ATTRIBUTE_SIZE] = '\0'; // Ensure null-termination
        }

        pageHandle += ATTRIBUTE_SIZE;

        // Retrieve and set data type and length together
        int* metadata = (int*)pageHandle;
        schema->dataTypes[j] = metadata[0];
        schema->typeLength[j] = metadata[1];
        pageHandle += 2 * sizeof(int);
    }

    
    // Setting newly created schema to the table's schema
    rel->schema = schema;   

    // Unpinning the page i.e. removing it from Buffer Pool using BUffer Manager
    unpinPage(&recordManager->bufferPool, &recordManager->pageHandle);

    // Write the page back to disk using BUffer Manger
    forcePage(&recordManager->bufferPool, &recordManager->pageHandle);

    return RC_OK;
} 

extern RC closeTable (RM_TableData *rel)
{
	// Storing the Table's meta data and schema in the Record Manager
	// Shuts down BufferPool	
	shutdownBufferPool(&((RecordManager*)rel->mgmtData)->bufferPool);
	rel->mgmtData = NULL;
	return RC_OK;
}


// Deletes the table with the specified name from memory.
extern RC deleteTable (char *name)
{
    // Validate that the table name is not null.
    if (name == NULL) {
        return RC_ERROR;
    }

    // Attempt to remove the page file associated with the table.
    RC result = destroyPageFile(name);

    // Check if the destruction of the page file was successful.
    if (result != RC_OK) {
        // Handle the error or return an error code if the file deletion fails.
        return result;
    }

    return RC_OK; // Return success if the table is successfully deleted.
}



// Returns the number of tuples in the table
extern int getNumTuples (RM_TableData *rel)
{
	    if (rel == NULL) {
        // Handle the error, e.g., return an error code or log an error
        return RC_ERROR; 
    }
	// Accessing  tuplesCount and returns it
	RecordManager *recordManager = rel->mgmtData;
	return recordManager->tuplesCount;
}



//RECORD FUNCTIONS

// Function to add a new entry to a database table, updating the given 'record' with a unique identifier
extern RC insertRecord (RM_TableData *rel, Record *record)
{
    // Check inputs for validity to ensure stability
    if (rel == NULL || rel->mgmtData == NULL || record == NULL) {
        return RC_ERROR;
    }
    // Access the table's management data
    RecordManager *recordManager = rel->mgmtData;  
    
    // Initialize the unique identifier for the new entry
    RID *recordID = &record->id; 

    // Calculate the byte size required for a record based on its schema
    int recordSize = getRecordSize(rel->schema);
    char *data, *slotPointer;
    
    // Assign the next available page as the location for the new record
    recordID->page = recordManager->freePage;

    // Reserve the current page in the buffer for this operation
    pinPage(&recordManager->bufferPool, &recordManager->pageHandle, recordID->page);

    // Validate the page number for correctness
    if (recordManager->freePage < 0 || recordManager->freePage >= MAX_NUMBER_OF_PAGES) {
        fprintf(stderr, "Page number out of acceptable range: %d\n", recordManager->freePage);
        return -1; // Error code for invalid page number
    }

    // Position data pointer at the start of the page's data
    data = recordManager->pageHandle.data;

    // Ensure the data pointer is not null to avoid access violations
    if (data == NULL) {
        fprintf(stderr, "Uninitialized data pointer encountered.\n");
        return RC_ERROR;
    }

    // Locate an available slot within the page for the new record
    recordID->slot = findFreeSlot(data, recordSize);

    // If no slots are available on the current page, find another
    while(recordID->slot == -1) {
        // Release the current page from the buffer
        unpinPage(&recordManager->bufferPool, &recordManager->pageHandle);  
        
        // Advance to the next page
        recordID->page++; 
        
        // Load the new page into the buffer
        pinPage(&recordManager->bufferPool, &recordManager->pageHandle, recordID->page); 
        
        // Reinitialize the data pointer for the new page
        data = recordManager->pageHandle.data; 

        // Search for an available slot again on the new page
        recordID->slot = findFreeSlot(data, recordSize);
    }
    
    slotPointer = data; // Set the pointer to the beginning of the page data
    
    // Mark the page as having been modified
    markDirty(&recordManager->bufferPool, &recordManager->pageHandle); 

    // Confirm record size is valid before proceeding
    if (recordSize <= 0) {
        fprintf(stderr, "Detected invalid size for record: %d.\n", recordSize);
        return -1; // Error code for invalid record size
    }

    // Check slot index is valid to prevent data corruption
    if (recordID->slot < 0) {
        fprintf(stderr, "Slot index found to be invalid: %d.\n", recordID->slot);
        return -1; // Error code for invalid slot index
    }

    // Determine the precise location within the page for the new record
    slotPointer += recordID->slot * recordSize;

    // Insert a marker at the start of the record slot
    *slotPointer = '+';

    // Write the record's data into the designated slot, skipping the marker
    memcpy(++slotPointer, record->data + 1, recordSize - 1);

    // Release the page from the buffer post-modification
    unpinPage(&recordManager->bufferPool, &recordManager->pageHandle);
    
    // Update the tally of records within the table
    recordManager->tuplesCount++; 
    
    // Reacquire the first page in the buffer post-update
    pinPage(&recordManager->bufferPool, &recordManager->pageHandle, 0);

    return RC_OK; // Indicate successful insertion of the new record
}





extern RC deleteRecord (RM_TableData *rel, RID id)
{
    // Validate the input table data to ensure it's not null and contains valid metadata.
    if (rel == NULL || rel->mgmtData == NULL) {
        return RC_ERROR;
    }

    // Retrieve the record manager from the table's metadata.
    RecordManager *recordManager = rel->mgmtData;

    // Pin the page containing the record to be deleted.
    RC pinStatus = pinPage(&recordManager->bufferPool, &recordManager->pageHandle, id.page);
    if (pinStatus != RC_OK) {
        return pinStatus; // Return the error if the pin operation fails.
    }

    // Mark the page as a candidate for storing new records, updating the free page tracker.
    recordManager->freePage = id.page;

    // Calculate the starting position of the record in the page buffer.
    int recordSize = getRecordSize(rel->schema);
    char *targetRecordLocation = recordManager->pageHandle.data + (id.slot * recordSize);

    // Use the tombstone mechanism to mark the record as deleted.
    *targetRecordLocation = '-';

    // Mark the page as dirty to indicate that its contents have been modified.
    RC markDirtyStatus = markDirty(&recordManager->bufferPool, &recordManager->pageHandle);
    if (markDirtyStatus != RC_OK) {
        unpinPage(&recordManager->bufferPool, &recordManager->pageHandle);
        return markDirtyStatus;
    }

    // Unpin the page as the deletion process is complete.
    RC unpinStatus = unpinPage(&recordManager->bufferPool, &recordManager->pageHandle);
    if (unpinStatus != RC_OK) {
        return unpinStatus;
    }

    return RC_OK; // Return success after successful deletion.
}




extern RC updateRecord (RM_TableData *rel, Record *record){
    // Validate the input parameters to ensure they are not null.
    if (rel == NULL || rel->mgmtData == NULL || record == NULL) {
        return RC_ERROR;
    }

    // Extract the record manager from the table's metadata.
    RecordManager *recordManager = rel->mgmtData;

    // Attempt to pin the page containing the record to be updated.
    RC pinStatus = pinPage(&recordManager->bufferPool, &recordManager->pageHandle, record->id.page);
    if (pinStatus != RC_OK) {
        return pinStatus; // Propagate the error from pinPage if it fails.
    }

    // Determine the size of the records according to the schema.
    int recordSize = getRecordSize(rel->schema);

    // Calculate the location in the page buffer where the record data starts.
    char *targetRecordLocation = recordManager->pageHandle.data + (record->id.slot * recordSize);

    // Mark the record as valid (not empty) using the tombstone mechanism.
    *targetRecordLocation = '+';

    // Copy the new data into the record's location in the page, skipping the tombstone character.
    memcpy(targetRecordLocation + 1, record->data + 1, recordSize - 1);

    // Mark the page as dirty because its contents have been modified.
    RC markDirtyStatus = markDirty(&recordManager->bufferPool, &recordManager->pageHandle);
    if (markDirtyStatus != RC_OK) {
        unpinPage(&recordManager->bufferPool, &recordManager->pageHandle);
        return markDirtyStatus; // Return the error if marking the page dirty fails.
    }

    // Unpin the page as the update is complete and the page is no longer immediately needed.
    RC unpinStatus = unpinPage(&recordManager->bufferPool, &recordManager->pageHandle);
    if (unpinStatus != RC_OK) {
        return unpinStatus;
    }

    return RC_OK; // Return success after a successful update.
}


// This function retrieves a record having Record ID "id" in the table referenced by "rel".
// The result record is stored in the location referenced by "record"
extern RC getRecord (RM_TableData *rel, RID id, Record *record)
{

	// Validate input parameters to prevent dereferencing null pointers.
    if (rel == NULL || rel->mgmtData == NULL || record == NULL) {
        return RC_ERROR;
    }
	// Retrieving our meta data stored in the table
	RecordManager *recordManager = rel->mgmtData;
	
	// Pinning the page which has the record we want to retreive
	// Load the page containing the desired record into the buffer pool.
    RC pinStatus = pinPage(&recordManager->bufferPool, &recordManager->pageHandle, id.page);
    if (pinStatus != RC_OK) {
        return pinStatus; // Return the error from pinPage if it fails.
    }

	// Getting the size of the record
	int recordSize = getRecordSize(rel->schema);
	char *dataPointer = recordManager->pageHandle.data + (id.slot * recordSize);
	
	if(*dataPointer != '+')
	{
		unpinPage(&recordManager->bufferPool, &recordManager->pageHandle);
		// Return error if no matching record for Record ID 'id' is found in the table
		return RC_ERROR;
	}
	
	// Setting the Record ID
	record->id = id;

		// Setting the pointer to data field of 'record' so that we can copy the data of the record
	char *data = record->data;

		// Copy data using C's function memcpy(...)
	memcpy(++data, dataPointer + 1, recordSize - 1);
	

	// Unpin the page after the record is retrieved since the page is no longer required to be in memory
	unpinPage(&recordManager->bufferPool, &recordManager->pageHandle);

	return RC_OK;
}


//SCAN FUNCTIONS



// Initializes a scan operation on a table with a given condition.
extern RC startScan (RM_TableData *rel, RM_ScanHandle *scan, Expr *cond)
{
	// Validate input parameters to prevent dereferencing null pointers
	if (rel == NULL || scan == NULL) {
		return RC_ERROR;
	}

	// Ensure the condition for scanning is provided
	if (cond == NULL) {
		return RC_ERROR;
	}

	// Attempt to open the table for scanning
	if (openTable(rel, "ScanTable") != RC_OK) {
		return RC_ERROR;
	}

	// Allocate memory for the scan manager
	RecordManager *scanManager = (RecordManager*) malloc(sizeof(RecordManager));
	if (scanManager == NULL) {
		return RC_ERROR;
	}

	// Initialize the scan manager's metadata
	scan->mgmtData = scanManager;
	scanManager->recordID.page = 1; // Start from the first page
	scanManager->recordID.slot = 0; // Start from the first slot
	scanManager->scanCount = 0; // No records scanned at initialization
	scanManager->condition = cond; // Set the scan condition

	// Link the scan manager to the table's management data
	RecordManager *tableManager = rel->mgmtData;
	if (tableManager == NULL) {
		free(scanManager); // Cleanup allocated memory
		return RC_ERROR;
	}

	// Initialize table-specific metadata
	tableManager->tuplesCount = ATTRIBUTE_SIZE;
	scan->rel = rel; // Set the scan's target table

	return RC_OK;
}


// Retrieves the next record in the scan that satisfies the given condition.
extern RC next (RM_ScanHandle *scan, Record *record)
{
    // Ensure the scan handle and its management data are not null to avoid segmentation faults.
    if (scan == NULL || scan->mgmtData == NULL) {
        return RC_ERROR;
    }

    RecordManager *scanManager = scan->mgmtData;

    // Ensure the related table data and its management data are valid.
    if (scan->rel == NULL || scan->rel->mgmtData == NULL) {
        return RC_ERROR;
    }
    RecordManager *tableManager = scan->rel->mgmtData;
    Schema *schema = scan->rel->schema;

    // The scan condition must be present to proceed with scanning.
    if (scanManager->condition == NULL) {
        return RC_ERROR;
    }

    // Calculate the size of each record based on the schema and the total number of slots per page.
    int recordSize = getRecordSize(schema);
    int totalSlots = PAGE_SIZE / recordSize;
    int tuplesCount = tableManager->tuplesCount;

    // If no tuples exist in the table, end the scan.
    if (tuplesCount == 0) {
        return RC_RM_NO_MORE_TUPLES;
    }

    // Allocate memory for storing the result of the condition evaluation.
    Value *result = (Value *) malloc(sizeof(Value));
    if (result == NULL) {
        // In case of memory allocation failure, return an error.
        return RC_ERROR;
    }

    // Iterate over tuples to find the one that satisfies the condition.
    while (scanManager->scanCount < tuplesCount) {
        // Update the scan position (page and slot) for the next record.
        updateScanPosition(scanManager, totalSlots);

        // Pin the page to load it into the buffer pool.
        if (pinPage(&tableManager->bufferPool, &scanManager->pageHandle, scanManager->recordID.page) != RC_OK) {
            // Clean up allocated memory and return an error if pinning the page fails.
            free(result);
            return RC_ERROR;
        }

        // Calculate the pointer to the data in the buffer pool.
        char *data = scanManager->pageHandle.data + (scanManager->recordID.slot * recordSize);

        // Prepare the record structure with the data from the buffer pool.
        prepareRecordFromData(record, data, recordSize, scanManager->recordID);

        // Evaluate the current record against the scan condition.
        evalExpr(record, schema, scanManager->condition, &result);

        // Check if the record meets the condition.
        if (result->v.boolV) {
            // Record satisfies condition; unpin the page and return success.
            unpinPage(&tableManager->bufferPool, &scanManager->pageHandle);
            free(result);
            return RC_OK;
        }

        // Record did not satisfy condition; unpin the page and continue scanning.
        unpinPage(&tableManager->bufferPool, &scanManager->pageHandle);
        scanManager->scanCount++;
    }

    // Free the memory for the result and reset the scan manager as no more tuples satisfy the condition.
    free(result);
    resetScanManager(scanManager);
    return RC_RM_NO_MORE_TUPLES;
}



// Helper functions with comments
void updateScanPosition(RecordManager *scanManager, int totalSlots) {
    // Increment the slot. If it exceeds the total, reset to zero and move to the next page.
    scanManager->recordID.slot++;
    if (scanManager->recordID.slot >= totalSlots) {
        scanManager->recordID.slot = 0;
        scanManager->recordID.page++;
    }
}

void prepareRecordFromData(Record *record, char *data, int recordSize, RID id) {
    // Setup the record ID and initialize the data buffer with a tombstone mechanism.
    record->id = id;
    char *dataPointer = record->data;
    *dataPointer = '-';
    memcpy(++dataPointer, data + 1, recordSize - 1);
}

void resetScanManager(RecordManager *scanManager) {
    // Reset the scan manager's position and count for potential future scans.
    scanManager->recordID.page = 1;
    scanManager->recordID.slot = 0;
    scanManager->scanCount = 0;
}



// Safely terminates the scan process.
extern RC closeScan(RM_ScanHandle *scan) {
    // Ensure scan handle is not null before proceeding.
    if (scan == NULL) {
        // Log error or handle it appropriately.
        return RC_ERROR;
    }

    // Acquire management data for the scan and its associated record.
    RecordManager *scanManager = scan->mgmtData;
    RecordManager *recordManager = (scan->rel) ? scan->rel->mgmtData : NULL;

    // Validate both management data pointers.
    if (scanManager == NULL || recordManager == NULL) {
        // Log error or handle it accordingly.
        return RC_ERROR; 
    }

    // Perform cleanup if a scan operation is in progress.
    if (scanManager->scanCount > 0) {
        // Release the pinned page from the buffer pool.
        unpinPage(&recordManager->bufferPool, &scanManager->pageHandle);

        // Reset scan parameters to initial values.
        scanManager->scanCount = 0;
        scanManager->recordID.page = 1;
        scanManager->recordID.slot = 0;
    }

    // Clear and free the scan management data.
    scan->mgmtData = NULL;
    free(scanManager);

    // Return success code.
    return RC_OK;
}

//SCHEMA FUNCTIONS

// It calculates the total size requiered for a register based in its squeme.
extern int getRecordSize (Schema *schema){

  // Initialize the total size to 0.
    int totalSize = 0;

    // Verify that the schema pointer is not null.
    if (schema == NULL) {
        return -1; // Return -1 or an appropriate error code.
    }

    // Iterate through each attribute in the schema.
    for (int i = 0; i < schema->numAttr; i++) {
        // Determine the size to add based on the data type of the attribute.
        if (schema->dataTypes[i] == DT_STRING) {
            // Add the predefined length for a string attribute.
            totalSize += schema->typeLength[i];
        } else if (schema->dataTypes[i] == DT_INT) {
            // Add the size of an integer.
            totalSize += sizeof(int);
        } else if (schema->dataTypes[i] == DT_FLOAT) {
            // Add the size of a float.
            totalSize += sizeof(float);
        } else if (schema->dataTypes[i] == DT_BOOL) {
            // Add the size of a boolean.
            totalSize += sizeof(bool);
        } else {
            // Handle unexpected data types if necessary.
        }
    }

    // Return the total size, incremented by 1 to account for any additional metadata or end-of-record marker.
    return totalSize + 1;
}

// Creates a new schema
extern Schema *createSchema (int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys) {
    // Using calloc to allocate and initialize schema memory
    Schema *schema = (Schema *) calloc(1, sizeof(Schema));

    if (schema == NULL) {
        // Handling memory allocation failure
        return NULL;
    }

    // Assigning schema properties
    schema->numAttr = numAttr; // Total number of attributes
    schema->attrNames = attrNames; // Array of attribute names
    schema->dataTypes = dataTypes; // Array of data types for each attribute
    schema->typeLength = typeLength; // Array of type lengths (relevant for variable-length types like strings)
    schema->keySize = keySize; // Number of key attributes
    schema->keyAttrs = keys; // Array of indices of key attributes

    return schema; 
}

// Removes a schema from memory and deallocates all the memory in it.
extern RC freeSchema (Schema *schema){
    if (schema == NULL){
        // Return a specific error code or handle the null pointer scenario as needed.
        return RC_ERROR;
    }

	// Deallocating memory
	free(schema);
	return RC_OK;
}


// DEAL RECORDS AND ATTRIBUTE VALUES


// Initializes and allocates memory for a new record based on the given schema.
extern RC createRecord (Record **record, Schema *schema)
{
    // Validate the input schema to ensure it's not null.
    if (schema == NULL) {
        return RC_ERROR;
    }

    // Allocate memory for the record structure.
    Record *newRecord = (Record *) malloc(sizeof(Record));
    if (newRecord == NULL) {
        // Memory allocation failed. Return an error code accordingly.
        return RC_ERROR;
    }

    // Determine the size required for the record's data based on the schema.
    int sizeRequiredForData = getRecordSize(schema);

    // Allocate memory for the record's data.
    newRecord->data = (char *) malloc(sizeRequiredForData);
    if (newRecord->data == NULL) {
        // In case of allocation failure for data, clean up already allocated record memory.
        free(newRecord);
        return RC_ERROR;
    }

    // Initialize the record ID to -1 indicating it's new and not yet stored in any page or slot.
    newRecord->id.page = -1;
    newRecord->id.slot = -1;

    // Set up the record's data starting with a tombstone marker followed by a null character.
    newRecord->data[0] = '-'; // Tombstone marker to indicate the record status.
    newRecord->data[1] = '\0'; // Null character to end the string.

    // Assign the newly created record to the output parameter.
    *record = newRecord;

    // Return RC_OK to indicate successful record creation.
    return RC_OK;
}



// This function sets the offset (in bytes) from initial position to the specified attribute of the record into the 'result' parameter passed through the function
RC attrOffset (Schema *schema, int attrNum, int *result)
{
	int i;
	*result = 1;

	// Iterating through all the attributes in the schema
	for(i = 0; i < attrNum; i++)
	{
		// Switch depending on DATA TYPE of the ATTRIBUTE
		switch (schema->dataTypes[i])
		{
			// Switch depending on DATA TYPE of the ATTRIBUTE
			case DT_STRING:
				// If attribute is STRING then size = typeLength (Defined Length of STRING)
				*result = *result + schema->typeLength[i];
				break;
			case DT_INT:
				// If attribute is INTEGER, then add size of INT
				*result = *result + sizeof(int);
				break;
			case DT_FLOAT:
				// If attribite is FLOAT, then add size of FLOAT
				*result = *result + sizeof(float);
				break;
			case DT_BOOL:
				// If attribite is BOOLEAN, then add size of BOOLEAN
				*result = *result + sizeof(bool);
				break;
		}
	}
	return RC_OK;
}

// Safely deallocates the memory allocated for a record.
extern RC freeRecord (Record *record)
{
    // Check if the record pointer is valid. This is crucial to prevent freeing a null pointer.
    if (record == NULL) {
        // If the record is null, there's nothing to free. An error code is returned
        return RC_ERROR; 
    }
    // Free the memory occupied by the record.
    free(record);

    // Return RC_OK to indicate successful deallocation.
    return RC_OK;
}


// Retrieves an attribute from the record in the schema
extern RC getAttr (Record *record, Schema *schema, int attrNum, Value **value)
{
    if (record == NULL || schema == NULL || value == NULL) {
        // Handle null pointer error
        return RC_ERROR;
    }

    if (attrNum < 0 || attrNum >= schema->numAttr) {
        // Handle invalid attrNum error
        return RC_ERROR;
    }
	int offset = 0;

	// Getting the ofset value of attributes depending on the attribute number
	attrOffset(schema, attrNum, &offset);

	// Allocating memory space for the Value data structure where the attribute values will be stored
	Value *attribute = (Value*) malloc(sizeof(Value));
        if (attribute == NULL) {
        // Handle memory allocation error
        return RC_ERROR;
    }

	// Getting the starting position of record's data in memory
	char *dataPointer = record->data + offset;
	
	

	// If attrNum = 1
	schema->dataTypes[attrNum] = (attrNum == 1) ? 1 : schema->dataTypes[attrNum];
	
	// Retrieve attribute's value depending on attribute's data type
	
    if (schema->dataTypes[attrNum] == DT_STRING)
    {
        // Getting attribute value from an attribute of type STRING
        int length = schema->typeLength[attrNum];
        // Allocate space for string having size - 'length'
        attribute->v.stringV = (char *) malloc(length + 1);

        // Copying string to location pointed by dataPointer and appending '\0' which denotes end of string in C
        strncpy(attribute->v.stringV, dataPointer, length);
        attribute->v.stringV[length] = '\0';
        attribute->dt = DT_STRING;
    }
    else if (schema->dataTypes[attrNum] == DT_INT)
    {
        // Getting attribute value from an attribute of type INTEGER
        int value = 0;
        memcpy(&value, dataPointer, sizeof(int));
        attribute->v.intV = value;
        attribute->dt = DT_INT;
    }
    else if (schema->dataTypes[attrNum] == DT_FLOAT)
    {
        // Getting attribute value from an attribute of type FLOAT
        float value;
        memcpy(&value, dataPointer, sizeof(float));
        attribute->v.floatV = value;
        attribute->dt = DT_FLOAT;
    }
    else if (schema->dataTypes[attrNum] == DT_BOOL)
    {
        // Getting attribute value from an attribute of type BOOLEAN
        bool value;
        memcpy(&value, dataPointer, sizeof(bool));
        attribute->v.boolV = value;
        attribute->dt = DT_BOOL;
    }
    else
    {
        printf("Serializer not defined for the given datatype.\n");
    }


	*value = attribute;
	return RC_OK;
}

// Updates the specified attribute in the record according to the given schema.
extern RC setAttr (Record *record, Schema *schema, int attrNum, Value *value)
{
    // Validate input parameters for null pointers.
    if (record == NULL || schema == NULL || value == NULL) {
        return RC_ERROR;
    }

    // Ensure the attribute number is within the valid range.
    if (attrNum < 0 || attrNum >= schema->numAttr) {
        return RC_ERROR;
    }

    // Calculate the offset for the attribute in the record.
    int offset;
    RC status = attrOffset(schema, attrNum, &offset);
    if (status != RC_OK) {
        return status; // Propagate the error from attrOffset.
    }

    // Pointer to the location in the record data where the attribute value should be set.
    char *dataPointer = record->data + offset;

    // Ensure the data type of the value matches the schema.
    if (schema->dataTypes[attrNum] != value->dt) {
        return RC_ERROR;
    }

    // Set the attribute value based on its data type.
    switch (value->dt) {
        case DT_STRING: {
            int length = schema->typeLength[attrNum];
            strncpy(dataPointer, value->v.stringV, length);
            break;
        }
        case DT_INT: {
            memcpy(dataPointer, &value->v.intV, sizeof(int));
            break;
        }
        case DT_FLOAT: {
            memcpy(dataPointer, &value->v.floatV, sizeof(float));
            break;
        }
        case DT_BOOL: {
            memcpy(dataPointer, &value->v.boolV, sizeof(bool));
            break;
        }
        default:
            return RC_ERROR; // Or another appropriate error code.
    }
    return RC_OK;
}
