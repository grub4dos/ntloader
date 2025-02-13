/* Copyright (c) Mark Harmstone 2020
 *
 * This file is part of Quibble.
 *
 * Quibble is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public Licence as published by
 * the Free Software Foundation, either version 3 of the Licence, or
 * (at your option) any later version.
 *
 * Quibble is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public Licence for more details.
 *
 * You should have received a copy of the GNU Lesser General Public Licence
 * along with Quibble.  If not, see <http://www.gnu.org/licenses/>. */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <wchar.h>
#ifndef NTLOADER_UTIL
#include "ntloader.h"
#endif
#include "reg.h"

#ifdef NTLOADER_UTIL
#define DBG(...) \
do \
{ \
    fprintf (stderr, __VA_ARGS__); \
} while (0)
#endif

enum reg_bool
{
    false = 0,
    true  = 1,
};

static inline int32_t
get_int32_size (uint8_t *data, uint64_t offset)
{
    int32_t ret;
    memcpy (&ret, data + offset, sizeof (int32_t));
    return -ret;
}

static enum reg_bool check_header(hive_t *h)
{
    HBASE_BLOCK *base_block = (HBASE_BLOCK *)h->data;
    uint32_t csum;
    enum reg_bool dirty = false;

    if (base_block->Signature != HV_HBLOCK_SIGNATURE)
    {
        DBG("Invalid signature.\n");
        return false;
    }

    if (base_block->Major != HSYS_MAJOR)
    {
        DBG("Invalid major value.\n");
        return false;
    }

    if (base_block->Minor < HSYS_MINOR)
    {
        DBG("Invalid minor value.\n");
        return false;
    }

    if (base_block->Type != HFILE_TYPE_PRIMARY)
    {
        DBG("Type was not HFILE_TYPE_PRIMARY.\n");
        return false;
    }

    if (base_block->Format != HBASE_FORMAT_MEMORY)
    {
        DBG("Format was not HBASE_FORMAT_MEMORY.\n");
        return false;
    }

    if (base_block->Cluster != 1)
    {
        DBG("Cluster was not 1.\n");
        return false;
    }

    if (base_block->Sequence1 != base_block->Sequence2)
    {
        DBG("Sequence1 != Sequence2.\n");
        base_block->Sequence2 = base_block->Sequence1;
        dirty = true;
    }

    // check checksum

    csum = 0;

    for (unsigned int i = 0; i < 127; i++)
        csum ^= ((uint32_t *)h->data)[i];

    if (csum == 0xffffffff)
        csum = 0xfffffffe;
    else if (csum == 0)
        csum = 1;

    if (csum != base_block->CheckSum)
    {
        DBG("Invalid checksum.\n");
        base_block->CheckSum = csum;
        dirty = true;
    }

    if (dirty)
    {
        DBG("Hive is dirty.\n");

        // FIXME - recover by processing LOG files (old style, < Windows 8.1)
        // FIXME - recover by processing LOG files (new style, >= Windows 8.1)
    }

    return true;
}

void reg_find_root(hive_t *h, HKEY *Key)
{
    HBASE_BLOCK *base_block = (HBASE_BLOCK *)h->data;
    *Key = 0x1000 + base_block->RootCell;
}

