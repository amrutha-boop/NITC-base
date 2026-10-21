#include "Algebra.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>

// will return if a string can be parsed as a floating point number
bool isNumber(char *str) {
    int len;
    float ignore;
    int ret = sscanf(str, "%f %n", &ignore, &len);
    return ret == 1 && len == strlen(str);
}

int Algebra::select(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE],
                    char attr[ATTR_SIZE], int op, char strVal[ATTR_SIZE]) {

    // Get source relation ID.
    int srcRelid = OpenRelTable::getRelId(srcRel);
    if (srcRelid == E_RELNOTOPEN)
        return E_RELNOTOPEN;

    // Get condition attribute details.
    AttrCatEntry attrCatEntry;
    if (AttrCacheTable::getAttrCatEntry(srcRelid, attr, &attrCatEntry) != SUCCESS)
        return E_ATTRNOTEXIST;

    // Convert condition value to the correct type.
    Attribute attrVal;
    int type = attrCatEntry.attrType;

    if (type == NUMBER) {
        if (isNumber(strVal))
            attrVal.nVal = atof(strVal);
        else
            return E_ATTRTYPEMISMATCH;
    }
    else if (type == STRING) {
        strcpy(attrVal.sVal, strVal);
    }

    // Get source relation details.
    RelCatEntry srcRelCatEntry;
    RelCacheTable::getRelCatEntry(srcRelid, &srcRelCatEntry);

    int src_nAttrs = srcRelCatEntry.numAttrs;

    // Get source relation attributes.
    char attr_names[src_nAttrs][ATTR_SIZE];
    int attr_types[src_nAttrs];

    for (int i = 0; i < src_nAttrs; i++) {
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(srcRelid, i, &attrCatEntry);

        strcpy(attr_names[i], attrCatEntry.attrName);
        attr_types[i] = attrCatEntry.attrType;
    }

    // Create target relation.
    int ret = Schema::createRel(targetRel, src_nAttrs, attr_names, attr_types);
    if (ret != SUCCESS)
        return ret;

    // Open target relation.
    int targetRelId = OpenRelTable::openRel(targetRel);

    if (targetRelId < 0 || targetRelId >= MAX_OPEN) {
        Schema::deleteRel(targetRel);
        return targetRelId;
    }

    // Reset search to start from the first record.
    RelCacheTable::resetSearchIndex(srcRelid);

    Attribute record[src_nAttrs];

    // Find and insert all matching records.
    while (BlockAccess::search(srcRelid, record, attr, attrVal, op) == SUCCESS) {

        ret = BlockAccess::insert(targetRelId, record);

        if (ret != SUCCESS) {
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return ret;
        }
    }

    // Close the target relation.
    Schema::closeRel(targetRel);

    return SUCCESS;
}

int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE]) {

    // Get source relation ID.
    int srcRelId = OpenRelTable::getRelId(srcRel);

    // Check if source relation is open.
    if (srcRelId == E_RELNOTOPEN)
        return E_RELNOTOPEN;

    // Get source relation details.
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);

    int numAttrs = relCatEntry.numAttrs;

    // Store source attribute names and types.
    char attrNames[numAttrs][ATTR_SIZE];
    int attrTypes[numAttrs];

    // Get details of every source attribute.
    for (int i = 0; i < numAttrs; i++) {
        AttrCatEntry attrCatEntry;

        AttrCacheTable::getAttrCatEntry(
            srcRelId, i, &attrCatEntry
        );

        strcpy(attrNames[i], attrCatEntry.attrName);
        attrTypes[i] = attrCatEntry.attrType;
    }


    /*** Creating and opening the target relation ***/

    // Create target relation.
    int ret = Schema::createRel(
        targetRel, numAttrs, attrNames, attrTypes
    );

    // Return error if creation fails.
    if (ret != SUCCESS)
        return ret;

    // Open target relation.
    int targetRelId = OpenRelTable::openRel(targetRel);

    // Delete target relation if opening fails.
    if (targetRelId < 0 || targetRelId >= MAX_OPEN) {
        Schema::deleteRel(targetRel);
        return targetRelId;
    }


    /*** Inserting projected records into the target relation ***/

    // Start scanning source relation from the first record.
    RelCacheTable::resetSearchIndex(srcRelId);

    Attribute record[numAttrs];

    // Fetch and insert every source record.
    while (BlockAccess::project(srcRelId, record) == SUCCESS) {

        ret = BlockAccess::insert(targetRelId, record);

        // Cleanup if insertion fails.
        if (ret != SUCCESS) {
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return ret;
        }
    }

    // Close target relation.
    Schema::closeRel(targetRel);

    return SUCCESS;
}

