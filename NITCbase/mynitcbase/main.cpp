#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"

#include <iostream>

int main(int argc, char *argv[]) {
  Disk disk_run;

  // create objects for the relation catalog and attribute catalog
  RecBuffer relCatBuffer(RELCAT_BLOCK);

  HeadInfo relCatHeader;

  // load the headers of both the blocks into relCatHeader and attrCatHeader.
  relCatBuffer.getHeader(&relCatHeader);

  for (int i = 0; i < relCatHeader.numEntries; i++) {

    Attribute relCatRecord[RELCAT_NO_ATTRS]; // will store the record from the relation catalog

    relCatBuffer.getRecord(relCatRecord, i);

    printf("Relation: %s\n", relCatRecord[RELCAT_REL_NAME_INDEX].sVal);
    int currentBlock= ATTRCAT_BLOCK;
    while(currentBlock !=-1)
    {
      RecBuffer attrCatBuffer(currentBlock);
      HeadInfo attrCatHeader;
      attrCatBuffer.getHeader(&attrCatHeader);

      //class->batch in students
      for(int j=0; j<attrCatHeader.numEntries; j++)
      {
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        attrCatBuffer.getRecord(attrCatRecord, j);

        if((strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, "Students") == 0) && 
          (strcmp(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, "Class") == 0)) 
        {
          strcpy(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, "Batch");
          attrCatBuffer.setRecord(attrCatRecord,j);
          break;
        }
      }

      //printing all attributes
      for (int j = 0; j < attrCatHeader.numEntries; j++) {

        // declare attrCatRecord and load the attribute catalog entry into it
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        attrCatBuffer.getRecord(attrCatRecord, j);

        if (strcmp(relCatRecord[RELCAT_REL_NAME_INDEX].sVal,
            attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal) == 0) {
          const char *attrType = attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER ? "NUM" : "STR";
          printf("  %s: %s\n",
        attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,
        attrType);
        }
      }
      currentBlock= attrCatHeader.rblock;
    }
    
    printf("\n");
  }

  return 0;
}


