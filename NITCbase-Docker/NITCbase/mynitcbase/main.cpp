#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include<iostream>
#include<cstring>
#include<bits/stdc++.h>
#include<sstream>
using namespace std;
int main(int argc, char *argv[]) {
  Disk disk_run;
    StaticBuffer buffer;
//   // create objects for the relation catalog and attribute catalog
//   RecBuffer relCatBuffer(RELCAT_BLOCK);
//   RecBuffer attrCatBuffer(ATTRCAT_BLOCK);

//   HeadInfo relCatHeader;
//   HeadInfo attrCatHeader;
//   int currentBlock1=ATTRCAT_BLOCK;
//   // load the headers of both the blocks into relCatHeader and attrCatHeader.
//   // (we will implement these functions later)
//   relCatBuffer.getHeader(&relCatHeader);
//   attrCatBuffer.getHeader(&attrCatHeader);

//     while (currentBlock1 != -1) {
//       RecBuffer attrCatBuffer(currentBlock1);
//       HeadInfo attrCatHeader;
//       attrCatBuffer.getHeader(&attrCatHeader);

//       bool found = false;
//       for (int i = 0; i < attrCatHeader.numEntries; i++) {
//           Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
//           attrCatBuffer.getRecord(attrCatRecord, i);
//           if (strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, "Students") == 0 &&
//               strcmp(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, "Class") == 0) {
//               strcpy(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, "Batch");
//               attrCatBuffer.setRecord(attrCatRecord, i);
//               found = true;
//               break;
//           }
//       }
//       if (found)
//           break;
//       currentBlock1 = attrCatHeader.rblock;
//   }

//   //for // Relation loop
// for (int i = 0; i < relCatHeader.numEntries; i++) {

//     Attribute relCatRecord[RELCAT_NO_ATTRS];

//     relCatBuffer.getRecord(relCatRecord, i);

//     printf("Relation: %s\n", relCatRecord[RELCAT_REL_NAME_INDEX].sVal);

//     int currentBlock = ATTRCAT_BLOCK;

//     while (currentBlock != INVALID_BLOCKNUM) {

//         RecBuffer attrCatBuffer(currentBlock);
//         HeadInfo attrCatHeader;
//         attrCatBuffer.getHeader(&attrCatHeader);

//         for (int j = 0; j < attrCatHeader.numEntries; j++) {

//             Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
//             attrCatBuffer.getRecord(attrCatRecord, j);

//             if (strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal,relCatRecord[RELCAT_REL_NAME_INDEX].sVal) == 0) {

//                 const char *attrType = attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER ? "NUM": "STR";

//                 printf("  %s: %s\n", attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, attrType);
//             }
//         }

//         currentBlock = attrCatHeader.rblock;
//     }

//     printf("\n");
// }

  return 0;
}