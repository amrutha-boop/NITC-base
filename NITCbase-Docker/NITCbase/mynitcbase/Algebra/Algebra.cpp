#include "Algebra.h"
#include<cstdio>
#include <cstring>
#include<bits/stdc++.h>

/* used to select all the records that satisfy a condition.
the arguments of the function are
- srcRel - the source relation we want to select from
- targetRel - the relation we want to select into. (ignore for now)
- attr - the attribute that the condition is checking
- op - the operator of the condition
- strVal - the value that we want to compare against (represented as a string)
*/
// will return if a string can be parsed as a floating point number
//is the string a floating point number 
bool isNumber(char *str) {
  int len;
  float ignore;
  /*
    sscanf returns the number of elements read, so if there is no float matching
    the first %f, ret will be 0, else it'll be 1

    %n gets the number of characters read. this scanf sequence will read the
    first float ignoring all the whitespace before and after. and the number of
    characters read that far will be stored in len. if len == strlen(str), then
    the string only contains a float with/without whitespace. else, there's other
    characters.
  */
  int ret = sscanf(str, "%f %n", &ignore, &len);
  return ret == 1 && len == strlen(str);
}

int Algebra::select(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], char attr[ATTR_SIZE], int op, char strVal[ATTR_SIZE]) {
  int srcRelId = OpenRelTable::getRelId(srcRel);  
  //printf("%d",srcRelId);    // we'll implement this later
  if (srcRelId == E_RELNOTOPEN) {
    return E_RELNOTOPEN;
  }

  AttrCatEntry attrCatEntry;
  // get the attribute catalog entry for attr, using AttrCacheTable::getAttrcatEntry()
  //    return E_ATTRNOTEXIST if it returns the error
  int ret=AttrCacheTable::getAttrCatEntry(srcRelId,attr,&attrCatEntry);
  if(ret==E_ATTRNOTEXIST){
    return E_ATTRNOTEXIST;
  }


  /*** Convert strVal (string) to an attribute of data type NUMBER or STRING ***/
  int type = attrCatEntry.attrType;
  Attribute attrVal;
  if (type == NUMBER) {
    if (isNumber(strVal)) {      
      attrVal.nVal = atof(strVal);
    } else {
  
      return E_ATTRTYPEMISMATCH;
    }
  } else if (type == STRING) {
    strcpy(attrVal.sVal, strVal);
  }

  /*** Selecting records from the source relation ***/

  // Before calling the search function, reset the search to start from the first hit
  // using RelCacheTable::resetSearchIndex()
  RelCacheTable::resetSearchIndex(srcRelId);
  RelCatEntry relCatEntry;
  // get relCatEntry using RelCacheTable::getRelCatEntry()
  RelCacheTable::getRelCatEntry(srcRelId,&relCatEntry);
  /************************
  The following code prints the contents of a relation directly to the output
  console. Direct console output is not permitted by the actual the NITCbase
  specification and the output can only be inserted into a new relation. We will
  be modifying it in the later stages to match the specification.
  ************************/

  printf("|");
  for (int i = 0; i < relCatEntry.numAttrs; ++i) {
    AttrCatEntry attrCatEntry;
    // get attrCatEntry at offset i using AttrCacheTable::getAttrCatEntry()
    AttrCacheTable::getAttrCatEntry(srcRelId,i,&attrCatEntry);
    printf(" %s |", attrCatEntry.attrName);
  }
  printf("\n");

  while (true) {
    RecId searchRes = BlockAccess::linearSearch(srcRelId, attr, attrVal, op);

    if (searchRes.block != -1 && searchRes.slot != -1) {
        // Create RecBuffer for the block containing the record
        RecBuffer recBuffer(searchRes.block);


        // Allocate space for the record
        Attribute record[relCatEntry.numAttrs];


        // Get the record
        recBuffer.getRecord(
            record,
            searchRes.slot
        );
          // Print the record
        printf("|");

        for (int i = 0; i < relCatEntry.numAttrs; ++i) {

            // Get information about attribute i
            AttrCatEntry attrCatEntry;

            AttrCacheTable::getAttrCatEntry(
                srcRelId,
                i,
                &attrCatEntry
            );


            // Print according to attribute type
            if (attrCatEntry.attrType == NUMBER) {

                printf(" %d |", (int)record[i].nVal);

            }
            else if (attrCatEntry.attrType == STRING) {

                printf(" %s |", record[i].sVal);
            }
        }

        printf("\n");

    } else {

      // (all records over)
      break;
    }
  }

  return SUCCESS;
}

int Algebra::insert(char relName[ATTR_SIZE], int nAttrs, char record[][ATTR_SIZE]){
    // check if the relation is open or relName is RELCAT_RELNAME or ATTRCAT_RELNAME
    if(strcmp(relName, RELCAT_RELNAME) == 0 || strcmp(relName, ATTRCAT_RELNAME) == 0)
        return E_NOTPERMITTED;

    int relId = OpenRelTable::getRelId(relName);
    if(relId == E_RELNOTOPEN)
        return E_RELNOTOPEN;

    // get the relCatEntry for the relation from the cache
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId, &relCatEntry);
    if(relCatEntry.numAttrs != nAttrs)
        return E_NATTRMISMATCH;

    // loop through the attributes and check if the types match with the attrCatEntry in the cache
    union Attribute recordValues[nAttrs];
    for(int i = 0; i < nAttrs; i++){
        // get the attrCatEntry for the ith attribute from the cache
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(relId, i, &attrCatEntry);

        int type = attrCatEntry.attrType;

        if (type == NUMBER){
            if (isNumber(record[i])) 
                recordValues[i].nVal = atof(record[i]);
            else
                return E_ATTRTYPEMISMATCH;
        }
        else if (type == STRING)
        {
            strcpy(recordValues[i].sVal, record[i]);
        }
    }

    // insert the record by calling BlockAccess::insert() function
    int retVal = BlockAccess::insert(relId, recordValues);
    return retVal;
}




