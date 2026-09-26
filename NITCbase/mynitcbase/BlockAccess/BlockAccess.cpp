#include "BlockAccess.h"

#include <cstring>


RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op) 
{
	// get the previous search index of the relation relId from the relation cache
	// (use RelCacheTable::getSearchIndex() function)
	RecId prevRecId;
	int ret= RelCacheTable::getSearchIndex(relId, &prevRecId);
	if(ret!=SUCCESS)
	{
		return RecId{-1, -1};
	}
	// let block and slot denote the record id of the record being currently checked
	int block;
	int slot;
	// if the current search index record is invalid(i.e. both block and slot = -1)
	if (prevRecId.block == -1 && prevRecId.slot == -1)
	{
		// (no hits from previous search; search should start from the
		// first record itself)
		RelCatEntry relCatEntry;
		// get the first record block of the relation from the relation cache
		// (use RelCacheTable::getRelCatEntry() function of Cache Layer)
		ret=RelCacheTable::getRelCatEntry(relId, &relCatEntry);
		if(ret!=SUCCESS)
		{
			return RecId{-1, -1};
		}
		block = relCatEntry.firstBlk;
		slot = 0;
	}
	else
	{
		// (there is a hit from previous search; search should start from
		// the record next to the search index record)
		block = prevRecId.block;
		slot = prevRecId.slot+1;
	}

	/* The following code searches for the next record in the relation
	that satisfies the given condition
	We start from the record id (block, slot) and iterate over the remaining
	records of the relation
	*/
	while (block != -1)
	{
		/* create a RecBuffer object for block (use RecBuffer Constructor for
		existing block) */
		RecBuffer recBuffer(block);
		// get the record with id (block, slot) using RecBuffer::getRecord()
		// get header of the block using RecBuffer::getHeader() function
		HeadInfo head;
		ret= recBuffer.getHeader(&head);
		if(ret!=SUCCESS)
		{
			return RecId{-1,-1};
		}
		// get slot map of the block using RecBuffer::getSlotMap() function
		int slotCount=head.numSlots;
		unsigned char slotMap[slotCount];
		ret=recBuffer.getSlotMap(slotMap);
		if(ret!=SUCCESS)
		{
			return RecId{-1,-1};
		}
		// If slot >= the number of slots per block(i.e. no more slots in this block)
		if(slot>=slotCount)
		{
			block=head.rblock;
			slot=0;
			continue;
		}

		if(slotMap[slot]==SLOT_UNOCCUPIED)
		{
			slot++;
			continue;
		}

		// compare record's attribute value to the the given attrVal as below:
		Attribute record[head.numAttrs];
    ret = recBuffer.getRecord(record, slot);
    if (ret != SUCCESS) 
		{
      return RecId{-1, -1};
    }
		AttrCatEntry attrCatEntry;
    ret = AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);
    if (ret != SUCCESS)
		{
      return RecId{-1, -1};
    }

		Attribute recordAttr = record[attrCatEntry.offset];
    int cmpVal = compareAttrs(recordAttr, attrVal, attrCatEntry.attrType);

		/* Next task is to check whether this record satisfies the given condition.
		It is determined based on the output of previous comparison and
		the op value received.
		The following code sets the cond variable if the condition is satisfied.
		*/
		if (
			(op == NE && cmpVal != 0) ||    // if op is "not equal to"
			(op == LT && cmpVal < 0) ||     // if op is "less than"
			(op == LE && cmpVal <= 0) ||    // if op is "less than or equal to"
			(op == EQ && cmpVal == 0) ||    // if op is "equal to"
			(op == GT && cmpVal > 0) ||     // if op is "greater than"
			(op == GE && cmpVal >= 0)       // if op is "greater than or equal to"
		) 
		{
			/*
			set the search index in the relation cache as
			the record id of the record that satisfies the given condition
			(use RelCacheTable::setSearchIndex function)
			*/
			RecId currentRecId;
      currentRecId.block = block;
      currentRecId.slot = slot;
      // Update search index to this newly found record
      ret = RelCacheTable::setSearchIndex(relId, &currentRecId);
      if (ret != SUCCESS) 
			{
        return RecId{-1, -1};
      }
      return currentRecId;
		}
		slot++;
	}

	// no record in the relation with Id relid satisfies the given condition
	return RecId{-1, -1};
}



