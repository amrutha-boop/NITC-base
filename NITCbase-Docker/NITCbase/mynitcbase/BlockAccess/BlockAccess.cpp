#include "BlockAccess.h"

#include <cstring>
#include<bits/stdc++.h>

//This method searches the relation specified linearly
// to find the next record that satisfies the specified condition
RecId BlockAccess::linearSearch( int relId,
    char attrName[ATTR_SIZE],
    union Attribute attrVal,
    int op) {
    RecId prevRecId;
    RelCacheTable::getSearchIndex(relId, &prevRecId);

    int block;
    int slot;

    // Get relation information EVERY time
    RelCatEntry relCatBuf;
    RelCacheTable::getRelCatEntry(relId, &relCatBuf);

    if (prevRecId.block == -1 && prevRecId.slot == -1) {

        block = relCatBuf.firstBlk;
        slot = 0;

    } else {

        block = prevRecId.block;
        slot = prevRecId.slot + 1;
    }

    while (block != -1) {

        RecBuffer obj(block);

        HeadInfo head;
        obj.getHeader(&head);

        int slotCount = head.numSlots;

        unsigned char slotmap[slotCount];
        obj.getSlotMap(slotmap);

        if (slot >= slotCount) {
            block = head.rblock;
            slot = 0;
            continue;
        }
       // printf("%d ", slotmap[slot]);
        if (slotmap[slot] ==SLOT_UNOCCUPIED) {

            slot++;
            continue;
        }

        Attribute rec[relCatBuf.numAttrs];

        obj.getRecord(rec, slot);

        AttrCatEntry attrCatBuf;

        AttrCacheTable::getAttrCatEntry(
            relId,
            attrName,
            &attrCatBuf
        );

        int offset = attrCatBuf.offset;

        int cmpVal = compareAttrs(
            rec[offset],
            attrVal,
            attrCatBuf.attrType
        );

        if (
            (op == NE && cmpVal != 0) ||
            (op == LT && cmpVal < 0) ||
            (op == LE && cmpVal <= 0) ||
            (op == EQ && cmpVal == 0) ||
            (op == GT && cmpVal > 0) ||
            (op == GE && cmpVal >= 0)
        ) {

            RecId searchIndex = {block, slot};

            RelCacheTable::setSearchIndex(
                relId,
                &searchIndex
            );

            return searchIndex;
        }

        slot++;
    }

    return RecId{-1, -1};
}

int BlockAccess::renameRelation(char oldName[ATTR_SIZE], char newName[ATTR_SIZE]){
    // Search the relation catalog for an entry with "RelName" = newName
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute newRelationName;
    strcpy(newRelationName.sVal, newName);
    RecId new_recId = linearSearch(RELCAT_RELID, RELCAT_ATTR_RELNAME, newRelationName, EQ);
    if(new_recId.block != -1 && new_recId.slot != -1){
        return E_RELEXIST;
    }

    // Search the relation catalog for an entry with "RelName" = oldName
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute oldRelationName;
    strcpy(oldRelationName.sVal, oldName);
    RecId old_recId = linearSearch(RELCAT_RELID, RELCAT_ATTR_RELNAME, oldRelationName, EQ);
    if(old_recId.block == -1 && old_recId.slot == -1){
        return E_RELNOTEXIST;
    }

    // Update the relation name in the relation catalog entry with "RelName" = oldName to newName and write back the updated record to the relation catalog
    RecBuffer recBuffer(RELCAT_BLOCK);
    Attribute record[RELCAT_NO_ATTRS];
    recBuffer.getRecord(record, old_recId.slot);
    record[RELCAT_REL_NAME_INDEX] = newRelationName; 
    int numAttrs = record[RELCAT_NO_ATTRIBUTES_INDEX].nVal;
    recBuffer.setRecord(record, old_recId.slot);

    // Update the relation name in the attribute catalog entries with "RelName" = oldName to newName and write back the updated records to the attribute catalog
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    RecId curr;
    Attribute attrRecord[ATTRCAT_NO_ATTRS];
    for(int i = 0; i < numAttrs; i++){
        curr = linearSearch(ATTRCAT_RELID, ATTRCAT_ATTR_RELNAME, oldRelationName, EQ);
        RecBuffer attrBuffer(curr.block);
        attrBuffer.getRecord(attrRecord, curr.slot);
        attrRecord[ATTRCAT_REL_NAME_INDEX] = newRelationName;
        attrBuffer.setRecord(attrRecord, curr.slot);
    }
    return SUCCESS;
}

