#include "BPlusTree.h"
#include <cstring>

RecId BPlusTree::bPlusSearch(int relId, char attrName[ATTR_SIZE], Attribute attrVal, int op) {
    IndexId searchIndex;

    AttrCacheTable::getSearchIndex(relId, attrName, &searchIndex);

    AttrCatEntry attrCatEntry;
    AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);

    int block, index;

    if (searchIndex.block == -1 && searchIndex.index == -1) {
        // Start at the root for a new search.
        block = attrCatEntry.rootBlock;
        index = 0;

        if (block == -1) {
            return RecId{-1, -1};
        }

    } else {
        // Resume immediately after the previously matched leaf entry.
        block = searchIndex.block;
        index = searchIndex.index + 1;

        IndLeaf leaf(block);

        HeadInfo leafHead;

        leaf.getHeader(&leafHead);

        if (index >= leafHead.numEntries) {
            // Continue with the next leaf when the current leaf is exhausted.
            block = leafHead.rblock;
            index = 0;
            if (block == -1) {
                return RecId{-1, -1};
            }
        }
    }

    while (StaticBuffer::getStaticBlockType(block) == IND_INTERNAL) {
        IndInternal internalBlk(block);

        HeadInfo intHead;
        internalBlk.getHeader(&intHead);
        InternalEntry intEntry;

        if (op == NE || op == LT || op == LE) {
            // These operators require scanning from the leftmost leaf.
            internalBlk.getEntry(&intEntry, 0);
            block = intEntry.lChild;

        } else {
            int i = 0;
            while (i < intHead.numEntries) {
                internalBlk.getEntry(&intEntry, i);
                int cmpVal = compareAttrs(intEntry.attrVal, attrVal, attrCatEntry.attrType);
                if (
                    (op == EQ && cmpVal >= 0) ||
                    (op == GE && cmpVal >= 0) ||
                    (op == GT && cmpVal > 0)
                ) {
                    break;
                }
                i++;
            }

            if (i != intHead.numEntries) {
                block = intEntry.lChild;

            } else {
                i = intHead.numEntries - 1;
                internalBlk.getEntry(&intEntry, i);
                block = intEntry.rChild;
            }
        }
    }

    while (block != -1) {
        IndLeaf leafBlk(block);
        HeadInfo leafHead;

        leafBlk.getHeader(&leafHead);
        Index leafEntry;
        while (index < leafHead.numEntries) {

            leafBlk.getEntry(&leafEntry, index);

            int cmpVal = compareAttrs(leafEntry.attrVal, attrVal, attrCatEntry.attrType);

            if (
                (op == EQ && cmpVal == 0) ||
                (op == LE && cmpVal <= 0) ||
                (op == LT && cmpVal < 0) ||
                (op == GT && cmpVal > 0) ||
                (op == GE && cmpVal >= 0) ||
                (op == NE && cmpVal != 0)
            ) {
                searchIndex.block = block;
                searchIndex.index = index;
                AttrCacheTable::setSearchIndex(relId, attrName, &searchIndex);

                return RecId{leafEntry.block, leafEntry.slot};

            } else if ((op == EQ || op == LE || op == LT) && cmpVal > 0) {
                return RecId {-1, -1};
            }

            ++index;
        }

        if (op != NE) {
            break;
        }

        // Continue through linked leaves for NE searches.
        block = leafHead.rblock;
        index = 0;
    }

    return RecId{-1, -1};
}