int BlockAccess::renameRelation(char oldName[ATTR_SIZE], char newName[ATTR_SIZE])
{
	/* reset the searchIndex of the relation catalog using
			RelCacheTable::resetSearchIndex() */
	RelCacheTable::resetSearchIndex(RELCAT_RELID);

  Attribute newRelationName;    // set newRelationName with newName
	strcpy(newRelationName.sVal, newName);

  // search the relation catalog for an entry with "RelName" = newRelationName
	char relNameAttr[ATTR_SIZE];
	strcpy(relNameAttr, RELCAT_ATTR_RELNAME);
	RecId recId = linearSearch(RELCAT_RELID, relNameAttr, newRelationName, EQ);
	// If relation with name newName already exists (result of linearSearch
	//                                               is not {-1, -1})
	if (recId.block != -1 && recId.slot != -1)
		return E_RELEXIST;

	/* reset the searchIndex of the relation catalog using
			RelCacheTable::resetSearchIndex() */
	RelCacheTable::resetSearchIndex(RELCAT_RELID);

  Attribute oldRelationName;    // set oldRelationName with oldName
	strcpy(oldRelationName.sVal, oldName);

  // search the relation catalog for an entry with "RelName" = oldRelationName
	recId = linearSearch(RELCAT_RELID, relNameAttr, oldRelationName, EQ);
	
	if(recId.block == -1 && recId.slot == -1)
	{
		return E_RELNOTEXIST;
	}

	/* get the relation catalog record of the relation to rename using a RecBuffer
			on the relation catalog [RELCAT_BLOCK] and RecBuffer.getRecord function
	*/
	RecBuffer recBuffer(recId.block);
	
	Attribute record[RELCAT_NO_ATTRS];
	int ret = recBuffer.getRecord(record, recId.slot);
	if (ret != SUCCESS)
	{
		return ret;
	}

	int numAttrs = (int)record[RELCAT_NO_ATTRIBUTES_INDEX].nVal;


	strcpy(record[RELCAT_REL_NAME_INDEX].sVal, newName);

	// set back the record value using RecBuffer.setRecord
	ret = recBuffer.setRecord(record, recId.slot);
	if (ret != SUCCESS)
	{
		return ret;
	}


	// reset the searchIndex of the attribute catalog using
	RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

	for (int i = 0; i <numAttrs; i++)
	{
  		char attrRelName[ATTR_SIZE];
		strcpy(attrRelName, ATTRCAT_ATTR_RELNAME);
		RecId attrRecId = linearSearch(ATTRCAT_RELID, attrRelName, oldRelationName, EQ);
    if (attrRecId.block == -1 && attrRecId.slot == -1)
    {
      return E_RELNOTEXIST;
    }

    RecBuffer attrBuffer(attrRecId.block);

    Attribute attrRecord[ATTRCAT_NO_ATTRS];
    ret = attrBuffer.getRecord(attrRecord, attrRecId.slot);
    if (ret != SUCCESS)
    {
      return ret;
    }

    strcpy(attrRecord[ATTRCAT_REL_NAME_INDEX].sVal, newName);

    ret = attrBuffer.setRecord(attrRecord, attrRecId.slot);
    if (ret != SUCCESS)
    {
      return ret;
    }
	}
	return SUCCESS;
}



