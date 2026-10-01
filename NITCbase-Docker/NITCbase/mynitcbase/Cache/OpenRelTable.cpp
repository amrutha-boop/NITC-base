#include "OpenRelTable.h"
#include <cstring>
#include <cstdlib>
#include<bits/stdc++.h>


    
OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN];

OpenRelTable::OpenRelTable() {

  // initialise all values in relCache and attrCache to be nullptr and all entries
  // in tableMetaInfo to be free

  for(int i=0;i<MAX_OPEN;i++){
   tableMetaInfo[i].free=true;
   RelCacheTable::relCache[i]=nullptr;
   AttrCacheTable::attrCache[i]=nullptr; 
  }

  // load the relation and attribute catalog into the relation cache (we did this already)


    /************ Setting up Relation Cache entries ************/

    /**** setting up Relation Catalog relation in the Relation Cache Table****/
    RecBuffer relCatBlock(RELCAT_BLOCK);
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_RELCAT);
    struct RelCacheEntry relCacheEntry;                 //Reused for adding attribute catalog's relational catalog entries also
    RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
    relCacheEntry.recId.block = RELCAT_BLOCK;
    relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_RELCAT;
    //RelCacheTable::relCache[RELCAT_RELID] is a pointer to a RelCacheEntry struct. We need to allocate memory for it and then copy the relCacheEntry into it.
    RelCacheTable::relCache[RELCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[RELCAT_RELID]) = relCacheEntry;

    /**** setting up Attribute Catalog relation in the Relation Cache Table ****/
    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
    relCatBlock.getRecord(attrCatRecord, RELCAT_SLOTNUM_FOR_ATTRCAT);
    RelCacheTable::recordToRelCatEntry(attrCatRecord, &relCacheEntry.relCatEntry);
    relCacheEntry.recId.block = RELCAT_BLOCK;
    relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT;
    RelCacheTable::relCache[ATTRCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[ATTRCAT_RELID]) = relCacheEntry;
 
      // load the relation and attribute catalog into the attribute cache (we did this already)

    /************ Setting up Attribute cache entries ************/

    /**** setting up Relation Catalog relation in the Attribute Cache Table ****/
    RecBuffer attrCatBlock2(ATTRCAT_BLOCK);
    Attribute attrCatRecord2[ATTRCAT_NO_ATTRS];
    AttrCacheEntry *head = nullptr;
    AttrCacheEntry *tail = nullptr;

    // Loop through the 6 attributes of the relation catalog
    for (int i = 0; i < RELCAT_NO_ATTRS; i++) {
        // Get the attribute record from the attribute catalog and convert it to an attrCatEntry.
        attrCatBlock2.getRecord(attrCatRecord2, i);
        AttrCacheEntry attrCacheEntry;
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord2, &attrCacheEntry.attrCatEntry);
        attrCacheEntry.recId.block = ATTRCAT_BLOCK;
        attrCacheEntry.recId.slot = i;

        // Allocate memory for the attrCacheEntry and add to the linked list.
        AttrCacheEntry* newEntry = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
        *newEntry = attrCacheEntry;
        newEntry->next = nullptr;

        if (head == nullptr) {
            head = newEntry;
            tail = newEntry;
        } else {
            tail->next = newEntry;
            tail = newEntry;
        }
    }
    // Initialize the head of the linked list for relation catalog attributes
    AttrCacheTable::attrCache[RELCAT_RELID] = head;

    /**** setting up Attribute Catalog relation in the Attribute Cache Table ****/
    AttrCacheEntry *head2 = nullptr;
    AttrCacheEntry *tail2 = nullptr;
    // Loop through the 6 attributes of the attribute catalog
    for (int i = RELCAT_NO_ATTRS; i < RELCAT_NO_ATTRS + ATTRCAT_NO_ATTRS; i++) {
        // Reuse the same buffer object instead of redeclaring it.
        attrCatBlock2.getRecord(attrCatRecord2, i);
        AttrCacheEntry attrCacheEntry;
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord2, &attrCacheEntry.attrCatEntry);
        attrCacheEntry.recId.block = ATTRCAT_BLOCK;
        attrCacheEntry.recId.slot = i;

        // Allocate memory for the attrCacheEntry and add to the linked list.
        AttrCacheEntry* newEntry = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
        *newEntry = attrCacheEntry;
        newEntry->next = nullptr;

        if (head2 == nullptr) {
            head2 = newEntry;
            tail2 = newEntry;
        } else {
            tail2->next = newEntry;
            tail2 = newEntry;
        }
    }
    // Initialize the head of the linked list for attribute catalog attributes
    AttrCacheTable::attrCache[ATTRCAT_RELID] = head2;

  /************ Setting up tableMetaInfo entries ************/

  // in the tableMetaInfo array
  //   set free = false for RELCAT_RELID and ATTRCAT_RELID
  tableMetaInfo[RELCAT_RELID].free=false;
  tableMetaInfo[ATTRCAT_RELID].free=false;
    //   set relname for RELCAT_RELID and ATTRCAT_RELID
  strcpy(tableMetaInfo[RELCAT_RELID].relName,RELCAT_RELNAME);
  strcpy(tableMetaInfo[ATTRCAT_RELID].relName,ATTRCAT_RELNAME);


}