int BPlusTree::bPlusDestroy(int rootBlockNum) {
    if (rootBlockNum < 0 || rootBlockNum >= DISK_BLOCKS) {
        return E_OUTOFBOUND;
    }

    int type = StaticBuffer::getStaticBlockType(rootBlockNum);

    if (type == IND_LEAF) {
        // declare an instance of IndLeaf for rootBlockNum using appropriate
        // constructor
         // release the block using BlockBuffer::releaseBlock().
        IndLeaf leafBuffer(rootBlockNum);
        leafBuffer.releaseBlock();

        return SUCCESS;

    } else if (type == IND_INTERNAL) {
        // declare an instance of IndInternal for rootBlockNum using appropriate
        // constructor
        IndInternal intBuffer(rootBlockNum);

        // load the header of the block using BlockBuffer::getHeader().
        HeadInfo curHead;
        intBuffer.getHeader(&curHead);
        int numEntries = curHead.numEntries;

        /*iterate through all the entries of the internalBlk and destroy the lChild
        of the first entry and rChild of all entries using BPlusTree::bPlusDestroy().
        (the rchild of an entry is the same as the lchild of the next entry.
         take care not to delete overlapping children more than once ) */
        for(int i=0; i<numEntries; i++){
            InternalEntry intEntry;
            intBuffer.getEntry(&intEntry, i);

            // delete the left child of 1st entry and right child of all entries
            if(i == 0) BPlusTree::bPlusDestroy(intEntry.lChild);
            BPlusTree::bPlusDestroy(intEntry.rChild);
        }

        // release the block using BlockBuffer::releaseBlock().
        intBuffer.releaseBlock();

        return SUCCESS;

    } else {
        // (block is not an index block.)
        return E_INVALIDBLOCK;
    }
}

int BPlusTree::bPlusCreate(int relId, char attrName[ATTR_SIZE]) {

    if(relId == RELCAT_RELID || relId == ATTRCAT_RELID)
        return E_NOTPERMITTED;


    // get the attribute catalog entry of attribute `attrName`
    // using AttrCacheTable::getAttrCatEntry()
    AttrCatEntry attrCatEntry;
    int attrRet = AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);

    if(attrRet != SUCCESS){
        return attrRet;
    }

    if (attrCatEntry.rootBlock != -1) {
        return SUCCESS;
    }

    /******Creating a new B+ Tree ******/

    // get a free leaf block using constructor 1 to allocate a new block
    IndLeaf rootBlockBuf;

    // (if the block could not be allocated, the appropriate error code
    //  will be stored in the blockNum member field of the object)

    // declare rootBlock to store the blockNumber of the new leaf block
    int rootBlock = rootBlockBuf.getBlockNum();

    // if there is no more disk space for creating an index
    if (rootBlock == E_DISKFULL) {
        return E_DISKFULL;
    }

    attrCatEntry.rootBlock = rootBlock;
    AttrCacheTable::setAttrCatEntry(relId, attrName, &attrCatEntry);

    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId, &relCatEntry);

    int block = relCatEntry.firstBlk;

    /***** Traverse all the blocks in the relation and insert them one
           by one into the B+ Tree *****/
    while (block != -1) {

        // declare a RecBuffer object for `block` (using appropriate constructor)
        RecBuffer curBlockBuffer(block);

        unsigned char slotMap[relCatEntry.numSlotsPerBlk];

        // load the slot map into slotMap using RecBuffer::getSlotMap().
        curBlockBuffer.getSlotMap(slotMap);

        for(int slot=0; slot<relCatEntry.numSlotsPerBlk; slot++)
        {
            if(slotMap[slot] == SLOT_OCCUPIED){
                Attribute record[relCatEntry.numAttrs];
                // load the record corresponding to the slot into `record`
                // using RecBuffer::getRecord().
                curBlockBuffer.getRecord(record,slot);

                int attrOffset = attrCatEntry.offset;
                // declare recId and store the rec-id of this record in it
                // RecId recId{block, slot};
                RecId recId{block, slot};
                Attribute attrVal = record[attrOffset];

                // insert the attribute value corresponding to attrName from the record
                // into the B+ tree using bPlusInsert.
                // (note that bPlusInsert will destroy any existing bplus tree if
                // insert fails i.e when disk is full)
                // retVal = bPlusInsert(relId, attrName, attribute value, recId);
                int ret = BPlusTree::bPlusInsert(relId, attrName, attrVal, recId);

                if(ret == E_DISKFULL){
                    return E_DISKFULL;
                }
            }
        }

        // get the header of the block using BlockBuffer::getHeader()
        HeadInfo curBlockHeader;
        // set block = rblock of current block (from the header)
        curBlockBuffer.getHeader(&curBlockHeader);
        block = curBlockHeader.rblock;
    }

    return SUCCESS;
}

