#include "BlockAccess.h"
#include <cstring>

int BlockAccess::project(int relId, Attribute *record) {
    // Get the previous search position.
    RecId prevRecId;
    RelCacheTable::getSearchIndex(relId, &prevRecId);

    // Track the current block and slot.
    int block, slot;

    // Start from the first block for a new scan.
    if (prevRecId.block == -1 && prevRecId.slot == -1)
    {
        RelCatEntry relCatEntry;
        RelCacheTable::getRelCatEntry(relId, &relCatEntry);

        block = relCatEntry.firstBlk;
        slot = 0;
    }
    else
    {
        block = prevRecId.block;
        slot = prevRecId.slot + 1;
    }


    // Find the next occupied record.
    while (block != -1)
    {
        // Open the current block.
        RecBuffer recBuffer(block);

        // Read the block header and slot map.
        HeadInfo head;
        recBuffer.getHeader(&head);
        unsigned char slotMap[head.numSlots];
        recBuffer.getSlotMap(slotMap);

        if(slot >= head.numSlots)
        {
            // Move to the next block.
            block = head.rblock;
            slot = 0;
        }
        else if (slotMap[slot] == SLOT_UNOCCUPIED) {
            // Skip unoccupied slots.
            slot++;
        }
        else {
            // Stop at the next occupied slot.
            break;
        }
    }

    if (block == -1){
        // No occupied record remains.
        return E_NOTFOUND;
    }

    // Save the found record position.
    RecId nextRecId{block, slot};

    // Update the search position.
    RelCacheTable::setSearchIndex(relId, &nextRecId);
    // Load the record into the output buffer.
    RecBuffer recBuffer(nextRecId.block);
    recBuffer.getRecord(record, nextRecId.slot);

    return SUCCESS;
}

RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op) {
    RecId prevRecId;
    RelCacheTable::getSearchIndex(relId, &prevRecId);
    int block;
    int slot;
    // Start at the first block for a new search.
    if (prevRecId.block == -1 && prevRecId.slot == -1)
    {
        RelCatEntry relCat;
        RelCacheTable::getRelCatEntry(relId, &relCat);
        block = relCat.firstBlk;
        slot = 0;
    }
    else
    {
        block = prevRecId.block;
        slot = prevRecId.slot + 1;
    }

    // Scan blocks for a matching record.
    while (block != -1)
    {
        // Open the current block.
        RecBuffer recBuffer(block);

        HeadInfo head;
        recBuffer.getHeader(&head);
        unsigned char slotMap[head.numSlots];
        recBuffer.getSlotMap(slotMap);
        // Move to the next block when slots are exhausted.
        if (slot >= head.numSlots) {
            block = head.rblock;
            slot = 0;
            continue;
        }
        // Skip unoccupied slots.
        if (slotMap[slot] == SLOT_UNOCCUPIED) {
            slot++;
            continue;
        }
        Attribute record[head.numAttrs];
        recBuffer.getRecord(record, slot);
        

        // Get the target attribute metadata.
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);
        int offset = attrCatEntry.offset;
        int cmpVal;
        cmpVal = compareAttrs(record[offset], attrVal, attrCatEntry.attrType);
        
        if (
            (op == NE && cmpVal != 0) ||   
            (op == LT && cmpVal < 0) ||    
            (op == LE && cmpVal <= 0) ||    
            (op == EQ && cmpVal == 0) ||   
            (op == GT && cmpVal > 0) ||     
            (op == GE && cmpVal >= 0)      
        ) {
            RecId recId;                    
            recId.block = block;
            recId.slot = slot;
            RelCacheTable::setSearchIndex(relId, &recId);                   // Save the search position.
            return RecId{block, slot};
        }
        slot++;
    }

    // No matching record was found.
    return RecId{-1, -1};
}

