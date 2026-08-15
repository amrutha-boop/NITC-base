#include "BlockBuffer.h"

#include <cstdlib>
#include <cstring>


BlockBuffer::BlockBuffer(int blockNum) {
  this->blockNum = blockNum;
}

// calls the parent class constructor
RecBuffer::RecBuffer(int blockNum) : BlockBuffer::BlockBuffer(blockNum) {}

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

    HeadInfo head;
    this->getHeader(&head);

    if (slotNum < 0 || slotNum >= head.numSlots)
        return E_OUTOFBOUND;

    //Load block from buffer
    unsigned char *bufferPtr;
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if (ret != SUCCESS) {
      return ret;
    }

    // Calculate the record position
    int recordSize = head.numAttrs * ATTR_SIZE;
    int offset = HEADER_SIZE + head.numSlots + recordSize * slotNum;

    memcpy(bufferPtr + offset, rec, recordSize);

    Disk::writeBlock(bufferPtr, this->blockNum);

    return SUCCESS;
}

int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char **buffPtr){
  int bufferNum = StaticBuffer::getBufferNum(this->blockNum);

  //If bufferNum = -1
  if(bufferNum == E_BLOCKNOTINBUFFER){
    bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);
    if(bufferNum == E_OUTOFBOUND)
      return E_OUTOFBOUND;
    Disk::readBlock(StaticBuffer::blocks[bufferNum], this->blockNum); 
  }

  //Store the pionte to this buffer
  *buffPtr = StaticBuffer::blocks[bufferNum];
  return SUCCESS;
}