int BPlusTree::bPlusInsert(int relId, char attrName[ATTR_SIZE], Attribute attrVal, RecId recId) {
  AttrCatEntry attrCatEntry;
  int getAttrRes = AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);
  if(getAttrRes != SUCCESS)
    return getAttrRes;
  
  int blockNum = attrCatEntry.rootBlock;
  if(blockNum == -1)
    return E_NOINDEX;
  
  int attrType = attrCatEntry.attrType;
  // find a leaf node to insert
  int leafBlockNum = findLeafToInsert(blockNum, attrVal, attrType);

  // create a leaf entry and call inserttoleaf function
  Index curEntry;
  curEntry.attrVal = attrVal, curEntry.block = recId.block, curEntry.slot = recId.slot;
  int insertRes = insertIntoLeaf(relId, attrName, leafBlockNum, curEntry);

  // if insert fails, destroy the whole bptree
  if(insertRes == E_DISKFULL) {
    bPlusDestroy(blockNum);

    attrCatEntry.rootBlock = -1;
    AttrCacheTable::setAttrCatEntry(relId, attrName, &attrCatEntry);
    return E_DISKFULL;
  }

  return SUCCESS;
}

int BPlusTree::findLeafToInsert(int rootBlock, Attribute attrVal, int attrType) {
  int blockNum = rootBlock;

  // iterates downwards through internal blocks till it reaches leaf node
  while(StaticBuffer::getStaticBlockType(blockNum) != IND_LEAF) {
    IndInternal intBuffer(blockNum);
    HeadInfo intHeader;
    intBuffer.getHeader(&intHeader);

    // finds correct child pointer to move to
    int foundIndex = -1;
    for(int index=0; index<intHeader.numEntries; index++) {
      InternalEntry intEntry;
      intBuffer.getEntry(&intEntry, index);

      // find the first entry whose value >= value to be inserted
      if(compareAttrs(intEntry.attrVal, attrVal, attrType) > 0) {
        foundIndex = index;
        break;
      }
    }

    // if all internal values are smaller than given value, move to righmost child
    if(foundIndex == -1) {
      InternalEntry intEntry;
      intBuffer.getEntry(&intEntry, intHeader.numEntries-1);
      blockNum = intEntry.rChild;
    }
    // else move to left of the found entry
    else {
      InternalEntry intEntry;
      intBuffer.getEntry(&intEntry, foundIndex);
      blockNum = intEntry.lChild;
    }
  }

  return blockNum;
}

int BPlusTree::insertIntoLeaf(int relId, char attrName[ATTR_SIZE], int blockNum, Index indexEntry) {
  AttrCatEntry attrCatEntry;
  AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);
  int attrType = attrCatEntry.attrType;

  IndLeaf leafBuffer(blockNum);
  HeadInfo leafHeader;
  leafBuffer.getHeader(&leafHeader);

  // insert all indexes in the leaf to an array
  Index indices[leafHeader.numEntries + 1];
  for(int i=0; i<leafHeader.numEntries; i++) {
    Index curEntry;
    leafBuffer.getEntry(&curEntry, i);
    indices[i] = curEntry;
  }

  // find the right spot to insert indexEntry such that the array remains sorted
  int arrayInsertIndex;
  for(arrayInsertIndex = leafHeader.numEntries - 1; arrayInsertIndex >= 0; arrayInsertIndex--) {
      if(compareAttrs(indices[arrayInsertIndex].attrVal, indexEntry.attrVal, attrType) > 0) {
          indices[arrayInsertIndex + 1] = indices[arrayInsertIndex];
      } else {
          break;
      }
  }
  arrayInsertIndex += 1;
  indices[arrayInsertIndex] = indexEntry;

  // if leaf was not full before insertion, hence we can directly copy the array to the leaf
  if(leafHeader.numEntries != MAX_KEYS_LEAF) {
    leafHeader.numEntries += 1;
    leafBuffer.setHeader(&leafHeader);

    for(int i=0; i<leafHeader.numEntries; i++)
      leafBuffer.setEntry(&indices[i], i);
    
    return SUCCESS;
  }

  // else if it was already full before insertion
  
  // split the current block and create a new right leaf block
  int newRightLeafNum = splitLeaf(blockNum, indices);
  if(newRightLeafNum == E_DISKFULL)
    return E_DISKFULL;

  InternalEntry entryToParent;
  entryToParent.attrVal = indices[MIDDLE_INDEX_LEAF].attrVal;
  entryToParent.lChild = blockNum;
  entryToParent.rChild = newRightLeafNum;

  int retVal;
  // if current leaf was not the root node
  if(leafHeader.pblock != -1) {
    retVal = insertIntoInternal(relId, attrName, leafHeader.pblock, entryToParent);
  }
  // the leaf was the root node
  else
    retVal = createNewRoot(relId, attrName, entryToParent.attrVal, blockNum, newRightLeafNum);

  if(retVal == E_DISKFULL)
    return E_DISKFULL;

  return SUCCESS;
}