reg_err_t
reg_enum_keys(hive_t *h, HKEY Key, uint32_t Index,
              wchar_t *Name, uint32_t NameLength)
{
    int32_t size;
    CM_KEY_NODE *nk;
    CM_KEY_FAST_INDEX *lh;
    enum reg_bool overflow = false;

    // FIXME - make sure no buffer overruns (here and elsewhere)
    // find parent key node

    size = get_int32_size (h->data, Key);

    if (size < 0)
        return REG_ERR_FILE_NOT_FOUND;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_NODE, Name[0]))
        return REG_ERR_BAD_ARGUMENT;

    nk = (CM_KEY_NODE *)((uint8_t *)h->data + Key + sizeof(int32_t));

    if (nk->Signature != CM_KEY_NODE_SIGNATURE)
        return REG_ERR_BAD_ARGUMENT;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_NODE,
            Name[0]) + nk->NameLength)
        return REG_ERR_BAD_ARGUMENT;

    // FIXME - volatile keys?

    if (Index >= nk->SubKeyCount || nk->SubKeyList == 0xffffffff)
        return REG_ERR_FILE_NOT_FOUND;

    // go to key index

    size = get_int32_size (h->data, 0x1000 + nk->SubKeyList);

    if (size < 0)
        return REG_ERR_FILE_NOT_FOUND;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_FAST_INDEX, List[0]))
        return REG_ERR_BAD_ARGUMENT;

    lh = (CM_KEY_FAST_INDEX *)((uint8_t *)h->data + 0x1000
                               + nk->SubKeyList + sizeof(int32_t));

    if (lh->Signature == CM_KEY_INDEX_ROOT)
    {
        CM_KEY_INDEX *ri = (CM_KEY_INDEX *)lh;

        for (size_t i = 0; i < ri->Count; i++)
        {
            CM_KEY_FAST_INDEX *lh2 = (CM_KEY_FAST_INDEX *)((uint8_t *)h->data + 0x1000 + ri->List[i] +
                                             sizeof(int32_t));

            if (lh2->Signature == CM_KEY_INDEX_ROOT)
            {
                // Do not recurse: CVE-2021-3622
                DBG("Reading nested CM_KEY_INDEX is not yet implemented\n");
                return REG_ERR_BAD_ARGUMENT;
            }
            else if (lh2->Signature != CM_KEY_HASH_LEAF
                     && lh2->Signature != CM_KEY_FAST_LEAF)
                return REG_ERR_BAD_ARGUMENT;

            if (lh2->Count > Index)
            {
                lh = lh2;
                break;
            }

            Index -= lh2->Count;
        }
    }
    else if (lh->Signature != CM_KEY_HASH_LEAF && lh->Signature != CM_KEY_FAST_LEAF)
        return REG_ERR_BAD_ARGUMENT;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_FAST_INDEX,
            List[0]) + (lh->Count * sizeof(CM_INDEX)))
        return REG_ERR_BAD_ARGUMENT;

    if (Index >= lh->Count)
        return REG_ERR_BAD_ARGUMENT;

    // find child key node

    size = get_int32_size (h->data, 0x1000 + lh->List[Index].Cell);

    if (size < 0)
        return REG_ERR_FILE_NOT_FOUND;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_NODE, Name[0]))
        return REG_ERR_BAD_ARGUMENT;

    CM_KEY_NODE *nk2 = (CM_KEY_NODE *)((uint8_t *)h->data + 0x1000
                        + lh->List[Index].Cell + sizeof(int32_t));

    if (nk2->Signature != CM_KEY_NODE_SIGNATURE)
        return REG_ERR_BAD_ARGUMENT;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_NODE,
            Name[0]) + nk2->NameLength)
        return REG_ERR_BAD_ARGUMENT;

    if (nk2->Flags & KEY_COMP_NAME)
    {
        unsigned int i = 0;
        char *nkname = (char *)nk2->Name;

        for (i = 0; i < nk2->NameLength; i++)
        {
            if (i >= NameLength)
            {
                overflow = true;
                break;
            }

            Name[i] = nkname[i];
        }

        Name[i] = 0;
    }
    else
    {
        unsigned int i = 0;

        for (i = 0; i < nk2->NameLength / sizeof(wchar_t); i++)
        {
            if (i >= NameLength)
            {
                overflow = true;
                break;
            }

            Name[i] = nk2->Name[i];
        }

        Name[i] = 0;
    }

    return overflow ? REG_ERR_OUT_OF_MEMORY : REG_ERR_NONE;
}