int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE],
                     int tar_nAttrs, char tar_Attrs[][ATTR_SIZE]) {

    // Get source relation ID.
    int srcRelId = OpenRelTable::getRelId(srcRel);

    // Check if source relation is open.
    if (srcRelId == E_RELNOTOPEN)
        return E_RELNOTOPEN;

    // Get source relation details.
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);

    int src_nAttrs = relCatEntry.numAttrs;

    // Store offsets and types of target attributes.
    int attr_offset[tar_nAttrs];
    int attr_types[tar_nAttrs];


    /*** Checking target attributes and storing their offsets and types ***/

    // Find each target attribute in the source relation.
    for (int i = 0; i < tar_nAttrs; i++) {

        AttrCatEntry attrCatEntry;

        // Get attribute details using its name.
        int ret = AttrCacheTable::getAttrCatEntry(
            srcRelId, tar_Attrs[i], &attrCatEntry
        );

        // Return error if attribute does not exist.
        if (ret != SUCCESS)
            return E_ATTRNOTEXIST;

        attr_offset[i] = attrCatEntry.offset;
        attr_types[i] = attrCatEntry.attrType;
    }


    /*** Creating and opening the target relation ***/

    // Create target relation.
    int ret = Schema::createRel(
        targetRel, tar_nAttrs, tar_Attrs, attr_types
    );

    // Return error if creation fails.
    if (ret != SUCCESS)
        return ret;

    // Open target relation.
    int targetRelId = OpenRelTable::openRel(targetRel);

    // Delete target relation if opening fails.
    if (targetRelId < 0 || targetRelId >= MAX_OPEN) {
        Schema::deleteRel(targetRel);
        return targetRelId;
    }


    /*** Inserting projected records into the target relation ***/

    // Start scanning source relation from the first record.
    RelCacheTable::resetSearchIndex(srcRelId);

    Attribute record[src_nAttrs];

    // Fetch every source record.
    while (BlockAccess::project(srcRelId, record) == SUCCESS) {

        Attribute proj_record[tar_nAttrs];

        // Copy selected attributes into projected record.
        for (int i = 0; i < tar_nAttrs; i++) {
            proj_record[i] = record[attr_offset[i]];
        }

        // Insert projected record into target relation.
        ret = BlockAccess::insert(targetRelId, proj_record);

        // Cleanup if insertion fails.
        if (ret != SUCCESS) {
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return ret;
        }
    }

    // Close target relation.
    Schema::closeRel(targetRel);

    return SUCCESS;
}

int Algebra::insert(char relName[ATTR_SIZE], int nAttrs, char record[][ATTR_SIZE]){
    // check if the relation is open or relName is RELCAT_RELNAME or ATTRCAT_RELNAME
    if(strcmp(relName, RELCAT_RELNAME) == 0 || strcmp(relName, ATTRCAT_RELNAME) == 0)
        return E_NOTPERMITTED;

    int relId = OpenRelTable::getRelId(relName);
    if(relId == E_RELNOTOPEN)
        return E_RELNOTOPEN;

    // get the relCatEntry for the relation from the cache
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId, &relCatEntry);
    if(relCatEntry.numAttrs != nAttrs)
        return E_NATTRMISMATCH;

    // loop through the attributes and check if the types match with the attrCatEntry in the cache
    union Attribute recordValues[nAttrs];
    for(int i = 0; i < nAttrs; i++){
        // get the attrCatEntry for the ith attribute from the cache
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(relId, i, &attrCatEntry);

        int type = attrCatEntry.attrType;

        if (type == NUMBER){
            if (isNumber(record[i])) 
                recordValues[i].nVal = atof(record[i]);
            else
                return E_ATTRTYPEMISMATCH;
        }
        else if (type == STRING)
        {
            strcpy(recordValues[i].sVal, record[i]);
        }
    }

    // insert the record by calling BlockAccess::insert() function
    int retVal = BlockAccess::insert(relId, recordValues);
    return retVal;
}

