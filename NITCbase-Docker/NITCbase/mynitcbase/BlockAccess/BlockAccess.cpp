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
    // get the relation catalog entry from relation cache
    // ( use RelCacheTable::getRelCatEntry() of Cache Layer)
        RelCatEntry relCatEntry;
        RelCacheTable::getRelCatEntry(relId, &relCatEntry);

    int blockNum = relCatEntry.firstBlk;

    // rec_id will be used to store where the new record will be inserted
    RecId rec_id = {-1, -1};

    int numOfSlots = relCatEntry.numSlotsPerBlk;
    int numOfAttributes = relCatEntry.numAttrs;

    int prevBlockNum = -1;

    /*
        Traversing the linked list of existing record blocks of the relation
        until a free slot is found OR
        until the end of the list is reached
    */

    while (blockNum != -1) {
        // create a RecBuffer object for blockNum (using appropriate constructor!)
        RecBuffer recBuf(blockNum);
        // get header of block(blockNum) using RecBuffer::getHeader() function
        HeadInfo head;
        recBuf.getHeader(&head);
        // get slot map of block(blockNum) using RecBuffer::getSlotMap() function
        unsigned char slotMap[head.numSlots];
        recBuf.getSlotMap(slotMap);
        // search for free slot in the block 'blockNum' and store it's rec-id in rec_id
        // (Free slot can be found by iterating over the slot map of the block)
        for (int i = 0; i < numOfSlots; i++)
        {
            if(slotMap[i] == SLOT_UNOCCUPIED){
                rec_id.block = blockNum;
                rec_id.slot = i;
                break;
            }
        }
        
        /* slot map stores SLOT_UNOCCUPIED if slot is free and
           SLOT_OCCUPIED if slot is occupied) */
        
        /* if a free slot is found, set rec_id and discontinue the traversal
           of the linked list of record blocks (break from the loop) */
        if (rec_id.block != -1 && rec_id.slot != -1)
        {
            break;
        }
        

        /* otherwise, continue to check the next block by updating the
           block numbers as follows:
              update prevBlockNum = blockNum
              update blockNum = header.rblock (next element in the linked
                                               list of record blocks)
        */
       prevBlockNum = blockNum;
       blockNum = head.rblock;
    }

    //  if no free slot is found in existing record blocks (rec_id = {-1, -1})
    if(rec_id.block == -1 && rec_id.slot == -1)
    {
        // if relation is RELCAT, do not allocate any more blocks
        //     return E_MAXRELATIONS;
        if(relId == RELCAT_RELID){
            return E_MAXRELATIONS;
        }

        // Otherwise,
        // get a new record block (using the appropriate RecBuffer constructor!)
        RecBuffer newRecBuffer;
        // get the block number of the newly allocated block
        int ret = newRecBuffer.getBlockNum();
        // (use BlockBuffer::getBlockNum() function)
        // let ret be the return value of getBlockNum() function call
        if (ret == E_DISKFULL) {
            return E_DISKFULL;
        }

        // Assign rec_id.block = new block number(i.e. ret) and rec_id.slot = 0
        rec_id.block = ret;
        rec_id.slot = 0;

        /*
            set the header of the new record block such that it links with
            existing record blocks of the relation
            set the block's header as follows:
            blockType: REC, pblock: -1
            lblock
                  = -1 (if linked list of existing record blocks was empty
                         i.e this is the first insertion into the relation)
                  = prevBlockNum (otherwise),
            rblock: -1, numEntries: 0,
            numSlots: numOfSlots, numAttrs: numOfAttributes
            (use BlockBuffer::setHeader() function)
        */
       HeadInfo newHead;
       newHead.blockType = REC;
       newHead.pblock = -1;
       newHead.lblock = prevBlockNum;
       newHead.rblock = -1;
       newHead.numEntries = 0;
       newHead.numSlots = numOfSlots;
       newHead.numAttrs = numOfAttributes;

       newRecBuffer.setHeader(&newHead);

        /*
            set block's slot map with all slots marked as free
            (i.e. store SLOT_UNOCCUPIED for all the entries)
            (use RecBuffer::setSlotMap() function)
        */
       unsigned char newSlotMap[numOfSlots];
       for (int i = 0; i < numOfSlots; i++)
       {
        newSlotMap[i] = SLOT_UNOCCUPIED;
       }
       newRecBuffer.setSlotMap(newSlotMap);

        // if prevBlockNum != -1
        if(prevBlockNum != -1)
        {
            // create a RecBuffer object for prevBlockNum
            // get the header of the block prevBlockNum and
            // update the rblock field of the header to the new block
            // number i.e. rec_id.block
            // (use BlockBuffer::setHeader() function)
            RecBuffer prevBlkBuffer(prevBlockNum);
            HeadInfo prevBlkHeader;
            prevBlkBuffer.getHeader(&prevBlkHeader);
            prevBlkHeader.rblock = rec_id.block;
            prevBlkBuffer.setHeader(&prevBlkHeader);
        }
        // else
        else
        {
            // update first block field in the relation catalog entry to the
            // new block (using RelCacheTable::setRelCatEntry() function)
            relCatEntry.firstBlk = rec_id.block;
            RelCacheTable::setRelCatEntry(relId, &relCatEntry);
        }

        // update last block field in the relation catalog entry to the
        // new block (using RelCacheTable::setRelCatEntry() function)
        relCatEntry.lastBlk = rec_id.block;
        RelCacheTable::setRelCatEntry(relId, &relCatEntry);
    }

    // create a RecBuffer object for rec_id.block
    // insert the record into rec_id'th slot using RecBuffer.setRecord())
    RecBuffer recBuffer(rec_id.block);
    recBuffer.setRecord(record, rec_id.slot);

    /* update the slot map of the block by marking entry of the slot to
       which record was inserted as occupied) */
    // (ie store SLOT_OCCUPIED in free_slot'th entry of slot map)
    // (use RecBuffer::getSlotMap() and RecBuffer::setSlotMap() functions)
    unsigned char slotMap[numOfSlots];
    recBuffer.getSlotMap(slotMap);
    slotMap[rec_id.slot] = SLOT_OCCUPIED;
    recBuffer.setSlotMap(slotMap);

    // increment the numEntries field in the header of the block to
    // which record was inserted
    // (use BlockBuffer::getHeader() and BlockBuffer::setHeader() functions)
    HeadInfo header;
    recBuffer.getHeader(&header);
    header.numEntries++;
    recBuffer.setHeader(&header);

    // Increment the number of records field in the relation cache entry for
    // the relation. (use RelCacheTable::setRelCatEntry function)
    RelCacheTable::getRelCatEntry(relId, &relCatEntry);
    relCatEntry.numRecs++;
    RelCacheTable::setRelCatEntry(relId, &relCatEntry);

    int flag = SUCCESS;
    for(int i=0; i<numOfAttributes; i++) {
        AttrCatEntry curAttrCatEntry;
        AttrCacheTable::getAttrCatEntry(relId, i, &curAttrCatEntry);

        if(curAttrCatEntry.rootBlock != -1) {
        int retVal = BPlusTree::bPlusInsert(relId, curAttrCatEntry.attrName, record[curAttrCatEntry.offset], rec_id);
        if(retVal == E_DISKFULL)
            flag = E_INDEX_BLOCKS_RELEASED;
        }
    }
        
    return flag;
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


    

