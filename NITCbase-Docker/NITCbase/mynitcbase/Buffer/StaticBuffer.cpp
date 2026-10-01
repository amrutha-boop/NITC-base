#include "StaticBuffer.h"

unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];
unsigned char StaticBuffer::blockAllocMap[DISK_BLOCKS];
// declare the blockAllocMap array

StaticBuffer::StaticBuffer() {
      //Copy Block Alloc Map from disk to buffer
    for (int i = 0; i < 4; i++) {
        Disk::readBlock(blockAllocMap + (i * BLOCK_SIZE), i);
    }

    // Initialize the remaining blocks in the buffer pool as free
  for (int bufferIndex = 0; bufferIndex  < BUFFER_CAPACITY; bufferIndex++) {
    metainfo[bufferIndex].free = true;
    metainfo[bufferIndex].dirty= false;
    metainfo[bufferIndex].timeStamp=-1;
    metainfo[bufferIndex].blockNum=-1;
  }
}

StaticBuffer::~StaticBuffer() {
        //Copy Block Alloc Map from buffer to disk
    for (int i = 0; i < 4; i++) {
        Disk::writeBlock(
            blockAllocMap + (i * BLOCK_SIZE),
            i
        );
    }
      // Write back all dirty blocks to disk
    for (int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++) {  
        if (!metainfo[bufferIndex].free && metainfo[bufferIndex].dirty) {
            int blockNum = metainfo[bufferIndex].blockNum;
            Disk::writeBlock(blocks[bufferIndex], blockNum);
        }
    }
}

int StaticBuffer::getFreeBuffer(int blockNum){
    // Check if blockNum is valid
    if(blockNum < 0 || blockNum >= DISK_BLOCKS)
        return E_OUTOFBOUND;

    // increase the timeStamp in metaInfo of all occupied buffers.
    for(int i = 0; i < BUFFER_CAPACITY; i++){
        if(!metainfo[i].free){
            metainfo[i].timeStamp++;
        }
    }

    // find a free buffer in the buffer pool and set bufferNum = index of that free buffer.
    int bufferNum = -1;
    for(int i = 0; i < BUFFER_CAPACITY; i++){
        if(metainfo[i].free){
            bufferNum = i;
            break;
        }
    }

    // if a free buffer is available, set bufferNum = index of that free buffer.
    if(bufferNum != -1){
        metainfo[bufferNum].free = false;
        metainfo[bufferNum].dirty = false;
        metainfo[bufferNum].blockNum = blockNum;
        metainfo[bufferNum].timeStamp = 0;
        return bufferNum;
    }

    // if no free buffer is available, find the buffer with the maximum timeStamp and set bufferNum = index of that buffer.
    int maxTimeStamp = -1;
    for(int i = 0; i < BUFFER_CAPACITY; i++){
        if(metainfo[i].timeStamp > maxTimeStamp){
            maxTimeStamp = metainfo[i].timeStamp;
            bufferNum = i;
        }
    }

    // if the buffer with the maximum timeStamp is dirty, write it back to the disk
    if(metainfo[bufferNum].dirty){
        Disk::writeBlock(blocks[bufferNum], metainfo[bufferNum].blockNum);
    }

    // update the metaInfo entry corresponding to bufferNum with
    metainfo[bufferNum].free = false;
    metainfo[bufferNum].dirty = false;
    metainfo[bufferNum].blockNum = blockNum;
    metainfo[bufferNum].timeStamp = 0;

    return bufferNum;
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

int StaticBuffer::setDirtyBit(int blockNum){
    // find the buffer index corresponding to the block using getBufferNum().
    int bufferNum = getBufferNum(blockNum);
    // if block is not present in the buffer (bufferNum = E_BLOCKNOTINBUFFER)
    //     return E_BLOCKNOTINBUFFER
        if(bufferNum == E_BLOCKNOTINBUFFER)
        return E_BLOCKNOTINBUFFER;
    // if blockNum is out of bound (bufferNum = E_OUTOFBOUND)
    //     return E_OUTOFBOUND
          if(bufferNum == E_OUTOFBOUND)
        return E_OUTOFBOUND;
    // else
    //     (the bufferNum is valid)
    //     set the dirty bit of that buffer to true in metainfo
        metainfo[bufferNum].dirty = true;

    return SUCCESS;
}