int BlockAccess::renameAttribute(char relName[ATTR_SIZE], char oldName[ATTR_SIZE], char newName[ATTR_SIZE]) {

	// reset the searchIndex of the relation catalog using
	RelCacheTable::resetSearchIndex(RELCAT_RELID);

	Attribute relNameAttr;    // set relNameAttr to relName
	strcpy(relNameAttr.sVal, relName);

	char relNameAttrName[ATTR_SIZE];
	strcpy(relNameAttrName, RELCAT_ATTR_RELNAME);

	// Search for the relation with name relName in relation catalog using linearSearch()
	RecId relRecId = linearSearch(RELCAT_RELID, relNameAttrName, relNameAttr, EQ);
	
	// If relation with name relName does not exist (search returns {-1,-1})
	if (relRecId.block == -1 && relRecId.slot == -1)	
		return E_RELNOTEXIST;

	// reset the searchIndex of the attribute catalog using
	RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

	/* declare variable attrToRenameRecId used to store the attr-cat recId
	of the attribute to rename */
	RecId attrToRenameRecId{-1, -1};
	Attribute attrCatEntryRecord[ATTRCAT_NO_ATTRS];

	/* iterate over all Attribute Catalog Entry record corresponding to the
		relation to find the required attribute */
	while (true) 
	{
		// linear search on the attribute catalog for RelName = relNameAttr
		char attrRelNameAttr[ATTR_SIZE];
		strcpy(attrRelNameAttr, ATTRCAT_ATTR_RELNAME);
		RecId attrRecId = linearSearch(ATTRCAT_RELID,attrRelNameAttr,relNameAttr,EQ);
		if (attrRecId.block == -1 && attrRecId.slot == -1)
		{
		break;
		}

		/* Get the record from the attribute catalog using RecBuffer.getRecord
			into attrCatEntryRecord */
		RecBuffer attrBuffer(attrRecId.block);
		int ret = attrBuffer.getRecord( attrCatEntryRecord,attrRecId.slot);
    if (ret != SUCCESS)
    {
      return ret;
    }
		// if attrCatEntryRecord.attrName = oldName
		//     attrToRenameRecId = block and slot of this record
		if (strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, oldName) == 0)
		{
    	attrToRenameRecId = attrRecId;
		}
		// if attrCatEntryRecord.attrName = newName
		//     return E_ATTREXIST;
		if (strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName) == 0)
		{
    	return E_ATTREXIST;
		}
	}

	// if attrToRenameRecId == {-1, -1}
	//     return E_ATTRNOTEXIST;
	if (attrToRenameRecId.block == -1 && attrToRenameRecId.slot == -1)
	{
    return E_ATTRNOTEXIST;
	}


	RecBuffer attrBuffer(attrToRenameRecId.block);

	int ret = attrBuffer.getRecord(attrCatEntryRecord,attrToRenameRecId.slot);
	if (ret != SUCCESS)
	{
		return ret;
	}

	strcpy(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,newName);

	ret = attrBuffer.setRecord(attrCatEntryRecord,attrToRenameRecId.slot);
	if (ret != SUCCESS)
	{
		return ret;
	}

	return SUCCESS;
}



