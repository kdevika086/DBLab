#include "OpenRelTable.h"
#include <cstring>
#include <cstdlib>
OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN];
OpenRelTable::OpenRelTable() {

  // initialize relCache and attrCache with nullptr
  for (int i = 0; i < MAX_OPEN; ++i) {
    RelCacheTable::relCache[i] = nullptr;
    AttrCacheTable::attrCache[i] = nullptr;
    tableMetaInfo[i].free = true;
  }

  tableMetaInfo[RELCAT_RELID].free = false;
	strcpy(tableMetaInfo[RELCAT_RELID].relName, "RELATIONCAT");

	tableMetaInfo[ATTRCAT_RELID].free = false;
	strcpy(tableMetaInfo[ATTRCAT_RELID].relName, "ATTRIBUTECAT");

	//part of exercise 1
	tableMetaInfo[2].free = false;
	strcpy(tableMetaInfo[2].relName, "Students");
	//ends here

  /**** setting up Relation Catalog relation in the Relation Cache Table****/
  RecBuffer relCatBlock(RELCAT_BLOCK);

  Attribute relCatRecord[RELCAT_NO_ATTRS];
  relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_RELCAT);

  struct RelCacheEntry relCacheEntry;
  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
  relCacheEntry.recId.block = RELCAT_BLOCK;
  relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_RELCAT;

  // allocate this on the heap because we want it to persist outside this function
  RelCacheTable::relCache[RELCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[RELCAT_RELID]) = relCacheEntry;

	//part of exercise 1
	// Set up Students in the relation cache
	Attribute record[RELCAT_NO_ATTRS];
	relCatBlock.getRecord(record, 2);

	RelCacheEntry studentsEntry;
	RelCacheTable::recordToRelCatEntry(record, &studentsEntry.relCatEntry);
	studentsEntry.recId.block = RELCAT_BLOCK;
	studentsEntry.recId.slot = 2;

	RelCacheTable::relCache[2] = (RelCacheEntry*)malloc(sizeof(RelCacheEntry));
	*(RelCacheTable::relCache[2]) = studentsEntry;
	//ends here

  /**** setting up Attribute Catalog relation in the Relation Cache Table ****/
  Attribute attrCatRelRecord[RELCAT_NO_ATTRS];
  relCatBlock.getRecord(attrCatRelRecord, RELCAT_SLOTNUM_FOR_ATTRCAT);

  struct RelCacheEntry attrCatRelCacheEntry;
  RelCacheTable::recordToRelCatEntry(attrCatRelRecord, &attrCatRelCacheEntry.relCatEntry);
  attrCatRelCacheEntry.recId.block = RELCAT_BLOCK;
  attrCatRelCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT;

  RelCacheTable::relCache[ATTRCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));

  *(RelCacheTable::relCache[ATTRCAT_RELID]) = attrCatRelCacheEntry;







  /************ Setting up Attribute cache entries ************/
  // (we need to populate attribute cache with entries for the relation catalog
  //  and attribute catalog.)

  /**** setting up Relation Catalog relation in the Attribute Cache Table ****/
  RecBuffer attrCatBlock(ATTRCAT_BLOCK);
  Attribute attrCatRecord[ATTRCAT_NO_ATTRS];

  AttrCacheEntry* prev = nullptr;
  for (int i = 0; i < ATTRCAT_NO_ATTRS; i++) 
	{
		attrCatBlock.getRecord(attrCatRecord, i);
		AttrCacheEntry* entry =(AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
		AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &entry->attrCatEntry);
		entry->recId.block = ATTRCAT_BLOCK;
		entry->recId.slot = i;
		entry->next = nullptr;
		//linking part
		if (i == 0) 
		{
			AttrCacheTable::attrCache[RELCAT_RELID] = entry;
		}
		else 
		{
			prev->next = entry;
		}
		prev = entry;
  }

	//part of Exercise 1
	AttrCacheEntry* prevStudents = nullptr;
	int currentBlock = ATTRCAT_BLOCK;
	while (currentBlock != -1) 
	{
		RecBuffer attrCatBuffer(currentBlock);

		HeadInfo attrCatHeader;
		attrCatBuffer.getHeader(&attrCatHeader);

		for (int i = 0; i < attrCatHeader.numEntries; i++) 
		{
			attrCatBuffer.getRecord(attrCatRecord, i);
			if (strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal,"Students") == 0) 
			{
				AttrCacheEntry* entry =(AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
				AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &entry->attrCatEntry);

				entry->recId.block = currentBlock;
				entry->recId.slot = i;
				entry->next = nullptr;

				if (prevStudents == nullptr) 
				{
					AttrCacheTable::attrCache[2] = entry;
				}
				else 
				{
					prevStudents->next = entry;
				}
				prevStudents = entry;
			}
		}
		currentBlock = attrCatHeader.rblock;
	}

  /**** setting up Attribute Catalog relation in the Attribute Cache Table ****/
  AttrCacheEntry* prevAttr = nullptr;
  for (int i = 6; i < ATTRCAT_NO_ATTRS * 2; i++) 
	{
		attrCatBlock.getRecord(attrCatRecord, i);
		AttrCacheEntry* entry = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
		AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &entry->attrCatEntry);

		entry->recId.block = ATTRCAT_BLOCK;
		entry->recId.slot = i;
		entry->next = nullptr;
		//linking part
		if (i == 6) 
		{
			AttrCacheTable::attrCache[ATTRCAT_RELID] = entry;
		}
		else 
		{	
			prevAttr->next = entry;
		}
		prevAttr = entry;
	}
}

int OpenRelTable::getFreeOpenRelTableEntry() 
{
	for (int i = 0; i < MAX_OPEN; i++) 
	{
		if (tableMetaInfo[i].free == true) 
		{
			return i;
		}
	}
	return E_MAXRELATIONS;
}

OpenRelTable::~OpenRelTable() {
  // free all the memory that you allocated in the constructor
  free(RelCacheTable::relCache[RELCAT_RELID]);
  free(RelCacheTable::relCache[ATTRCAT_RELID]);
	free(RelCacheTable::relCache[2]);

  // Free RELATIONCAT attribute cache linked list
  AttrCacheEntry* current = AttrCacheTable::attrCache[RELCAT_RELID];

  while (current != nullptr) {
		AttrCacheEntry* next = current->next;
		free(current);
		current = next;
  }

  // Free ATTRIBUTECAT attribute cache linked list
  current = AttrCacheTable::attrCache[ATTRCAT_RELID];

  while (current != nullptr) {
		AttrCacheEntry* next = current->next;
		free(current);
		current = next;
  }
	current = AttrCacheTable::attrCache[2];

	while (current != nullptr) {
		AttrCacheEntry* next = current->next;
		free(current);
		current = next;
	}
}




// int OpenRelTable::getRelId(char relName[ATTR_SIZE]) 
// {

//     for (int i = 0; i < MAX_OPEN; i++) {
// 			if (tableMetaInfo[i].free == false &&
// 			strcmp(tableMetaInfo[i].relName, relName) == 0) 
// 			{
// 				return i;
// 			}
//     }

//     return E_RELNOTOPEN;
// }