static reg_err_t
find_child_key(hive_t *h, HKEY parent,
               const wchar_t *namebit, size_t nblen, HKEY *key)
{
    int32_t size;
    CM_KEY_NODE *nk;
    CM_KEY_FAST_INDEX *lh;

    // find parent key node

    size = get_int32_size (h->data, parent);

    if (size < 0)
        return REG_ERR_FILE_NOT_FOUND;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_NODE, Name[0]))
        return REG_ERR_BAD_ARGUMENT;

    nk = (CM_KEY_NODE *)((uint8_t *)h->data + parent + sizeof(int32_t));

    if (nk->Signature != CM_KEY_NODE_SIGNATURE)
        return REG_ERR_BAD_ARGUMENT;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_NODE,
            Name[0]) + nk->NameLength)
        return REG_ERR_BAD_ARGUMENT;

    if (nk->SubKeyCount == 0 || nk->SubKeyList == 0xffffffff)
        return REG_ERR_FILE_NOT_FOUND;

    // go to key index

    size = get_int32_size (h->data, 0x1000 + nk->SubKeyList);

    if (size < 0)
        return REG_ERR_FILE_NOT_FOUND;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_FAST_INDEX, List[0]))
        return REG_ERR_BAD_ARGUMENT;

    lh = (CM_KEY_FAST_INDEX *)((uint8_t *)h->data + 0x1000
                                + nk->SubKeyList + sizeof(int32_t));

    if (lh->Signature != CM_KEY_HASH_LEAF && lh->Signature != CM_KEY_FAST_LEAF)
        return REG_ERR_BAD_ARGUMENT;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_FAST_INDEX,
            List[0]) + (lh->Count * sizeof(CM_INDEX)))
        return REG_ERR_BAD_ARGUMENT;

    // FIXME - check against hashes if CM_KEY_HASH_LEAF

    for (unsigned int i = 0; i < lh->Count; i++)
    {
        CM_KEY_NODE *nk2;

        size = get_int32_size (h->data, 0x1000 + lh->List[i].Cell);

        if (size < 0)
            continue;

        if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_NODE, Name[0]))
            continue;

        nk2 = (CM_KEY_NODE *)((uint8_t *)h->data + 0x1000
                              + lh->List[i].Cell + sizeof(int32_t));

        if (nk2->Signature != CM_KEY_NODE_SIGNATURE)
            continue;

        if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_NODE,
                Name[0]) + nk2->NameLength)
            continue;

        // FIXME - use string protocol here to do comparison properly?

        if (nk2->Flags & KEY_COMP_NAME)
        {
            unsigned int j;
            char *name = (char *)nk2->Name;

            if (nk2->NameLength != nblen)
                continue;

            for (j = 0; j < nk2->NameLength; j++)
            {
                wchar_t c1 = name[j];
                wchar_t c2 = namebit[j];

                if (c1 >= 'A' && c1 <= 'Z')
                    c1 = c1 - 'A' + 'a';

                if (c2 >= 'A' && c2 <= 'Z')
                    c2 = c2 - 'A' + 'a';

                if (c1 != c2)
                    break;
            }

            if (j != nk2->NameLength)
                continue;

            *key = 0x1000 + lh->List[i].Cell;

            return REG_ERR_NONE;
        }
        else
        {
            unsigned int j;

            if (nk2->NameLength / sizeof(wchar_t) != nblen)
                continue;

            for (j = 0; j < nk2->NameLength / sizeof(wchar_t); j++)
            {
                wchar_t c1 = nk2->Name[j];
                wchar_t c2 = namebit[j];

                if (c1 >= 'A' && c1 <= 'Z')
                    c1 = c1 - 'A' + 'a';

                if (c2 >= 'A' && c2 <= 'Z')
                    c2 = c2 - 'A' + 'a';

                if (c1 != c2)
                    break;
            }

            if (j != nk2->NameLength / sizeof(wchar_t))
                continue;

            *key = 0x1000 + lh->List[i].Cell;

            return REG_ERR_NONE;
        }
    }

    return REG_ERR_FILE_NOT_FOUND;
}