int BlockAccess::renameAttribute(char relName[ATTR_SIZE], char oldName[ATTR_SIZE], char newName[ATTR_SIZE]) {
    // Check if the relation exists in the relation catalog
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute relNameAttr;
    strcpy(relNameAttr.sVal, relName);
    RecId relId = linearSearch(RELCAT_RELID, RELCAT_ATTR_RELNAME, relNameAttr, EQ);
    if(relId.block == -1 && relId.slot == -1)
        return E_RELNOTEXIST;

    // Check if the attribute with newName already exists in the attribute catalog for the given relation
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    RecId attrToRenameRecId{-1, -1};
    Attribute attrCatEntryRecord[ATTRCAT_NO_ATTRS];
    while (true) {
        RecId curr = linearSearch(ATTRCAT_RELID, ATTRCAT_ATTR_RELNAME, relNameAttr, EQ);
        if(curr.block == -1 && curr.slot == -1)
            break;
        
        // Get the record for the current attribute catalog entry
        RecBuffer recAttrBuffer(curr.block);
        Attribute record[ATTRCAT_NO_ATTRS];
        recAttrBuffer.getRecord(record, curr.slot);
        if(strcmp(record[ATTRCAT_ATTR_NAME_INDEX].sVal, oldName) == 0)
            attrToRenameRecId = curr;
        if(strcmp(record[ATTRCAT_ATTR_NAME_INDEX].sVal, newName) == 0)
            return E_ATTREXIST;
    }

    if(attrToRenameRecId.block == -1 && attrToRenameRecId.slot == -1)
        return E_ATTRNOTEXIST;


    // Update the attribute name in the attribute catalog entry with "AttrName" = oldName to newName and write back the updated record to the attribute catalog
    RecBuffer recAttrBuffer(attrToRenameRecId.block);
    recAttrBuffer.getRecord(attrCatEntryRecord, attrToRenameRecId.slot);
    strcpy(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName);
    recAttrBuffer.setRecord(attrCatEntryRecord, attrToRenameRecId.slot);

    return SUCCESS;
}

int BlockAccess::insert(int relId, Attribute *record) {
    // Get relation catalog entry.
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId, &relCatEntry);
    int blockNum = relCatEntry.firstBlk;

    // Insert position in the relation.
    RecId rec_id = {-1, -1};
    int numOfSlots = relCatEntry.numSlotsPerBlk;
    int numOfAttributes = relCatEntry.numAttrs;
    int prevBlockNum = -1;

    // Find an empty slot in existing blocks.
    while (blockNum != -1) {
        RecBuffer recBuffer(blockNum);
        HeadInfo header;
        recBuffer.getHeader(&header);
        unsigned char slotMap[header.numSlots];
        recBuffer.getSlotMap(slotMap);

        for (int i = 0; i < header.numSlots; i++) {
            if (slotMap[i] == SLOT_UNOCCUPIED) {
                rec_id.block = blockNum;
                rec_id.slot = i;
                break;
            }
        }

        if (rec_id.block != -1 && rec_id.slot != -1) {
            break;
        }

        prevBlockNum = blockNum;
        blockNum = header.rblock;
    }

    // No empty slot found; allocate a new block.
    if (rec_id.block == -1 && rec_id.slot == -1) {
        if (relId == RELCAT_RELID)
            return E_MAXRELATIONS;

        RecBuffer recBuffer;
        int ret = recBuffer.getBlockNum();
        if (ret == E_DISKFULL) {
            return E_DISKFULL;
        }

        rec_id.block = ret;
        rec_id.slot = 0;

        HeadInfo newBlockHeader;
        newBlockHeader.blockType = REC;
        newBlockHeader.pblock = -1;
        newBlockHeader.lblock = (prevBlockNum == -1) ? -1 : prevBlockNum;
        newBlockHeader.rblock = -1;
        newBlockHeader.numEntries = 0;
        newBlockHeader.numSlots = numOfSlots;
        newBlockHeader.numAttrs = numOfAttributes;
        recBuffer.setHeader(&newBlockHeader);

        unsigned char newSlotMap[numOfSlots];
        for (int i = 0; i < numOfSlots; i++) {
            newSlotMap[i] = SLOT_UNOCCUPIED;
        }
        recBuffer.setSlotMap(newSlotMap);

        if (prevBlockNum != -1) {
            RecBuffer prevRecBuffer(prevBlockNum);
            HeadInfo prevHeader;
            prevRecBuffer.getHeader(&prevHeader);
            prevHeader.rblock = rec_id.block;
            prevRecBuffer.setHeader(&prevHeader);
        } else {
            relCatEntry.firstBlk = rec_id.block;
            RelCacheTable::setRelCatEntry(relId, &relCatEntry);
        }

        relCatEntry.lastBlk = rec_id.block;
        RelCacheTable::setRelCatEntry(relId, &relCatEntry);
    }

    // Insert record and update metadata.
    RecBuffer recBuffer(rec_id.block);
    recBuffer.setRecord(record, rec_id.slot);

    unsigned char slotMap[numOfSlots];
    recBuffer.getSlotMap(slotMap);
    slotMap[rec_id.slot] = SLOT_OCCUPIED;
    recBuffer.setSlotMap(slotMap);

    HeadInfo header;
    recBuffer.getHeader(&header);
    header.numEntries++;
    recBuffer.setHeader(&header);

    relCatEntry.numRecs++;
    RelCacheTable::setRelCatEntry(relId, &relCatEntry);

    return SUCCESS;
}