// split a full leaf node and divide the entries between 2 leaf nodes and set suitable headers
int BPlusTree::splitLeaf(int leafBlockNum, Index indices[]) {
  IndLeaf leftLeaf(leafBlockNum);
  IndLeaf rightLeaf;
  
  int leftLeafNum = leftLeaf.getBlockNum();
  int rightLeafNum = rightLeaf.getBlockNum();

  if(rightLeafNum == E_DISKFULL)
    return E_DISKFULL;

  HeadInfo leftLeafHead, rightLeafHead;
  leftLeaf.getHeader(&leftLeafHead);
  rightLeaf.getHeader(&rightLeafHead);

  // set the header of right leaf
  rightLeafHead.numEntries = (MAX_KEYS_LEAF + 1)/2;
  rightLeafHead.pblock = leftLeafHead.pblock;
  rightLeafHead.lblock = leftLeafNum;
  rightLeafHead.rblock = leftLeafHead.rblock;
  rightLeaf.setHeader(&rightLeafHead);

  // set the header of left leaf
  leftLeafHead.numEntries = (MAX_KEYS_LEAF+1)/2;
  leftLeafHead.rblock = rightLeafNum;
  leftLeaf.setHeader(&leftLeafHead);

  // set the entries of left leaf
  for(int i=0; i<32; i++)
    leftLeaf.setEntry(&indices[i], i);
  // set entries of right leaf
  for(int i=32; i<64; i++)
    rightLeaf.setEntry(&indices[i], i-32);

  return rightLeafNum;
}

// inserts to internal node and handles edges case when internal node is fulll
int BPlusTree::insertIntoInternal(int relId, char attrName[ATTR_SIZE], int intBlockNum, InternalEntry intEntry) {
  AttrCatEntry attrCatEntry;
  AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);
  int attrType = attrCatEntry.attrType;

  IndInternal intBuffer(intBlockNum);
  HeadInfo intHeader;
  intBuffer.getHeader(&intHeader);

  // insert all indexes in the internal block to an array
  InternalEntry intEntries[intHeader.numEntries + 1];
  for(int i=0; i<intHeader.numEntries; i++) {
    InternalEntry curEntry;
    intBuffer.getEntry(&curEntry, i);
    intEntries[i] = curEntry;
  }

  // find the right spot to insert intEntry such that the array remains sorted
  int arrayInsertIndex;
  for(arrayInsertIndex = intHeader.numEntries - 1; arrayInsertIndex >= 0; arrayInsertIndex--) {
      if(compareAttrs(intEntries[arrayInsertIndex].attrVal, intEntry.attrVal, attrType) > 0) {
          intEntries[arrayInsertIndex + 1] = intEntries[arrayInsertIndex];
      } else {
          break;
      }
  }
  arrayInsertIndex += 1;
  intEntries[arrayInsertIndex] = intEntry;
  
  // change the lchild and rchild of neighbouring entries
  if(arrayInsertIndex < intHeader.numEntries)
    intEntries[arrayInsertIndex+1].lChild = intEntry.rChild;
  if(arrayInsertIndex > 0)
    intEntries[arrayInsertIndex-1].rChild = intEntry.lChild;

  // if internal node wasnt full before we added the new entry
  if(intHeader.numEntries != MAX_KEYS_INTERNAL) {
    intHeader.numEntries += 1;
    intBuffer.setHeader(&intHeader);

    for(int i=0; i<intHeader.numEntries; i++)
      intBuffer.setEntry(&intEntries[i], i);
    
    return SUCCESS;
  }
  
  // else if it was full already

  // split the internal node to 2
  int newRightInternal = splitInternal(intBlockNum, intEntries);

  // if splitting failed due to disk full, delete the right tree of intentry
  if(newRightInternal == E_DISKFULL) {
    bPlusDestroy(intEntry.rChild);
    return E_DISKFULL;
  }
  
  // create a new entry for the parent
  InternalEntry intEntryToParent;
  intEntryToParent.attrVal = intEntries[MIDDLE_INDEX_INTERNAL].attrVal;
  intEntryToParent.lChild = intBlockNum;
  intEntryToParent.rChild = newRightInternal;

  int insertRes;
  if(intHeader.pblock != -1) 
    insertRes = insertIntoInternal(relId, attrName, intHeader.pblock, intEntryToParent);
  else
    insertRes = createNewRoot(relId, attrName, intEntryToParent.attrVal, intBlockNum, newRightInternal);

  if(insertRes != SUCCESS) 
    return insertRes;
  return SUCCESS;
}

