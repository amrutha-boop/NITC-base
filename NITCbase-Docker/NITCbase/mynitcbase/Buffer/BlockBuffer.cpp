#include "BlockBuffer.h"

#include <cstdlib>
#include <cstring>


BlockBuffer::BlockBuffer(int blockNum) {
  this->blockNum = blockNum;
}

BlockBuffer::BlockBuffer(char blockType){
    int ret = getFreeBlock(blockType);
    this->blockNum = ret;
}


// calls the parent class constructor
RecBuffer::RecBuffer(int blockNum) : BlockBuffer::BlockBuffer(blockNum) {}

RecBuffer::RecBuffer() : BlockBuffer('R'){}

int BlockBuffer::getBlockNum(){
    return this->blockNum;
}

// load the block header into the argument pointer
int BlockBuffer::getHeader(struct HeadInfo *head) {
  //Load the block into a buffer and return pointer to the buffer
  unsigned char *bufferPtr;
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) {
    return ret;  
  }

  // populate the numEntries, numAttrs and numSlots fields in *head
  memcpy(&head->lblock, bufferPtr + 8, 4);
  memcpy(&head->rblock, bufferPtr + 12, 4);
  memcpy(&head->numEntries, bufferPtr + 16, 4);
  memcpy(&head->numAttrs, bufferPtr + 20, 4);
  memcpy(&head->numSlots, bufferPtr + 24, 4);

  return SUCCESS;
}

// load the record at slotNum into the argument pointer
int RecBuffer::getRecord(union Attribute *rec, int slotNum) {
  struct HeadInfo head;
  this->getHeader(&head);

  int attrCount = head.numAttrs;
  int slotCount = head.numSlots;
  if (slotNum < 0 || slotNum >= head.numSlots)
    return E_OUTOFBOUND;

  //Load block from buffer
  unsigned char *bufferPtr;
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) {
    return ret;
  }

  /* record at slotNum will be at offset HEADER_SIZE + slotMapSize + (recordSize * slotNum)
     - each record will have size attrCount * ATTR_SIZE
     - slotMap will be of size slotCount
  */
  int recordSize = attrCount * ATTR_SIZE;
  int offset = HEADER_SIZE + slotCount + recordSize * slotNum;
  unsigned char *slotPointer = bufferPtr + offset;/* calculate buffer + offset */

  // load the record into the rec data structure
  memcpy(rec, slotPointer, recordSize);

  return SUCCESS;
}

int RecBuffer::setRecord(union Attribute *rec, int slotNum) {
    unsigned char *bufferPtr;

    int retVal = loadBlockAndGetBufferPtr(&bufferPtr);
    if (retVal != SUCCESS)
        return retVal;

    HeadInfo head;
    getHeader(&head);

    int numAttrs = head.numAttrs;
    int numSlots = head.numSlots;

    if (slotNum < 0 || slotNum >= numSlots)
        return E_OUTOFBOUND;

    int recordSize = ATTR_SIZE * numAttrs;

    unsigned char *recordPtr =
        bufferPtr + HEADER_SIZE
        + numSlots
        + slotNum * recordSize;

    memcpy(recordPtr, rec, recordSize);

    retVal = StaticBuffer::setDirtyBit(this->blockNum);
    if (retVal != SUCCESS)
        return retVal;

    return SUCCESS;
}

int BlockBuffer::setHeader(struct HeadInfo *head){
    // Get the buffer pionter and convert it to HeadInfo pointer
    unsigned char *bufferPtr;
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if (ret != SUCCESS)
        return ret;
    struct HeadInfo *bufferHeader = (struct HeadInfo *)bufferPtr;

    // Copy the fields from the argument head to the buffer header
    bufferHeader->pblock = head->pblock;
    bufferHeader->lblock = head->lblock;
    bufferHeader->rblock = head->rblock;
    bufferHeader->numEntries = head->numEntries;
    bufferHeader->numAttrs = head->numAttrs;
    bufferHeader->numSlots = head->numSlots;

    // set the dirty bit of the block in the buffer
    ret = StaticBuffer::setDirtyBit(this->blockNum);
    if(ret != SUCCESS)
        return ret;
    return SUCCESS;
}

int BlockBuffer::setBlockType(int blockType){
    // Load the block into a buffer and get the pointer to the buffer
    unsigned char *bufferPtr;
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if(ret != SUCCESS)  
        return ret;

    // set the blockType field in the buffer header to the argument blockType    
    *((int32_t *)bufferPtr) = blockType;

    // set the blockType in the StaticBuffer::blockAllocMap for this blockNum to the argument blockType 
    StaticBuffer::blockAllocMap[this->blockNum] = blockType;

    // update dirty bit
    ret = StaticBuffer::setDirtyBit(this->blockNum);
    if(ret != SUCCESS)
        return ret;

    return SUCCESS;
}


