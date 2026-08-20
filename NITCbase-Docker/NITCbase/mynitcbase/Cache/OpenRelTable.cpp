#include "OpenRelTable.h"
#include <cstring>
#include <cstdlib>
#include<bits/stdc++.h>
OpenRelTable::OpenRelTable() {
    // initialize relCache and attrCache with nullptr
    for (int i = 0; i < MAX_OPEN; ++i) {
        RelCacheTable::relCache[i] = nullptr;
        AttrCacheTable::attrCache[i] = nullptr;
    }

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



    /************ Setting up Students in the Cache ************/

    #define STUDENTS_RELID 2
    Attribute studentsRelCatRecord[RELCAT_NO_ATTRS];
    relCatBlock.getRecord(studentsRelCatRecord,2);

    RelCacheEntry studentsRelCacheEntry;

    RelCacheTable::recordToRelCatEntry(studentsRelCatRecord,&studentsRelCacheEntry.relCatEntry);

    studentsRelCacheEntry.recId.block = RELCAT_BLOCK;
    studentsRelCacheEntry.recId.slot = 2;

    RelCacheTable::relCache[STUDENTS_RELID] =
        (RelCacheEntry *)malloc(sizeof(RelCacheEntry));

    *(RelCacheTable::relCache[STUDENTS_RELID]) =
        studentsRelCacheEntry;


    // ---------- Attribute Cache entries for Students ----------

    AttrCacheEntry *head3 = nullptr;
    AttrCacheEntry *tail3 = nullptr;

    // Find the Students attributes in Attribute Catalog.
    // Assuming Students' 4 attributes are stored immediately
    // after the 12 catalog attributes:
    //
    // RELATIONCAT  -> slots 0-5
    // ATTRIBUTECAT -> slots 6-11
    // Students     -> slots 12-15

    for (int i = 12; i < 16; i++) {
        Attribute studentsAttrRecord[ATTRCAT_NO_ATTRS];

        attrCatBlock2.getRecord(studentsAttrRecord,i);

        AttrCacheEntry studentsAttrCacheEntry;
        AttrCacheTable::recordToAttrCatEntry(studentsAttrRecord, &studentsAttrCacheEntry.attrCatEntry);

        studentsAttrCacheEntry.recId.block = ATTRCAT_BLOCK;
        studentsAttrCacheEntry.recId.slot = i;

        AttrCacheEntry *newEntry =(AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));

        *newEntry = studentsAttrCacheEntry;
        newEntry->next = nullptr;

        if (head3 == nullptr) {
            head3 = newEntry;
            tail3 = newEntry;
        }
        else {
            tail3->next = newEntry;
            tail3 = newEntry;
        }
    }

    AttrCacheTable::attrCache[STUDENTS_RELID] = head3;
}
int OpenRelTable::getRelId(char relName[ATTR_SIZE]) {

    if (strcmp(relName, RELCAT_RELNAME) == 0) {
        return RELCAT_RELID;
    }

    if (strcmp(relName, ATTRCAT_RELNAME) == 0) {
        return ATTRCAT_RELID;
    }
    if (strcmp(relName, "Students") == 0) {
        return 2;
    } 
    return E_RELNOTOPEN;
}
OpenRelTable::~OpenRelTable() {
  // free all the memory that you allocated in the constructor
    for (int i = 0; i < MAX_OPEN; ++i) {
        if (RelCacheTable::relCache[i] != nullptr) {
            free(RelCacheTable::relCache[i]);
            RelCacheTable::relCache[i] = nullptr;
        }
    
        AttrCacheEntry* current = AttrCacheTable::attrCache[i];
        while (current != nullptr) {
            AttrCacheEntry* next = current->next;
            free(current);
            current = next;
        }
        AttrCacheTable::attrCache[i] = nullptr;
    }
}