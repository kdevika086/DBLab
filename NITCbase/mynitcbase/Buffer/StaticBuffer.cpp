#include "StaticBuffer.h"

unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];

struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];

StaticBuffer::StaticBuffer() 
{
  // initialise all blocks as free
  for (int bufferIndex=0 ; bufferIndex< BUFFER_CAPACITY; bufferIndex+=1)
	{
  	metainfo[bufferIndex].free = true;
    metainfo[bufferIndex].dirty = false;
    metainfo[bufferIndex].timeStamp = -1;
    metainfo[bufferIndex].blockNum = -1;
  }
}

StaticBuffer::~StaticBuffer()
{
  for (int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++)
  {
    if (metainfo[bufferIndex].free == false && metainfo[bufferIndex].dirty == true)
    {
      Disk::writeBlock(blocks[bufferIndex],metainfo[bufferIndex].blockNum);
    }
  }
}


int StaticBuffer::getFreeBuffer(int blockNum) 
{
	//check if blockNum is valid
  if (blockNum < 0 || blockNum >= DISK_BLOCKS) 
	{
    return E_OUTOFBOUND;
  }

  // increase the timeStamp of all occupied buffers
  for (int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++)
  {
    if (metainfo[bufferIndex].free == false)
    {
      metainfo[bufferIndex].timeStamp++;
    }
  }

  // bufferNum stores the buffer that will be allocated
  int bufferNum = -1;

  // first look for a free buffer
  for (int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++)
  {
    if (metainfo[bufferIndex].free == true)
    {
      bufferNum = bufferIndex;
      break;
    }
  }

  // if no free buffer exists, find the LRU buffer
  if (bufferNum == -1)
  {
    int maxTimeStamp = -1;
    for (int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++)
    {
      if (metainfo[bufferIndex].timeStamp > maxTimeStamp)
      {
        maxTimeStamp = metainfo[bufferIndex].timeStamp;
        bufferNum = bufferIndex;
      }
    }

    // write back the LRU buffer if it is dirty
    if (metainfo[bufferNum].dirty == true)
    {
      Disk::writeBlock(blocks[bufferNum], metainfo[bufferNum].blockNum);
    }
  }

  // update metadata for the allocated buffer
  metainfo[bufferNum].free = false;
  metainfo[bufferNum].dirty = false;
  metainfo[bufferNum].blockNum = blockNum;
  metainfo[bufferNum].timeStamp = 0;

  return bufferNum;
}

/* Get the buffer index where a particular block is stored
   or E_BLOCKNOTINBUFFER otherwise
*/
int StaticBuffer::getBufferNum(int blockNum) 
{
	//check if blockNum is valid
  if (blockNum < 0 || blockNum >= DISK_BLOCKS) 
	{
    return E_OUTOFBOUND;
  }
  // find and return the bufferIndex which corresponds to blockNum (check metainfo)
  for (int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++) 
	{

    if (metainfo[bufferIndex].free == false &&
      metainfo[bufferIndex].blockNum == blockNum) 
		{
			return bufferIndex;
    }
  }
  // if block is not in the buffer
  return E_BLOCKNOTINBUFFER;
}


int StaticBuffer::setDirtyBit(int blockNum)
{
  // find the buffer corresponding to blockNum
  int bufferNum = getBufferNum(blockNum);
  // blockNum is outside the valid range
  if (bufferNum == E_OUTOFBOUND)
  {
    return E_OUTOFBOUND;
  }
  // block is not present in the buffer
  if (bufferNum == E_BLOCKNOTINBUFFER)
  {
    return E_BLOCKNOTINBUFFER;
  }
  // mark the buffer as dirty
  metainfo[bufferNum].dirty = true;
  return SUCCESS;
}