int BlockBuffer::getFreeBlock(int blockType){   
    // find a free block in the StaticBuffer::blockAllocMap
    int freeBlock = -1;
    for(int i = 0; i < DISK_BLOCKS; i++){
        if(StaticBuffer::blockAllocMap[i] == UNUSED_BLK){
            freeBlock = i;
            break;
        }
    }
    if(freeBlock  == -1)
        return E_DISKFULL;

    // set the object's blockNum to the block number of the free block.
    this->blockNum = freeBlock;

    //  get a free buffer in the buffer pool using getFreeBuffer()
    int freeBufferNum = StaticBuffer::getFreeBuffer(freeBlock);
    if(freeBufferNum == E_CACHEFULL)
        return E_CACHEFULL;

    // set the block type of the free block to the input block type using setBlockType().
    struct HeadInfo head;
    head.pblock = -1;
    head.lblock = -1;
    head.rblock = -1;
    head.numEntries = 0;
    head.numAttrs = 0;
    head.numSlots = 0;
    this->setHeader(&head);

    // update the block type of the block to the input block type using setBlockType().
    this->setBlockType(blockType);

    // return block number of the free block.
    return freeBlock;
}


int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char **buffPtr) {
    int bufferNum = StaticBuffer::getBufferNum(this->blockNum);

    // Block is already present in buffer
    if (bufferNum != E_BLOCKNOTINBUFFER) {
        if (bufferNum == E_OUTOFBOUND)
            return E_OUTOFBOUND;

        // Make this buffer the most recently used
        for (int i = 0; i < BUFFER_CAPACITY; i++) {
            if (!StaticBuffer::metainfo[i].free)
                StaticBuffer::metainfo[i].timeStamp++;
        }
        StaticBuffer::metainfo[bufferNum].timeStamp = 0;
    }

    // Block is not present in buffer
    else {
        bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);
        if (bufferNum == E_OUTOFBOUND)
            return E_OUTOFBOUND;

        // Read block from disk into buffer
        Disk::readBlock(
            StaticBuffer::blocks[bufferNum],
            this->blockNum
        );
    }

    // Return pointer to the buffer
    *buffPtr = StaticBuffer::blocks[bufferNum];
    return SUCCESS;
}

/* used to get the slotmap from a record block
NOTE: this function expects the caller to allocate memory for `*slotMap`
*/
int RecBuffer::getSlotMap(unsigned char *slotMap) {
  unsigned char *bufferPtr;

  // get the starting address of the buffer containing the block using loadBlockAndGetBufferPtr().
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) {
    return ret;
  }

  struct HeadInfo head;
  // get the header of the block using getHeader() function
  this->getHeader(&head);
  int slotCount =  head.numSlots/* number of slots in block from header */;

  // get a pointer to the beginning of the slotmap in memory by offsetting HEADER_SIZE
  unsigned char *slotMapInBuffer = bufferPtr + HEADER_SIZE;

  // copy the values from `slotMapInBuffer` to `slotMap` (size is `slotCount`)
  memcpy(slotMap, slotMapInBuffer, slotCount);
  return SUCCESS;
}

int RecBuffer::setSlotMap(unsigned char *slotMap) {
    // Load the block into a buffer and get the pointer to the buffer
    unsigned char *bufferPtr;
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if (ret != SUCCESS)
        return ret;

    // get the header of the block and find numSlots
    struct HeadInfo head;
    ret = getHeader(&head);
    if (ret != SUCCESS)
        return ret;
    int numSlots = head.numSlots;

    // copy the slotMap into the buffer at the correct offset
    memcpy(bufferPtr + HEADER_SIZE, slotMap, numSlots);

    // set the dirty bit of the block in the buffer
    ret = StaticBuffer::setDirtyBit(this->blockNum);
    if (ret != SUCCESS)
        return ret;

    return SUCCESS;
}
int compareAttrs(union Attribute attr1, union Attribute attr2, int attrType) {

    double diff;
    if (attrType == STRING)
       diff = strcmp(attr1.sVal, attr2.sVal);

    else
        diff = attr1.nVal - attr2.nVal;

    
    if (diff > 0)  return 1;
    if (diff < 0) return -1;
      return 0;
    
}
