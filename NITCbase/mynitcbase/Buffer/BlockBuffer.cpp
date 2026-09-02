#include "BlockBuffer.h"

#include <cstdlib>
#include <cstring>

BlockBuffer::BlockBuffer(int blockNum) {
  this->blockNum= blockNum;
}

// calls the parent class constructor
RecBuffer::RecBuffer(int blockNum) : BlockBuffer::BlockBuffer(blockNum) {}

// load the block header into the argument pointer
int BlockBuffer::getHeader(struct HeadInfo *head) {

  unsigned char *bufferPtr;
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) 
  {
    return ret;   // return any errors that might have occured in the process
  }

  // populate the numEntries, numAttrs and numSlots fields in *head
  memcpy(&head->numSlots, bufferPtr + 24, 4);
  memcpy(&head->numAttrs, bufferPtr + 20, 4);
  memcpy(&head->numEntries, bufferPtr + 16, 4);
  memcpy(&head->rblock, bufferPtr + 12, 4);
  memcpy(&head->lblock, bufferPtr + 8, 4);

  return SUCCESS;
}

// load the record at slotNum into the argument pointer
int RecBuffer::getRecord(union Attribute *rec, int slotNum) {
  struct HeadInfo head;

  // get the header using this.getHeader() function
  this->getHeader(&head);

  int attrCount = head.numAttrs;
  int slotCount = head.numSlots;

  unsigned char *bufferPtr;
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) 
  {
    return ret;
  }

  int recordSize = attrCount * ATTR_SIZE;
  unsigned char *slotPointer = bufferPtr+ HEADER_SIZE+ slotCount + (recordSize*slotNum);

  // load the record into the rec data structure
  memcpy(rec, slotPointer, recordSize);

  return SUCCESS;
}

int RecBuffer::setRecord(union Attribute *rec, int slotNum)
{
  unsigned char *bufferPtr;
  // get the buffer containing the block
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS)
  {
    return ret;
  }
  // get the header of the block
  struct HeadInfo head;
  ret = this->getHeader(&head);
  if (ret != SUCCESS)
  {
    return ret;
  }

  int attrCount = head.numAttrs;
  int slotCount = head.numSlots;
  // check if slotNum is valid
  if (slotNum < 0 || slotNum >= slotCount)
  {
    return E_OUTOFBOUND;
  }

  // calculate the size of one record
  int recordSize = attrCount * ATTR_SIZE;
  // point to the required record in the buffer
  unsigned char *slotPointer = bufferPtr + HEADER_SIZE + slotCount + (recordSize * slotNum);

  // copy the input record into the buffer
  memcpy(slotPointer, rec, recordSize);

  // mark the buffer as dirty
  ret = StaticBuffer::setDirtyBit(this->blockNum);
  if (ret != SUCCESS)
  {
    return ret;
  }

  return SUCCESS;
}

int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char **buffPtr) 
{
  // check whether the block is already present in the buffer
  int bufferNum = StaticBuffer::getBufferNum(this->blockNum);

  // blockNum is outside the valid range
  if (bufferNum == E_OUTOFBOUND)
  {
    return E_OUTOFBOUND;
  }

  // block is already present in the buffer
  if (bufferNum != E_BLOCKNOTINBUFFER)
  {
    // increment timestamp of all other occupied buffers
    for (int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++)
    {
      if (bufferIndex != bufferNum && StaticBuffer::metainfo[bufferIndex].free == false)
      {
        StaticBuffer::metainfo[bufferIndex].timeStamp++;
      }
    }
    // this buffer was just accessed, so reset its timestamp
    StaticBuffer::metainfo[bufferNum].timeStamp = 0;
  }
  else
  {
    // block is not in buffer, so allocate a buffer
    bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);
    if (bufferNum == E_OUTOFBOUND)
    {
      return E_OUTOFBOUND;
    }
    // load the block from disk into the allocated buffer
    Disk::readBlock(StaticBuffer::blocks[bufferNum], this->blockNum);
  }
  // store pointer to the buffer containing the block
  *buffPtr = StaticBuffer::blocks[bufferNum];

  return SUCCESS;
}



/* used to get the slotmap from a record block
NOTE: this function expects the caller to allocate memory for `*slotMap`
*/
int RecBuffer::getSlotMap(unsigned char *slotMap) 
{
  unsigned char *bufferPtr;
  // get the starting address of the buffer containing the block using loadBlockAndGetBufferPtr().
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) 
  {
    return ret;
  }
  // get the header of the block using getHeader() function
  struct HeadInfo head;
  this->getHeader(&head);
  int slotCount =head.numSlots;
  // get a pointer to the beginning of the slotmap in memory by offsetting HEADER_SIZE
  unsigned char *slotMapInBuffer = bufferPtr + HEADER_SIZE;
  // copy the values from `slotMapInBuffer` to `slotMap` (size is `slotCount`)
  memcpy(slotMap, slotMapInBuffer, slotCount);
  return SUCCESS;
}


int compareAttrs(Attribute attr1, Attribute attr2, int attrType)
{ 
  double diff;
  if(attrType == STRING)
  {
    diff=strcmp(attr1.sVal, attr2.sVal);
  }
  else
  {
    diff=attr1.nVal- attr2.nVal;
  }
  if(diff>0)
  {
    return 1;
  }
  if(diff<0)
  {
    return -1;
  }
  return 0;
}