#include "Schema.h"

#include <cmath>
#include <cstring>

int Schema::openRel(char relName[ATTR_SIZE]) {
  int ret = OpenRelTable::openRel(relName);

  // the OpenRelTable::openRel() function returns the rel-id if successful
  // a valid rel-id will be within the range 0 <= relId < MAX_OPEN and any
  // error codes will be negative
  if(ret >= 0){
    return SUCCESS;
  }

  //otherwise it returns an error message
  return ret;
}

int Schema::closeRel(char relName[ATTR_SIZE]) {
/* relation is relation catalog or attribute catalog */
  if (!strcmp(relName,RELCAT_RELNAME) || !strcmp(relName,ATTRCAT_RELNAME)) {
    return E_NOTPERMITTED;
  }

  // this function returns the rel-id of a relation if it is open or
  // E_RELNOTOPEN if it is not. we will implement this later.
  int relId = OpenRelTable::getRelId(relName);

  if (relId==E_RELNOTOPEN) {
    return E_RELNOTOPEN;
  }

  return OpenRelTable::closeRel(relId);
}

int Schema::createRel(char relName[],int nAttrs, char attrs[][ATTR_SIZE],int attrtype[]){
    // Store the relation name as an attribute value.
    Attribute relNameAsAttribute;
    strcpy(relNameAsAttribute.sVal, relName);

    // declare a variable targetRelId of type RecId
    RecId targetRelId={-1,-1};

    // Reset the relation catalog search index.
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    // Find an existing relation with the same name.(checking)
    targetRelId = BlockAccess::linearSearch(RELCAT_RELID, RELCAT_ATTR_RELNAME, relNameAsAttribute, EQ);

    // Reject duplicate relation names. that is alredy existing
    if(targetRelId.block != -1 || targetRelId.slot != -1)
      return E_RELEXIST;

    // compare every pair of attributes of attrNames[] array
    // if any attribute names have same string value,
    //     return E_DUPLICATEATTR (i.e 2 attributes have same value)
    //duplicate attribute names
    for(int i = 0; i < nAttrs - 1; i++)
    for(int j = i + 1; j < nAttrs; j++)
        if(strcmp(attrs[i], attrs[j]) == 0)
          return E_DUPLICATEATTR;
  

    /* declare relCatRecord of type Attribute which will be used to store the
       record corresponding to the new relation which will be inserted
       into relation catalog */
        // Build the relation catalog record.
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    strcpy(relCatRecord[RELCAT_REL_NAME_INDEX].sVal, relName);
    relCatRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal = nAttrs;
    relCatRecord[RELCAT_NO_RECORDS_INDEX].nVal = 0;
    relCatRecord[RELCAT_FIRST_BLOCK_INDEX].nVal = -1;
    relCatRecord[RELCAT_LAST_BLOCK_INDEX].nVal = -1;
    relCatRecord[RELCAT_NO_SLOTS_PER_BLOCK_INDEX].nVal = floor((2016 / (16 * nAttrs + 1)));
    // (number of slots is calculated as specified in the physical layer docs)

    // Insert the relation catalog record.
    int retVal = BlockAccess::insert(RELCAT_RELID, relCatRecord);
        if(retVal != SUCCESS)
      return retVal;
       // if BlockAccess::insert fails return retVal
    // (this call could fail if there is no more space in the relation catalog)

    // Insert one attribute catalog record per attribute.
    for(int i = 0; i < nAttrs; i++){
        Attribute attrCatRecord[6];
        strcpy(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, relName);
        strcpy(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, attrs[i]);
        attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal = attrtype[i];
        attrCatRecord[ATTRCAT_PRIMARY_FLAG_INDEX].nVal = -1;
        attrCatRecord[ATTRCAT_ROOT_BLOCK_INDEX].nVal = -1;
        attrCatRecord[ATTRCAT_OFFSET_INDEX].nVal = i;


       // Roll back the relation record if the attribute insert fails.
       retVal = BlockAccess::insert(ATTRCAT_RELID, attrCatRecord);
       if(retVal != SUCCESS){
          Schema::deleteRel(relName);
          //delete the relation by calling deleteRel(targetrel) of schema layer
          return E_DISKFULL;
       }
    }

    return SUCCESS;
}


int Schema::deleteRel(char *relName) {
    // if the relation to delete is either Relation Catalog or Attribute Catalog,
    //     return E_NOTPERMITTED
    if(strcmp(relName, RELCAT_RELNAME) == 0 || strcmp(relName, ATTRCAT_RELNAME) == 0)
      return E_NOTPERMITTED;
   

    // get the rel-id using appropriate method of OpenRelTable class by
    // passing relation name as argument
    int relId = OpenRelTable::getRelId(relName);

    // if relation is opened in open relation table, return E_RELOPEN
        // Reject open relations.
    if(relId != E_RELNOTOPEN)
        return E_RELOPEN;

     int retVal = BlockAccess::deleteRelation(relName);
    return retVal;

    /* the only that should be returned from deleteRelation() is E_RELNOTEXIST.
       The deleteRelation call may return E_OUTOFBOUND from the call to
       loadBlockAndGetBufferPtr, but if your implementation so far has been
       correct, it should not reach that point. That error could only occur
       if the BlockBuffer was initialized with an invalid block number.
    */
}

int Schema:: renameRel(char oldRelName[ATTR_SIZE], char newRelName[ATTR_SIZE]) {
    // if the oldRelName or newRelName is either Relation Catalog or Attribute Catalog,
        // return E_NOTPERMITTED
    if(!strcmp(oldRelName,RELCAT_RELNAME) || !strcmp(oldRelName,ATTRCAT_RELNAME) ||
    !strcmp(newRelName,RELCAT_RELNAME) || !strcmp(newRelName,ATTRCAT_RELNAME))
    return E_NOTPERMITTED;    
        // (check if the relation names are either "RELATIONCAT" and "ATTRIBUTECAT".
        // you may use the following constants: RELCAT_RELNAME and ATTRCAT_RELNAME)

    // if the relation is open
    int ret=OpenRelTable::getRelId(oldRelName);
      if(ret!=E_RELNOTOPEN) return E_RELOPEN; 
    //    (check if OpenRelTable::getRelId() returns E_RELNOTOPEN)
    //    return E_RELOPEN

    int retVal = BlockAccess::renameRelation(oldRelName, newRelName);
    return retVal;
}

int Schema::renameAttr(char *relName, char *oldAttrName, char *newAttrName) {
    // if the relName is either Relation Catalog or Attribute Catalog,
        // return E_NOTPERMITTED
        // (check if the relation names are either "RELATIONCAT" and "ATTRIBUTECAT".
        // you may use the following constants: RELCAT_RELNAME and ATTRCAT_RELNAME)
     if(!strcmp(relName,RELCAT_RELNAME) || !strcmp(relName,ATTRCAT_RELNAME))
    return E_NOTPERMITTED;  
    // if the relation is open
        //    (check if OpenRelTable::getRelId() returns E_RELNOTOPEN)
        //    return E_RELOPEN
      int ret=OpenRelTable::getRelId(relName);
      if(ret!=E_RELNOTOPEN) return E_RELOPEN; 
    // Call BlockAccess::renameAttribute with appropriate arguments.
     int retVal = BlockAccess::renameAttribute(relName,oldAttrName, newAttrName);
    return retVal;
    // return the value returned by the above renameAttribute() call
}