int OpenRelTable::getFreeOpenRelTableEntry() {

  /* traverse through the tableMetaInfo array,
    find a free entry in the Open Relation Table.*/
    for(int i=0;i<MAX_OPEN;i++){
        if(tableMetaInfo[i].free==true)
        return i;
    }

    return E_CACHEFULL;
  // if found return the relation id, else return E_CACHEFULL.
}



int OpenRelTable::getRelId( char relName[ATTR_SIZE]) {

  /* traverse through the tableMetaInfo array,
    find the entry in the Open Relation Table corresponding to relName.*/
        for(int i=0;i<MAX_OPEN;i++){
        if(tableMetaInfo[i].free==false && !strcmp(tableMetaInfo[i].relName,relName))
        return i;
    }

  // if found return the relation id, else indicate that the relation do not
  // have an entry in the Open Relation Table.
  return E_RELNOTOPEN;
}

int OpenRelTable::openRel(char relName[ATTR_SIZE]) {
    /* the relation `relName` already has an entry in the Open Relation Table */
   int ret= OpenRelTable::getRelId(relName);
  if(ret>=0){
    return ret;
  }

  /* find a free slot in the Open Relation Table*/
    int relId=OpenRelTable::getFreeOpenRelTableEntry();

  if (relId==E_CACHEFULL){
    return E_CACHEFULL;
  }

  // let relId be used to store the free slot.
 

  /****** Setting up Relation Cache entry for the relation ******/

  /* search for the entry with relation name, relName, in the Relation Catalog 
     
      Care should be taken to reset the searchIndex of the relation RELCAT_RELID
      before calling linearSearch().*/
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute relNameAttr;
    strcpy(relNameAttr.sVal, relName);
    char relNameAttrName[ATTR_SIZE];
    strcpy(relNameAttrName, "RelName");
    RecId relcatRecId =BlockAccess::linearSearch(RELCAT_RELID,relNameAttrName,relNameAttr,EQ);

  // relcatRecId stores the rec-id of the relation `relName` in the Relation Catalog.

  if (relcatRecId.block == -1 && relcatRecId.slot== -1 ) {
    // (the relation is not found in the Relation Catalog.)
    return E_RELNOTEXIST;
  }

  /* read the record entry corresponding to relcatRecId 
   // Read the relation catalog record*/
  RecBuffer relCatBlock(relcatRecId.block);

  Attribute relCatRecord[RELCAT_NO_ATTRS];

  relCatBlock.getRecord(relCatRecord,relcatRecId.slot);
   // Create relation cache entry
  RelCacheEntry relCacheEntry;

  RelCacheTable::recordToRelCatEntry(relCatRecord,&relCacheEntry.relCatEntry);
   
     /* update the recId field of this Relation Cache entry to relcatRecId.*/
    relCacheEntry.recId=relcatRecId;
    /* use the Relation Cache entry to set the relId-th entry of the RelCacheTable.
    NOTE: make sure to allocate memory for the RelCacheEntry using malloc()
  */
  // Allocate memory
  RelCacheTable::relCache[relId] =(RelCacheEntry*)malloc(sizeof(RelCacheEntry));

  *(RelCacheTable::relCache[relId]) = relCacheEntry;

  /****** Setting up Attribute Cache entry for the relation ******/

  // let listHead be used to hold the head of the linked list of attrCache entries.
  AttrCacheEntry* listHead = nullptr;
  AttrCacheEntry* listTail = nullptr;

  /*iterate over all the entries in the Attribute Catalog corresponding to each
  attribute of the relation relName by multiple calls of BlockAccess::linearSearch()
  care should be taken to reset the searchIndex of the relation, ATTRCAT_RELID,
  corresponding to Attribute Catalog before the first call to linearSearch().*/
  {
      /* let attrcatRecId store a valid record id an entry of the relation, relName,
      in the Attribute Catalog.*/
     // RecId attrcatRecId;

      /* read the record entry corresponding to attrcatRecId and create an
      Attribute Cache entry on it using RecBuffer::getRecord() and
      AttrCacheTable::recordToAttrCatEntry().
      update the recId field of this Attribute Cache entry to attrcatRecId.
      add the Attribute Cache entry to the linked list of listHead .*/
      // NOTE: make sure to allocate memory for the AttrCacheEntry using malloc()
  }

  // set the relIdth entry of the AttrCacheTable to listHead.
    // Reset search index of Attribute Catalog
  RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

  // Search all attributes belonging to this relation
        char relNameAttrName2[ATTR_SIZE];
        strcpy(relNameAttrName2, "RelName");
  while (true) {

    Attribute relNameAttr2;
    strcpy(relNameAttr2.sVal, relName);

    RecId attrcatRecId = BlockAccess::linearSearch( ATTRCAT_RELID,relNameAttrName2, relNameAttr2, EQ);

    // no more attributes
    if (attrcatRecId.block == -1 && attrcatRecId.slot == -1) {
      break;
    }

    // Read attribute catalog record
    RecBuffer attrCatBlock(attrcatRecId.block);

    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];

    attrCatBlock.getRecord( attrCatRecord, attrcatRecId.slot );

    // Create attribute cache entry
    AttrCacheEntry attrCacheEntry;

    AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &attrCacheEntry.attrCatEntry);

    attrCacheEntry.recId = attrcatRecId;
    attrCacheEntry.next = nullptr;

    // Allocate memory
    AttrCacheEntry* newEntry =(AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));

    *newEntry = attrCacheEntry;

    // Add to linked list
    if (listHead == nullptr) {
      listHead = newEntry;
      listTail = newEntry;
    }
    else {
      listTail->next = newEntry;
      listTail = newEntry;
    }
  }

  // Store linked list in attribute cache
  AttrCacheTable::attrCache[relId] = listHead;


  /****** Setting up metadata in the Open Relation Table for the relation******/

  // update the relIdth entry of the tableMetaInfo with free as false and
  // relName as the input.
   tableMetaInfo[relId].free = false;

  strcpy( tableMetaInfo[relId].relName, relName);

  return relId;

}

