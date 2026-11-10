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

    // Reset search to start from the first record 
    RelCacheTable::resetSearchIndex(srcRelid);
    AttrCacheTable::resetSearchIndex(srcRelid, attr);

    Attribute record[src_nAttrs];
    numComparisons=0;
    // Find and insert all matching records.
    while (BlockAccess::search(srcRelid, record, attr, attrVal, op) == SUCCESS) {

        ret = BlockAccess::insert(targetRelId, record);

        if (ret != SUCCESS) {
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return ret;
        }
    }
    printf("Number of Comparisons:%d\n",numComparisons);

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

int Algebra::join(char srcRelation1[ATTR_SIZE], char srcRelation2[ATTR_SIZE], char targetRelation[ATTR_SIZE], char attribute1[ATTR_SIZE], char attribute2[ATTR_SIZE])
{
    // 1. Get relation IDs
    int relId1 = OpenRelTable::getRelId(srcRelation1);
    int relId2 = OpenRelTable::getRelId(srcRelation2);

    if (relId1 < 0 || relId1 >= MAX_OPEN || relId2 < 0 || relId2 >= MAX_OPEN)
        return E_RELNOTOPEN;

    // 2. Get attribute entries
    AttrCatEntry attr1, attr2;
    if (AttrCacheTable::getAttrCatEntry(relId1, attribute1, &attr1) != SUCCESS)
        return E_ATTRNOTEXIST;

    if (AttrCacheTable::getAttrCatEntry(relId2, attribute2, &attr2) != SUCCESS)
        return E_ATTRNOTEXIST;

    // 3. Type check
    if (attr1.attrType != attr2.attrType)
        return E_ATTRTYPEMISMATCH;

    // 4. Get relation metadata
    RelCatEntry rel1, rel2;
    RelCacheTable::getRelCatEntry(relId1, &rel1);
    RelCacheTable::getRelCatEntry(relId2, &rel2);

    int n1 = rel1.numAttrs;
    int n2 = rel2.numAttrs;

    // 5. Duplicate attribute check (excluding join attribute2)
    AttrCatEntry temp1, temp2;
    for (int i = 0; i < n2; i++)
    {
        if (i == attr2.offset) continue;

        AttrCacheTable::getAttrCatEntry(relId2, i, &temp2);

        for (int j = 0; j < n1; j++)
        {
            AttrCacheTable::getAttrCatEntry(relId1, j, &temp1);

            if (strcmp(temp2.attrName, temp1.attrName) == 0)
                return E_DUPLICATEATTR;
        }
    }

    // 6. Create index on relation2 if not present
    if (attr2.rootBlock == -1)
    {
        int ret = BPlusTree::bPlusCreate(relId2, attribute2);
        if (ret != SUCCESS)
            return ret;

        // refresh metadata
        AttrCacheTable::getAttrCatEntry(relId2, attribute2, &attr2);
    }

    // 7. Prepare target schema
    int targetAttrs = n1 + n2 - 1;
    char attrNames[targetAttrs][ATTR_SIZE];
    int attrTypes[targetAttrs];

    // copy rel1 attributes
    for (int i = 0; i < n1; i++)
    {
        AttrCacheTable::getAttrCatEntry(relId1, i, &temp1);
        strcpy(attrNames[i], temp1.attrName);
        attrTypes[i] = temp1.attrType;
    }

    // copy rel2 attributes (skip join attr)
    int idx = n1;
    for (int i = 0; i < n2; i++)
    {
        if (i == attr2.offset) continue;

        AttrCacheTable::getAttrCatEntry(relId2, i, &temp2);
        strcpy(attrNames[idx], temp2.attrName);
        attrTypes[idx] = temp2.attrType;
        idx++;
    }

    // 8. Create target relation
    int ret = Schema::createRel(targetRelation, targetAttrs, attrNames, attrTypes);
    if (ret != SUCCESS)
        return ret;

    int targetRelId = OpenRelTable::openRel(targetRelation);
    if (targetRelId < 0 || targetRelId >= MAX_OPEN)
    {
        Schema::deleteRel(targetRelation);
        return targetRelId;
    }

    // 9. Prepare records
    Attribute rec1[n1], rec2[n2], out[targetAttrs];

    // reset outer relation
    RelCacheTable::resetSearchIndex(relId1);

    // 10. Join logic
    
    while (BlockAccess::project(relId1, rec1) == SUCCESS)
    {
        RelCacheTable::resetSearchIndex(relId2);
        AttrCacheTable::resetSearchIndex(relId2, attribute2);
        
        while (BlockAccess::search(relId2, rec2, attribute2,
                                   rec1[attr1.offset], EQ) == SUCCESS)
        {
            // copy rel1
            for (int i = 0; i < n1; i++)
                out[i] = rec1[i];

            // copy rel2 except join attr
            int k = n1;
            for (int i = 0; i < n2; i++)
            {
                if (i == attr2.offset) continue;
                out[k++] = rec2[i];
            }

            ret = BlockAccess::insert(targetRelId, out);
            if (ret != SUCCESS)
            {
                OpenRelTable::closeRel(targetRelId);
                Schema::deleteRel(targetRelation);
                return ret;
            }
        }
    }

    // 11. Close relation
    OpenRelTable::closeRel(targetRelId);

    return SUCCESS;
}
