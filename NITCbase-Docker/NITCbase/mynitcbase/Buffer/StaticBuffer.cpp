#include "StaticBuffer.h"

unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];

StaticBuffer::StaticBuffer() {
  for (int bufferIndex = 0; bufferIndex  < BUFFER_CAPACITY; bufferIndex++) {
    metainfo[bufferIndex].free = true;
  }
}

StaticBuffer::~StaticBuffer() {}

int StaticBuffer::getFreeBuffer(int blockNum) {
  if (blockNum < 0 || blockNum >= DISK_BLOCKS) {
    return E_OUTOFBOUND;
  }
  int allocatedBuffer;

  //Iterate through the eta data to find free block(Lru not implemented)
  for(int bufferBlock = 0; bufferBlock < BUFFER_CAPACITY; bufferBlock++){
    if(metainfo[bufferBlock].free){
        allocatedBuffer = bufferBlock;
        break;
    }
  }

  metainfo[allocatedBuffer].free = false;
  metainfo[allocatedBuffer].blockNum = blockNum;

  return allocatedBuffer;
}

/* Get the buffer index where a particular block is stored
   or E_BLOCKNOTINBUFFER otherwise
*/
int StaticBuffer::getBufferNum(int blockNum) {
    if(blockNum < 0 || blockNum >= DISK_BLOCKS)
        return E_OUTOFBOUND;

    for(int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++){
        if(metainfo[bufferIndex].blockNum == blockNum)
            return bufferIndex;
    }
    // if block is not in the buffer
    return E_BLOCKNOTINBUFFER;
}