int OpenRelTable::closeRel(int relId) {
    if (relId == RELCAT_RELID || relId == ATTRCAT_RELID) 
        return E_NOTPERMITTED;

    if (relId < 0 || relId >= MAX_OPEN) 
        return E_OUTOFBOUND;

    if (tableMetaInfo[relId].free)
        return E_RELNOTOPEN;

    // If Relation Cache entry was modified, write it back
    if (RelCacheTable::relCache[relId]->dirty) {
        RelCatEntry relCatEntry = RelCacheTable::relCache[relId]->relCatEntry;
        union Attribute record[RELCAT_NO_ATTRS];
        RelCacheTable::relCatEntryToRecord(&relCatEntry,record);
        RecId recId = RelCacheTable::relCache[relId]->recId;
        RecBuffer relCatBlock(recId.block);
        int ret = relCatBlock.setRecord(record, recId.slot);
        if (ret != SUCCESS)
            return ret;
    }    
    // free the memory allocated in the relation cache
    free(RelCacheTable::relCache[relId]);
    RelCacheTable::relCache[relId] = nullptr;

    // free the memory allocated in the attribute cache
    AttrCacheEntry *current = AttrCacheTable::attrCache[relId];
    while (current != nullptr) {
        AttrCacheEntry *next = current->next;
        free(current);
        current = next;
    }
    AttrCacheTable::attrCache[relId] = nullptr;

    // mark the entry in the open relation table as free
    tableMetaInfo[relId].free = true;
    return SUCCESS;
}


OpenRelTable::~OpenRelTable() {

  // close all open relations (from rel-id = 2 onwards. Why?) 
  for (int i = 2; i < MAX_OPEN; ++i) {
    if (!tableMetaInfo[i].free) {
      OpenRelTable::closeRel(i); // we will implement this function later
    }
  }

  // free the memory allocated for rel-id 0 and 1 in the caches
  //relation caches
    if(RelCacheTable::relCache[RELCAT_RELID]!=nullptr){
        free(RelCacheTable::relCache[RELCAT_RELID]);
        RelCacheTable::relCache[RELCAT_RELID]=nullptr;
    } 
    if(RelCacheTable::relCache[ATTRCAT_RELID]!=nullptr){
        free(RelCacheTable::relCache[ATTRCAT_RELID]);
        RelCacheTable::relCache[ATTRCAT_RELID]=nullptr;
    }
///aatribute caches
    for(int i=0;i<MAX_OPEN;i++){
        AttrCacheEntry *current=AttrCacheTable::attrCache[i];

        while(current!=nullptr){
            AttrCacheEntry *next=current->next;
            free(current);
            current=next;
        }
        AttrCacheTable::attrCache[i]=nullptr;
    }

}