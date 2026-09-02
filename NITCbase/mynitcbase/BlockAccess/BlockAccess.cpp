#include "BlockAccess.h"

#include <cstring>


RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op) {
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