int BlockAccess::insert(int relId, Attribute *record) 
{
	// get the relation catalog entry from relation cache
	RelCatEntry relCatEntry;
	int ret = RelCacheTable::getRelCatEntry(relId, &relCatEntry);
	if (ret != SUCCESS)
	{
		return ret;
	}

	int blockNum = relCatEntry.firstBlk;

	// rec_id will be used to store where the new record will be inserted
	RecId rec_id = {-1, -1};

	int numOfSlots = relCatEntry.numSlotsPerBlk;
	int numOfAttributes = relCatEntry.numAttrs;
	int prevBlockNum = -1;

	/*
		Traversing the linked list of existing record blocks of the relation
		until a free slot is found OR
		until the end of the list is reached
	*/
	while (blockNum != -1) 
	{
		// create a RecBuffer object for blockNum (using appropriate constructor!)
		RecBuffer recBuffer(blockNum);	

		// get header of block(blockNum) using RecBuffer::getHeader() function
		HeadInfo head;
    ret = recBuffer.getHeader(&head);
    if (ret != SUCCESS)
    {
      return ret;
    }

		// get slot map of block(blockNum) using RecBuffer::getSlotMap() function
		unsigned char slotMap[numOfSlots];
    ret = recBuffer.getSlotMap(slotMap);
    if (ret != SUCCESS)
    {
      return ret;
    }

		// search for free slot in the block 'blockNum' and store it's rec-id in rec_id
		for (int slot = 0; slot < numOfSlots; slot++)
    {
			if (slotMap[slot] == SLOT_UNOCCUPIED)
			{
				rec_id.block = blockNum;
				rec_id.slot = slot;
				break;
			}
    }

		// free slot found
		if (rec_id.block != -1)
    {
      break;
    }

		//free slot not found
		prevBlockNum = blockNum;
    blockNum = head.rblock;
	}

	//  if no free slot is found in existing record blocks (rec_id = {-1, -1})
	if (rec_id.block == -1)
	{
		// if relation is RELCAT, do not allocate any more blocks
		if (relId == RELCAT_RELID)
    {
      return E_MAXRELATIONS;
    }

		// Otherwise,
		RecBuffer newRecBuffer;
		ret = newRecBuffer.getBlockNum();
		if (ret == E_DISKFULL) 
		{
			return E_DISKFULL;
		}

		// Assign rec_id.block = new block number(i.e. ret) and rec_id.slot = 0
		rec_id.block = ret;
    rec_id.slot = 0;

		//set header for newly allocated block
		HeadInfo head;
		head.blockType = REC;
		head.pblock = -1;
		head.lblock = prevBlockNum;
		head.rblock = -1;
		head.numEntries = 0;
		head.numAttrs = numOfAttributes;
		head.numSlots = numOfSlots;

		ret = newRecBuffer.setHeader(&head);
		if (ret != SUCCESS)
		{
			return ret;
		}

		unsigned char slotMap[numOfSlots];
		for (int i = 0; i < numOfSlots; i++)
		{
			slotMap[i] = SLOT_UNOCCUPIED;
		}

		ret = newRecBuffer.setSlotMap(slotMap);
		if (ret != SUCCESS)
		{
			return ret;
		}

		if (prevBlockNum != -1)
		{
			// create a RecBuffer object for prevBlockNum
			RecBuffer prevRecBuffer(prevBlockNum);

			// get the header of the block prevBlockNum
			HeadInfo prevHead;
			ret = prevRecBuffer.getHeader(&prevHead);
			if (ret != SUCCESS)
			{
				return ret;
			}

			// update the rblock field of the header to the new block
			prevHead.rblock = rec_id.block;

			ret = prevRecBuffer.setHeader(&prevHead);
			if (ret != SUCCESS)
			{
				return ret;
			
			}
		}
		else
		{
			// update first block field in the relation catalog entry to the new block
			relCatEntry.firstBlk = rec_id.block;
			ret = RelCacheTable::setRelCatEntry(relId, &relCatEntry);
			if (ret != SUCCESS)
			{
				return ret;
			}
		}

		// update last block field in the relation catalog entry to the new block
		relCatEntry.lastBlk = rec_id.block;
		ret = RelCacheTable::setRelCatEntry(relId, &relCatEntry);
		if (ret != SUCCESS)
		{
			return ret;
		}
	}

	// create a RecBuffer object for rec_id.block
	RecBuffer recBuffer(rec_id.block);

	// insert the record into rec_id'th slot using RecBuffer.setRecord())
	ret = recBuffer.setRecord(record, rec_id.slot);
	if (ret != SUCCESS)
	{
		return ret;
	}

	unsigned char slotMap[numOfSlots];
	ret = recBuffer.getSlotMap(slotMap);
	if (ret != SUCCESS)
	{
		return ret;
	}

	slotMap[rec_id.slot] = SLOT_OCCUPIED;
	ret = recBuffer.setSlotMap(slotMap);
	if (ret != SUCCESS)
	{
		return ret;
	}

	// increment the numEntries field in the header of the block to which record was inserted
	HeadInfo head;
	ret = recBuffer.getHeader(&head);
	if (ret != SUCCESS)
	{
		return ret;
	}

	head.numEntries++;
	ret = recBuffer.setHeader(&head);
	if (ret != SUCCESS)
	{
		return ret;
	}

	// Increment the number of records field in the relation cache entry for the relation
	relCatEntry.numRecs++;
	ret = RelCacheTable::setRelCatEntry(relId, &relCatEntry);
	if (ret != SUCCESS)
	{
		return ret;
	}

	return SUCCESS;
}