reg_err_t
reg_find_key(hive_t *h, HKEY Parent, const wchar_t *Path, HKEY *Key)
{
    reg_err_t Status;
    size_t nblen;
    HKEY k;

    do
    {
        nblen = 0;
        while (Path[nblen] != '\\' && Path[nblen] != 0)
            nblen++;

        Status = find_child_key(h, Parent, Path, nblen, &k);
        if (Status)
            return Status;

        if (Path[nblen] == 0 || (Path[nblen] == '\\' && Path[nblen + 1] == 0))
        {
            *Key = k;
            return Status;
        }

        Parent = k;
        Path = &Path[nblen + 1];
    } while (true);
}

reg_err_t
reg_enum_values(hive_t *h, HKEY Key,
                uint32_t Index, wchar_t *Name,
                uint32_t NameLength, uint32_t *Type)
{
    int32_t size;
    CM_KEY_NODE *nk;
    uint32_t *list;
    CM_KEY_VALUE *vk;
    enum reg_bool overflow = false;

    // find key node

    size = get_int32_size (h->data, Key);

    if (size < 0)
        return REG_ERR_FILE_NOT_FOUND;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_NODE, Name[0]))
        return REG_ERR_BAD_ARGUMENT;

    nk = (CM_KEY_NODE *)((uint8_t *)h->data + Key + sizeof(int32_t));

    if (nk->Signature != CM_KEY_NODE_SIGNATURE)
        return REG_ERR_BAD_ARGUMENT;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_NODE,
            Name[0]) + nk->NameLength)
        return REG_ERR_BAD_ARGUMENT;

    if (Index >= nk->ValuesCount || nk->Values == 0xffffffff)
        return REG_ERR_FILE_NOT_FOUND;

    // go to key index

    size = get_int32_size (h->data, 0x1000 + nk->Values);

    if (size < 0)
        return REG_ERR_FILE_NOT_FOUND;

    if ((uint32_t)size < sizeof(int32_t) + (sizeof(uint32_t) * nk->ValuesCount))
        return REG_ERR_BAD_ARGUMENT;

    list = (uint32_t *)((uint8_t *)h->data + 0x1000 + nk->Values + sizeof(int32_t));

    // find value node

    size = get_int32_size (h->data, 0x1000 + list[Index]);

    if (size < 0)
        return REG_ERR_FILE_NOT_FOUND;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_VALUE, Name[0]))
        return REG_ERR_BAD_ARGUMENT;

    vk = (CM_KEY_VALUE *)((uint8_t *)h->data + 0x1000
                          + list[Index] + sizeof(int32_t));

    if (vk->Signature != CM_KEY_VALUE_SIGNATURE)
        return REG_ERR_BAD_ARGUMENT;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_VALUE,
            Name[0]) + vk->NameLength)
        return REG_ERR_BAD_ARGUMENT;

    if (vk->Flags & VALUE_COMP_NAME)
    {
        unsigned int i = 0;
        char *nkname = (char *)vk->Name;

        for (i = 0; i < vk->NameLength; i++)
        {
            if (i >= NameLength)
            {
                overflow = true;
                break;
            }

            Name[i] = nkname[i];
        }

        Name[i] = 0;
    }
    else
    {
        unsigned int i = 0;

        for (i = 0; i < vk->NameLength / sizeof(wchar_t); i++)
        {
            if (i >= NameLength)
            {
                overflow = true;
                break;
            }

            Name[i] = vk->Name[i];
        }

        Name[i] = 0;
    }

    *Type = vk->Type;

    return overflow ? REG_ERR_OUT_OF_MEMORY : REG_ERR_NONE;
}

