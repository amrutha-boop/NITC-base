#include "BlockAccess.h"

#include <cstring>
#include<bits/stdc++.h>
RecId BlockAccess::linearSearch(
    int relId,
    char attrName[ATTR_SIZE],
    union Attribute attrVal,
    int op
) {
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