// splits a full internal node and sets the suitable headers
int BPlusTree::splitInternal(int intBlockNum, InternalEntry internalEntries[]) {
  IndInternal rightInternal;
  IndInternal leftInternal(intBlockNum);

  int rightBlockNum = rightInternal.getBlockNum();
  int leftBlockNum = leftInternal.getBlockNum();

  if(rightBlockNum == E_DISKFULL)
    return E_DISKFULL;

  HeadInfo leftHead, rightHead;
  leftInternal.getHeader(&leftHead);
  rightInternal.getHeader(&rightHead);

  // set headers of left and right blocks
  rightHead.numEntries = (MAX_KEYS_INTERNAL)/2;
  rightHead.pblock = leftHead.pblock;
  rightInternal.setHeader(&rightHead);
  
  leftHead.numEntries = (MAX_KEYS_INTERNAL)/2;
  leftInternal.setHeader(&leftHead);

  // set the entries in left and right block
  for(int i=0; i<50; i++) 
    leftInternal.setEntry(&internalEntries[i], i);
  for(int i=51; i<101; i++)
    rightInternal.setEntry(&internalEntries[i], i-51);

  // set the parent of all children of rightblock to "rightblocknum"
  for(int i=51; i<101; i++) {
    if(i == 51) {
      BlockBuffer childBuffer(internalEntries[i].lChild);
      HeadInfo head;
      childBuffer.getHeader(&head);
      head.pblock = rightBlockNum;
      childBuffer.setHeader(&head);
    }
    BlockBuffer childBuffer(internalEntries[i].rChild);
    HeadInfo head;
    childBuffer.getHeader(&head);
    head.pblock = rightBlockNum;
    childBuffer.setHeader(&head);
  }

  return rightBlockNum;
}

// creates a new root
int BPlusTree::createNewRoot(int relId, char attrName[ATTR_SIZE], Attribute attrVal, int lChild, int rChild) {
  AttrCatEntry attrCatEntry;
  AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);

  IndInternal newRootBlock;
  int rootNum = newRootBlock.getBlockNum();

  if(rootNum == E_DISKFULL) {
    bPlusDestroy(rChild);
    return E_DISKFULL;
  }

  // set entries as 1
  HeadInfo rootHead;
  newRootBlock.getHeader(&rootHead);
  rootHead.numEntries = 1;
  newRootBlock.setHeader(&rootHead);

  InternalEntry newEntry;
  newEntry.lChild = lChild, newEntry.rChild = rChild, newEntry.attrVal = attrVal;
  newRootBlock.setEntry(&newEntry, 0);

  // set the parent of lchild and rchild to current new root block
  BlockBuffer lChildBuffer(lChild);
  HeadInfo lChildHead;
  lChildBuffer.getHeader(&lChildHead);
  lChildHead.pblock = rootNum;
  lChildBuffer.setHeader(&lChildHead);
  BlockBuffer rChildBuffer(rChild);
  HeadInfo rChildHead;
  rChildBuffer.getHeader(&rChildHead);
  rChildHead.pblock = rootNum;
  rChildBuffer.setHeader(&rChildHead);

  // changes rootblock value in cache
  attrCatEntry.rootBlock = rootNum;
  AttrCacheTable::setAttrCatEntry(relId, attrName, &attrCatEntry);
  
  return SUCCESS;
}