int BlockAccess::deleteRelation(char* relName) {
  if(strcmp(relName, RELCAT_RELNAME) == 0 || strcmp(relName, ATTRCAT_RELNAME) == 0)
    return E_NOTPERMITTED;

  Attribute relNameAttr;
  strcpy(relNameAttr.sVal, relName);

  // searches in relation catalog for the corresponding record
  RelCacheTable::resetSearchIndex(RELCAT_RELID);
  RecId recIdInRelCat = BlockAccess::linearSearch(RELCAT_RELID, (char*)RELCAT_ATTR_RELNAME, relNameAttr, EQ);
  if(recIdInRelCat.block == -1 && recIdInRelCat.slot == -1)
    return E_RELNOTEXIST;

  Attribute relCatRecord[RELCAT_NO_ATTRS];
  RecBuffer relCatBuffer(RELCAT_BLOCK);
  relCatBuffer.getRecord(relCatRecord, recIdInRelCat.slot);

  int numAttrs = relCatRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal;
  int numSlotsPerBlock = relCatRecord[RELCAT_NO_SLOTS_PER_BLOCK_INDEX].nVal;

  // deletes all the record blocks belonging to the relation
  int curBlock = relCatRecord[RELCAT_FIRST_BLOCK_INDEX].nVal;
  while(curBlock != -1) {
    RecBuffer delBlockBuffer(curBlock);
    HeadInfo delBlockHead;
    delBlockBuffer.getHeader(&delBlockHead);
    int rBlock = delBlockHead.rblock;
    delBlockBuffer.releaseBlock();
    curBlock = rBlock;
  }


  int numOfAttrsDeleted = 0;
  RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

  // iterate through attribute catalog and delete corresponding records
  while(true) {
    RecId recIdInAttrCat = BlockAccess::linearSearch(ATTRCAT_RELID, (char*)ATTRCAT_ATTR_RELNAME, relNameAttr, EQ);

    if(recIdInAttrCat.block == -1 && recIdInAttrCat.slot == -1)
      break;

    numOfAttrsDeleted += 1;

    HeadInfo curAttrCatHead;
    RecBuffer curAttrCatBuffer(recIdInAttrCat.block);
    curAttrCatBuffer.getHeader(&curAttrCatHead);

    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
    curAttrCatBuffer.getRecord(attrCatRecord, recIdInAttrCat.slot);

    int rootBlock = attrCatRecord[ATTRCAT_ROOT_BLOCK_INDEX].nVal;
    
    // update slotmap of block of attribute catalog
    unsigned char slotMap[numSlotsPerBlock];
    curAttrCatBuffer.getSlotMap(slotMap);
    slotMap[recIdInAttrCat.slot] = SLOT_UNOCCUPIED;
    curAttrCatBuffer.setSlotMap(slotMap);
    
    // update header of block of attribute catalog
    curAttrCatHead.numEntries -= 1;
    curAttrCatBuffer.setHeader(&curAttrCatHead);

    // maintain the linked list
    if(curAttrCatHead.numEntries == 0) {
      if(curAttrCatHead.lblock != -1){
        RecBuffer leftBlock(curAttrCatHead.lblock);
        HeadInfo leftBlockHead;
        leftBlock.getHeader(&leftBlockHead);
        leftBlockHead.rblock = curAttrCatHead.rblock;
        leftBlock.setHeader(&leftBlockHead);
      }
      if(curAttrCatHead.rblock != -1) {
        RecBuffer rightBlock(curAttrCatHead.rblock);
        HeadInfo rightBlockHead;
        rightBlock.getHeader(&rightBlockHead);
        rightBlockHead.lblock = curAttrCatHead.lblock;
        rightBlock.setHeader(&rightBlockHead);
      }
      // if last block of attrcat is changed
      else {
        relCatRecord[RELCAT_LAST_BLOCK_INDEX].nVal = curAttrCatHead.lblock;
      }

      curAttrCatBuffer.releaseBlock();
    }

    if(rootBlock != -1) {
      BPlusTree::bPlusDestroy(rootBlock);
    }
  }

  // update header of relation catalog
  HeadInfo relCatHead;
  relCatBuffer.getHeader(&relCatHead);
  relCatHead.numEntries -= 1;
  relCatBuffer.setHeader(&relCatHead);

  // update slotmap of relation catalog
  unsigned char relCatSlotMap[SLOTMAP_SIZE_RELCAT_ATTRCAT];
  relCatBuffer.getSlotMap(relCatSlotMap);
  relCatSlotMap[recIdInRelCat.slot] = SLOT_UNOCCUPIED;
  relCatBuffer.setSlotMap(relCatSlotMap);

  // update cache entries corresponding to relcat and attrcat
  RelCatEntry relCatEntry;
  RelCacheTable::getRelCatEntry(RELCAT_RELID, &relCatEntry);
  relCatEntry.numRecs -= 1;
  RelCacheTable::setRelCatEntry(RELCAT_RELID, &relCatEntry);

  RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &relCatEntry);
  relCatEntry.numRecs -= numOfAttrsDeleted;
  RelCacheTable::setRelCatEntry(ATTRCAT_RELID, &relCatEntry);

  return SUCCESS;
}