reg_err_t
reg_query_value(hive_t *h, HKEY Key,
                const wchar_t *Name, void **Data,
                uint32_t *DataLength, uint32_t *Type)
{
    int32_t size;
    CM_KEY_NODE *nk;
    uint32_t *list;
    unsigned int namelen = wcslen(Name);

    // find key node

    size = get_int32_size (h->data, Key);

    if (size < 0)
        return REG_ERR_FILE_NOT_FOUND;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_NODE, Name[0]))
        return REG_ERR_BAD_ARGUMENT;

    nk = (CM_KEY_NODE *)((uint8_t *)h->data + Key + sizeof(int32_t));

    if (nk->Signature != CM_KEY_NODE_SIGNATURE)
        return REG_ERR_BAD_ARGUMENT;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_NODE,
            Name[0]) + nk->NameLength)
        return REG_ERR_BAD_ARGUMENT;

    if (nk->ValuesCount == 0 || nk->Values == 0xffffffff)
        return REG_ERR_FILE_NOT_FOUND;

    // go to key index

    size = get_int32_size (h->data, 0x1000 + nk->Values);

    if (size < 0)
        return REG_ERR_FILE_NOT_FOUND;

    if ((uint32_t)size < sizeof(int32_t) + (sizeof(uint32_t) * nk->ValuesCount))
        return REG_ERR_BAD_ARGUMENT;

    list = (uint32_t *)((uint8_t *)h->data + 0x1000 + nk->Values + sizeof(int32_t));

    // find value node

    for (unsigned int i = 0; i < nk->ValuesCount; i++)
    {
        CM_KEY_VALUE *vk;

        size = get_int32_size (h->data, 0x1000 + list[i]);

        if (size < 0)
            continue;

        if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_VALUE, Name[0]))
            continue;

        vk = (CM_KEY_VALUE *)((uint8_t *)h->data + 0x1000 + list[i] + sizeof(int32_t));

        if (vk->Signature != CM_KEY_VALUE_SIGNATURE)
            continue;

        if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_VALUE,
                Name[0]) + vk->NameLength)
            continue;

        if (vk->Flags & VALUE_COMP_NAME)
        {
            unsigned int j;
            char *valname = (char *)vk->Name;

            if (vk->NameLength != namelen)
                continue;

            for (j = 0; j < vk->NameLength; j++)
            {
                wchar_t c1 = valname[j];
                wchar_t c2 = Name[j];

                if (c1 >= 'A' && c1 <= 'Z')
                    c1 = c1 - 'A' + 'a';

                if (c2 >= 'A' && c2 <= 'Z')
                    c2 = c2 - 'A' + 'a';

                if (c1 != c2)
                    break;
            }

            if (j != vk->NameLength)
                continue;
        }
        else
        {
            unsigned int j;

            if (vk->NameLength / sizeof(wchar_t) != namelen)
                continue;

            for (j = 0; j < vk->NameLength / sizeof(wchar_t); j++)
            {
                wchar_t c1 = vk->Name[j];
                wchar_t c2 = Name[j];

                if (c1 >= 'A' && c1 <= 'Z')
                    c1 = c1 - 'A' + 'a';

                if (c2 >= 'A' && c2 <= 'Z')
                    c2 = c2 - 'A' + 'a';

                if (c1 != c2)
                    break;
            }

            if (j != vk->NameLength / sizeof(wchar_t))
                continue;
        }

        if (vk->DataLength & CM_KEY_VALUE_SPECIAL_SIZE)   // data stored as data offset
        {
            size_t datalen = vk->DataLength & ~CM_KEY_VALUE_SPECIAL_SIZE;
            uint8_t *ptr;
#if 0
            if (datalen == 4)
                ptr = (uint8_t *)&vk->Data;
            else if (datalen == 2)
                ptr = (uint8_t *)&vk->Data + 2;
            else if (datalen == 1)
                ptr = (uint8_t *)&vk->Data + 3;
#else
            if (datalen == 4 || datalen == 2 || datalen == 1)
                ptr = (uint8_t *)&vk->Data;
#endif
            else if (datalen == 0)
                ptr = NULL;
            else
                return REG_ERR_BAD_ARGUMENT;

            *Data = ptr;
        }
        else
        {
            size = get_int32_size (h->data, 0x1000 + vk->Data);

            if ((uint32_t)size < vk->DataLength)
                return REG_ERR_BAD_ARGUMENT;

            *Data = (uint8_t *)h->data + 0x1000 + vk->Data + sizeof(int32_t);
        }

        // FIXME - handle long "data block" values

        *DataLength = vk->DataLength & ~CM_KEY_VALUE_SPECIAL_SIZE;
        *Type = vk->Type;

        return REG_ERR_NONE;
    }

    return REG_ERR_FILE_NOT_FOUND;
}

