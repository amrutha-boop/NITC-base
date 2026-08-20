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
  attrCatEntry->attrType = record[ATTRCAT_ATTR_TYPE_INDEX].nVal;
  attrCatEntry->primaryFlag = record[ATTRCAT_PRIMARY_FLAG_INDEX].nVal;
  attrCatEntry->rootBlock = record[ATTRCAT_ROOT_BLOCK_INDEX].nVal;
  attrCatEntry->offset = record[ATTRCAT_OFFSET_INDEX].nVal;
}