int BlockAccess::renameRelation(char oldName[ATTR_SIZE], char newName[ATTR_SIZE]){
    // Reject an existing destination name.
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute newRelationName;
    strcpy(newRelationName.sVal, newName);
    RecId new_recId = linearSearch(RELCAT_RELID, RELCAT_ATTR_RELNAME, newRelationName, EQ);
    if(new_recId.block != -1 && new_recId.slot != -1){
        return E_RELEXIST;
    }

    // Find the source relation.
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute oldRelationName;
    strcpy(oldRelationName.sVal, oldName);
    RecId old_recId = linearSearch(RELCAT_RELID, RELCAT_ATTR_RELNAME, oldRelationName, EQ);
    if(old_recId.block == -1 && old_recId.slot == -1){
        return E_RELNOTEXIST;
    }

    // Rename the relation catalog entry.
    RecBuffer recBuffer(RELCAT_BLOCK);
    Attribute record[RELCAT_NO_ATTRS];
    recBuffer.getRecord(record, old_recId.slot);
    record[RELCAT_REL_NAME_INDEX] = newRelationName; 
    int numAttrs = record[RELCAT_NO_ATTRIBUTES_INDEX].nVal;
    recBuffer.setRecord(record, old_recId.slot);

    // Rename related attribute catalog entries.
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
    // Verify that the relation exists.
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute relNameAttr;
    strcpy(relNameAttr.sVal, relName);
    RecId relId = linearSearch(RELCAT_RELID, RELCAT_ATTR_RELNAME, relNameAttr, EQ);
    if(relId.block == -1 && relId.slot == -1)
        return E_RELNOTEXIST;

    // Check for a duplicate attribute name.
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    RecId attrToRenameRecId{-1, -1};
    Attribute attrCatEntryRecord[ATTRCAT_NO_ATTRS];
    while (true) {
        RecId curr = linearSearch(ATTRCAT_RELID, ATTRCAT_ATTR_RELNAME, relNameAttr, EQ);
        if(curr.block == -1 && curr.slot == -1)
            break;
        
        // Read the current attribute entry.
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


    // Rename the attribute catalog entry.
    RecBuffer recAttrBuffer(attrToRenameRecId.block);
    recAttrBuffer.getRecord(attrCatEntryRecord, attrToRenameRecId.slot);
    strcpy(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName);
    recAttrBuffer.setRecord(attrCatEntryRecord, attrToRenameRecId.slot);

    return SUCCESS;
}

int BlockAccess::insert(int relId, Attribute *record) {
    // Load relation metadata.
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId, &relCatEntry);
    int blockNum = relCatEntry.firstBlk;

    // Initialize the insertion position.
    RecId rec_id = {-1, -1};
    int numOfSlots = relCatEntry.numSlotsPerBlk;
    int numOfAttributes = relCatEntry.numAttrs;
    int prevBlockNum = -1;

    // Find an available slot.
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

    // Allocate a block when no slot is available.
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

    // Insert the record and update metadata.
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

int BlockAccess::search(int relId, Attribute *record, char attrName[ATTR_SIZE], Attribute attrVal, int op) {
    RecId recId;

    // Get the attribute catalog entry from the attribute cache.
    AttrCatEntry attrCatEntry;
    int ret = AttrCacheTable::getAttrCatEntry(
        relId, attrName, &attrCatEntry
    );

    if (ret != SUCCESS)
        return ret;

    // Check whether an index exists for this attribute.
    if (attrCatEntry.rootBlock == -1) {
        // No index: search using linear search.
        recId = linearSearch(relId, attrName, attrVal, op);
    } else {
        // Index exists: search using the B+ tree.
        recId = BPlusTree::bPlusSearch(relId, attrName, attrVal, op);
    }

    // Return an error if no matching record was found.
    if (recId.block == -1 && recId.slot == -1)
        return E_NOTFOUND;

    // Read the matching record from its record block.
    RecBuffer recBuffer(recId.block);
    ret = recBuffer.getRecord(record, recId.slot);

    if (ret != SUCCESS)
        return ret;

    return SUCCESS;
}

int BlockAccess::deleteRelation(char relName[ATTR_SIZE]) {
    // Prevent deletion of system catalogs.
    if (strcmp(relName, RELCAT_RELNAME) == 0 || strcmp(relName, ATTRCAT_RELNAME) == 0)
        return E_NOTPERMITTED;

    // Check if the relation exists
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute relNameAttr;
    strcpy(relNameAttr.sVal, relName);
    RecId relCatRecId = linearSearch(RELCAT_RELID, RELCAT_ATTR_RELNAME, relNameAttr, EQ);
    if (relCatRecId.block == -1 && relCatRecId.slot == -1)
        return E_RELNOTEXIST;

    // Read the relation catalog record.
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    RecBuffer relCatBuffer(relCatRecId.block);
    relCatBuffer.getRecord(relCatRecord, relCatRecId.slot);

    // Get the first data block.
    int currentBlock = relCatRecord[RELCAT_FIRST_BLOCK_INDEX].nVal;

    // Delete all data blocks.
    while (currentBlock != -1) {
        // Load the current block and read its header.
        RecBuffer block(currentBlock);
        HeadInfo head;
        block.getHeader(&head);
        int nextBlock = head.rblock;

        // Release the current block.
        block.releaseBlock();
        currentBlock = nextBlock;
    }

    // Reset ATTRCAT search index.
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    int numberOfAttributesDeleted = 0;

    // Delete all attributes of the relation.
    while (true) {
        // Find the next attribute entry.
        RecId attrCatRecId = linearSearch(ATTRCAT_RELID, ATTRCAT_ATTR_RELNAME, relNameAttr, EQ);

        // Stop when no more entries exist.
        if (attrCatRecId.block == -1 && attrCatRecId.slot == -1)
            break;

        numberOfAttributesDeleted++;

        // Load the attribute catalog block.
        RecBuffer attrCatBuffer(attrCatRecId.block);

        // Read the block header.
        HeadInfo head;
        attrCatBuffer.getHeader(&head);

        // Read the attribute catalog record.
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        attrCatBuffer.getRecord(attrCatRecord, attrCatRecId.slot);

        // Store the index root block.
        int rootBlock = attrCatRecord[ATTRCAT_ROOT_BLOCK_INDEX].nVal;

        // Get the slot map.
        unsigned char slotMap[head.numSlots];
        attrCatBuffer.getSlotMap(slotMap);

        // Mark the slot as unused.
        slotMap[attrCatRecId.slot] = SLOT_UNOCCUPIED;

        // Update the slot map.
        attrCatBuffer.setSlotMap(slotMap);

        // Decrease the number of entries.
        head.numEntries--;

        // Update the block header.
        attrCatBuffer.setHeader(&head);

        // Remove empty blocks from the linked list.
        if (head.numEntries == 0) {

            int leftBlock = head.lblock;
            int rightBlock = head.rblock;

            // Update the left block.
            if (leftBlock != -1) {

                RecBuffer leftBuffer(leftBlock);

                HeadInfo leftHead;
                leftBuffer.getHeader(&leftHead);

                leftHead.rblock = rightBlock;

                leftBuffer.setHeader(&leftHead);
            }

            // Update the right block.
            if (rightBlock != -1) {

                RecBuffer rightBuffer(rightBlock);

                HeadInfo rightHead;
                rightBuffer.getHeader(&rightHead);

                rightHead.lblock = leftBlock;

                rightBuffer.setHeader(&rightHead);
            }
            else {

                // Update ATTRCAT's last block.
                RelCatEntry attrCatEntry;

                RelCacheTable::getRelCatEntry(
                    ATTRCAT_RELID, &attrCatEntry);

                attrCatEntry.lastBlk = leftBlock;

                RelCacheTable::setRelCatEntry(
                    ATTRCAT_RELID, &attrCatEntry);
            }

            // Release the empty block.
            attrCatBuffer.releaseBlock();
        }

        // Delete the index if one exists.
        if (rootBlock != -1) {
            // BPlusTree::bPlusDestroy(rootBlock);
        }
    }

    // Read RELCAT's block header.
    HeadInfo relCatHead;
    relCatBuffer.getHeader(&relCatHead);

    // Decrease the number of RELCAT entries.
    relCatHead.numEntries--;

    // Update RELCAT's block header.
    relCatBuffer.setHeader(&relCatHead);

    // Get RELCAT's slot map.
    unsigned char relCatSlotMap[relCatHead.numSlots];
    relCatBuffer.getSlotMap(relCatSlotMap);

    // Mark the relation slot as unused.
    relCatSlotMap[relCatRecId.slot] = SLOT_UNOCCUPIED;

    // Update RELCAT's slot map.
    relCatBuffer.setSlotMap(relCatSlotMap);

    // Update RELCAT cache metadata.
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(RELCAT_RELID, &relCatEntry);

    relCatEntry.numRecs--;

    RelCacheTable::setRelCatEntry(RELCAT_RELID, &relCatEntry);

    // Update ATTRCAT cache metadata.
    RelCatEntry attrCatEntry;
    RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &attrCatEntry);

    attrCatEntry.numRecs -= numberOfAttributesDeleted;

    RelCacheTable::setRelCatEntry(ATTRCAT_RELID, &attrCatEntry);

    return SUCCESS;
}