static void clear_volatile(hive_t *h, HKEY key)
{
    int32_t size;
    CM_KEY_NODE *nk;
    uint16_t sig;

    size = get_int32_size (h->data, key);

    if (size < 0)
        return;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_NODE, Name[0]))
        return;

    nk = (CM_KEY_NODE *)((uint8_t *)h->data + key + sizeof(int32_t));

    if (nk->Signature != CM_KEY_NODE_SIGNATURE)
        return;

    nk->VolatileSubKeyList = 0xbaadf00d;
    nk->VolatileSubKeyCount = 0;

    if (nk->SubKeyCount == 0 || nk->SubKeyList == 0xffffffff)
        return;

    size = get_int32_size (h->data, 0x1000 + nk->SubKeyList);
    memcpy (&sig, (uint8_t*)h->data + 0x1000 + nk->SubKeyList + sizeof(int32_t), sizeof(uint16_t));

    if (sig == CM_KEY_HASH_LEAF || sig == CM_KEY_FAST_LEAF)
    {
        CM_KEY_FAST_INDEX *lh =
            (CM_KEY_FAST_INDEX *)((uint8_t *)h->data + 0x1000
                                  + nk->SubKeyList + sizeof(int32_t));

        for (unsigned int i = 0; i < lh->Count; i++)
            clear_volatile(h, 0x1000 + lh->List[i].Cell);
    }
    else if (sig == CM_KEY_INDEX_ROOT)
    {
        CM_KEY_INDEX *ri =
            (CM_KEY_INDEX *)((uint8_t *)h->data + 0x1000
                             + nk->SubKeyList + sizeof(int32_t));

        for (unsigned int i = 0; i < ri->Count; i++)
            clear_volatile(h, 0x1000 + ri->List[i]);
    }
    else
    {
        DBG("Unhandled registry signature 0x%x.\n", sig);
    }
}

static enum reg_bool validate_bins(const uint8_t *data, size_t len)
{
    size_t off = 0;

    data += 0x1000;

    while (off < len)
    {
        const HBIN *hb = (HBIN *)data;
        if (hb->Signature != HV_HBIN_SIGNATURE)
        {
            DBG("Invalid hbin signature in hive_t at offset %zx.\n", off);
            return false;
        }

        if (hb->FileOffset != off)
        {
            DBG("hbin FileOffset in hive_t was 0x%x, expected %zx.\n", hb->FileOffset,
                 off);
            return false;
        }

        if (hb->Size > len - off)
        {
            DBG("hbin overrun in hive_t at offset %zx.\n", off);
            return false;
        }

        if (hb->Size & 0xfff)
        {
            DBG("hbin Size in hive_t at offset %zx was not multiple of 1000.\n", off);
            return false;
        }

        off += hb->Size;
        data += hb->Size;
    }

    return true;
}

reg_err_t reg_open_hive(hive_t *h)
{
    if (!check_header(h))
    {
        DBG("Header check failed.\n");
        return REG_ERR_BAD_ARGUMENT;
    }

    const HBASE_BLOCK *base_block = (HBASE_BLOCK *)h->data;

    // do sanity-checking of hive_t, to avoid a bug check 74 later on
    if (!validate_bins((uint8_t *)h->data, base_block->Length))
    {
        return REG_ERR_BAD_ARGUMENT;
    }

    clear_volatile(h, 0x1000 + base_block->RootCell);

    return REG_ERR_NONE;
}
