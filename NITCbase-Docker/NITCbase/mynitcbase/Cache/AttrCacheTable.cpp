#include "AttrCacheTable.h"
#include <cstring>

AttrCacheEntry* AttrCacheTable::attrCache[MAX_OPEN];


/* returns the attribute with name `attrName` for the relation corresponding to relId
NOTE: this function expects the caller to allocate memory for `*attrCatBuf`
*/
int AttrCacheTable::getAttrCatEntry(int relId, char attrName[ATTR_SIZE], AttrCatEntry* attrCatBuf) {

  // check that relId is valid and corresponds to an open relation
    if (relId < 0 || relId >= MAX_OPEN) {
    return E_OUTOFBOUND;
  }
  if (attrCache[relId] == nullptr) {
    return E_RELNOTOPEN;
  }
  // iterate over the entries in the attribute cache and set attrCatBuf to the entry that
  //    matches attrName
    // traverse the linked list of attribute cache entries to given name
  for (AttrCacheEntry* entry = attrCache[relId]; entry != nullptr; entry = entry->next) {
    if (!strcmp(entry->attrCatEntry.attrName ,attrName)) {
        *attrCatBuf = entry->attrCatEntry;
      return SUCCESS;
    }     
  }

  // no attribute with name attrName for the relation
  return E_ATTRNOTEXIST;
}

int AttrCacheTable::getAttrCatEntry(int relId, int attrOffset, AttrCatEntry* attrCatBuf) {

  // check that relId is valid and corresponds to an open relation
    if (relId < 0 || relId >= MAX_OPEN) {
    return E_OUTOFBOUND;
  }
  if (attrCache[relId] == nullptr) {
    return E_RELNOTOPEN;
  }
  // iterate over the entries in the attribute cache and set attrCatBuf to the entry that
  //    matches attrName
    // traverse the linked list of attribute cache entries to given name
  for (AttrCacheEntry* entry = attrCache[relId]; entry != nullptr; entry = entry->next) {
    if (entry->attrCatEntry.offset==attrOffset) {
        *attrCatBuf = entry->attrCatEntry;
      return SUCCESS;
    }     
  }

  // no attribute with name attrName for the relation
  return E_ATTRNOTEXIST;
}

//Converts the attribute catlaog record to AttrCAtEntry type
void AttrCacheTable::recordToAttrCatEntry(union Attribute record[ATTRCAT_NO_ATTRS], AttrCatEntry* attrCatEntry) {
  strcpy(attrCatEntry->relName, record[ATTRCAT_REL_NAME_INDEX].sVal);
  strcpy(attrCatEntry->attrName, record[ATTRCAT_ATTR_NAME_INDEX].sVal);
  attrCatEntry->attrType =(int)record[ATTRCAT_ATTR_TYPE_INDEX].nVal;
  attrCatEntry->primaryFlag = record[ATTRCAT_PRIMARY_FLAG_INDEX].nVal;
  attrCatEntry->rootBlock = record[ATTRCAT_ROOT_BLOCK_INDEX].nVal;
  attrCatEntry->offset = record[ATTRCAT_OFFSET_INDEX].nVal;
}
//Gives the value of searchIndex field of the given attribute in the specified relation from Attribute Cache Table. 
//This is used by the B+ Tree search algorithm to find the location of the previous hit so that the search can be resumed from the next leaf index entry.
int AttrCacheTable::getSearchIndex(int relId, char attrName[ATTR_SIZE], IndexId *searchIndex) {
    if (relId < 0 || relId >= MAX_OPEN)
        return E_OUTOFBOUND;

    if (AttrCacheTable::attrCache[relId] == nullptr)
        return E_RELNOTOPEN;

    AttrCacheEntry *curr = AttrCacheTable::attrCache[relId];

    while (curr) {
        if (strcmp(curr->attrCatEntry.attrName, attrName) == 0) {
            *searchIndex = curr->searchIndex;
            return SUCCESS;
        }
        curr = curr->next;
    }

    return E_ATTRNOTEXIST;
}
//Gives the value of searchIndex field of the given attribute in the specified relation from Attribute Cache Table.
// This is used by the B+ Tree search algorithm to find the location of the previous hit so that the search can be resumed from the next leaf index entry.

int AttrCacheTable::getSearchIndex(int relId, int attrOffset, IndexId *searchIndex) {
    if (relId < 0 || relId >= MAX_OPEN)
        return E_OUTOFBOUND;

    if (AttrCacheTable::attrCache[relId] == nullptr)
        return E_RELNOTOPEN;

    AttrCacheEntry *curr = AttrCacheTable::attrCache[relId];

    while (curr) {
        if (curr->attrCatEntry.offset == attrOffset) {
            *searchIndex = curr->searchIndex;
            return SUCCESS;
        }
        curr = curr->next;
    }

    return E_ATTRNOTEXIST;
}

//Sets the value of searchIndex field of the given attribute in the specified relation's Attribute Cache Table entry.
// This is used by the B+ Tree search algorithm to set the location of the previous hit so that the search can be resumed from the next leaf index entry
int AttrCacheTable::setSearchIndex(int relId, char attrName[ATTR_SIZE], IndexId *searchIndex) {
    if (relId < 0 || relId >= MAX_OPEN)
        return E_OUTOFBOUND;

    if (AttrCacheTable::attrCache[relId] == nullptr)
        return E_RELNOTOPEN;

    AttrCacheEntry *curr = AttrCacheTable::attrCache[relId];

    while (curr) {
        if (strcmp(curr->attrCatEntry.attrName, attrName) == 0) {
            curr->searchIndex = *searchIndex;
            return SUCCESS;
        }
        curr = curr->next;
    }

    return E_ATTRNOTEXIST;
}


int AttrCacheTable::setSearchIndex(int relId, int attrOffset, IndexId *searchIndex) {
    if (relId < 0 || relId >= MAX_OPEN)
        return E_OUTOFBOUND;

    if (AttrCacheTable::attrCache[relId] == nullptr)
        return E_RELNOTOPEN;

    AttrCacheEntry *curr = AttrCacheTable::attrCache[relId];

    while (curr) {
        if (curr->attrCatEntry.offset == attrOffset) {
            curr->searchIndex = *searchIndex;
            return SUCCESS;
        }
        curr = curr->next;
    }

    return E_ATTRNOTEXIST;
}

//Resets the value of searchIndex field of the given attribute in the specified relation from Attribute Cache Table to {-1, -1}.
// This is used so that the B+ tree search can be restarted from the root.
int AttrCacheTable::resetSearchIndex(int relId, char attrName[ATTR_SIZE]) {
    IndexId indexId = {-1, -1};
    return AttrCacheTable::setSearchIndex(relId, attrName, &indexId);
}


int AttrCacheTable::resetSearchIndex(int relId, int attrOffset) {
    IndexId indexId = {-1, -1};
    return AttrCacheTable::setSearchIndex(relId, attrOffset, &indexId);
}