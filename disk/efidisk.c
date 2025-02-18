/*
 *  ntloader  --  Microsoft Windows NT6+ loader
 *  Copyright (C) 2025  A1ive.
 *
 *  ntloader is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published
 *  by the Free Software Foundation, either version 3 of the License,
 *  or (at your option) any later version.
 *
 *  ntloader is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with ntloader.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <ntloader.h>
#include <efi.h>
#include <efidisk.h>
#include <msdos.h>
#include <gpt.h>

static struct efidisk_data *efi_hd = 0;
static struct efidisk_data *efi_cd = 0;
static struct efidisk_data *efi_fd = 0;

static EFI_HANDLE *
locate_handle (EFI_LOCATE_SEARCH_TYPE search_type,
               EFI_GUID *protocol, void *search_key, UINTN *num_handles)
{
    EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
    EFI_STATUS status;
    EFI_HANDLE *buffer;
    UINTN buffer_size = 8 * sizeof (EFI_HANDLE);

    buffer = efi_malloc (buffer_size);

    status = bs->LocateHandle (search_type, protocol, search_key,
                               &buffer_size, buffer);
    if (status == EFI_BUFFER_TOO_SMALL)
    {
        efi_free (buffer);
        buffer = efi_malloc (buffer_size);
        status = bs->LocateHandle (search_type, protocol, search_key,
                                   &buffer_size, buffer);
    }

    if (status != EFI_SUCCESS)
        efi_free (buffer);

    *num_handles = buffer_size / sizeof (EFI_HANDLE);
    return buffer;
}

static void *
open_protocol (EFI_HANDLE handle, EFI_GUID *protocol, UINT32 attributes)
{
    EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
    EFI_STATUS status;
    void *interface;
    status = bs->OpenProtocol (handle, protocol, &interface,
                               efi_image_handle, 0, attributes);
    if (status != EFI_SUCCESS)
        return 0;
    return interface;
}

static EFI_DEVICE_PATH_PROTOCOL *
get_device_path (EFI_HANDLE handle)
{
    return open_protocol (handle, &efi_device_path_protocol_guid,
                          EFI_OPEN_PROTOCOL_GET_PROTOCOL);
}

#define DP_TYPE(dp)    ((dp)->Type & 0x7f)
#define DP_SUBTYPE(dp) ((dp)->SubType)
#define DP_LENGTH(dp)  (((UINT16)((dp)->Length[1] << 8 )) | (dp)->Length[0])
#define DP_VALID(dp)   ((dp) != NULL && (DP_LENGTH (dp) >= 4))

#define END_ENTIRE_DP(dp) \
(!DP_VALID (dp) || (DP_TYPE (dp) == END_DEVICE_PATH_TYPE \
&& (DP_SUBTYPE (dp) == END_ENTIRE_DEVICE_PATH_SUBTYPE)))

#define NEXT_DP(dp) \
(DP_VALID (dp) ? ((EFI_DEVICE_PATH_PROTOCOL *) \
((char *) (dp) + DP_LENGTH (dp))) : NULL)

#define EFI_VENDOR_APPLE_GUID \
{ 0x2B0585EB, 0xD8B8, 0x49A9,	\
    { 0x8B, 0x8C, 0xE2, 0x1B, 0x01, 0xAE, 0xF2, 0xB7 } \
}

/** Return the device path node right before the end node.  */
EFI_DEVICE_PATH_PROTOCOL *
find_last_device_path (const EFI_DEVICE_PATH_PROTOCOL *dp)
{
    EFI_DEVICE_PATH_PROTOCOL *next, *p;
    if (END_ENTIRE_DP (dp))
        return 0;
    for (p = (void *) dp, next = NEXT_DP (p);! END_ENTIRE_DP (next);
         p = next, next = NEXT_DP (next))
         ;
    return p;
}

/* Compare device paths.  */
static int
compare_device_paths (const EFI_DEVICE_PATH *dp1, const EFI_DEVICE_PATH *dp2)
{
    if (! dp1 || ! dp2)
        return 1;
    if (dp1 == dp2)
        return 0;
    while (DP_VALID (dp1) && DP_VALID (dp2))
    {
        UINT8 type1, type2;
        UINT8 subtype1, subtype2;
        UINT16 len1, len2;
        int ret;
        type1 = DP_TYPE (dp1);
        type2 = DP_TYPE (dp2);
        if (type1 != type2)
            return (int) type2 - (int) type1;
        subtype1 = DP_SUBTYPE (dp1);
        subtype2 = DP_SUBTYPE (dp2);
        if (subtype1 != subtype2)
            return (int) subtype1 - (int) subtype2;
        len1 = DP_LENGTH (dp1);
        len2 = DP_LENGTH (dp2);
        if (len1 != len2)
            return (int) len1 - (int) len2;
        ret = memcmp (dp1, dp2, len1);
        if (ret != 0)
            return ret;
        if (END_ENTIRE_DP (dp1))
            break;
        dp1 = (EFI_DEVICE_PATH *) ((char *) dp1 + len1);
        dp2 = (EFI_DEVICE_PATH *) ((char *) dp2 + len2);
    }

    /*
     * There's no "right" answer here, but we probably don't want to call a valid
     * dp and an invalid dp equal, so pick one way or the other.
     */
    if (DP_VALID (dp1) && !DP_VALID (dp2))
        return 1;
    else if (!DP_VALID (dp1) && DP_VALID (dp2))
        return -1;

    return 0;
}

/* Duplicate a device path.  */
EFI_DEVICE_PATH *
duplicate_device_path (const EFI_DEVICE_PATH *dp)
{
    EFI_DEVICE_PATH *p;
    size_t total_size = 0;
    for (p = (EFI_DEVICE_PATH *) dp; ; p = NEXT_DP (p))
    {
        size_t len = DP_LENGTH (p);
        /*
         * In the event that we find a node that's completely garbage, for
         * example if we get to 0x7f 0x01 0x02 0x00 ... (EndInstance with a size
         * of 2), END_ENTIRE_DP() will be true and
         * NEXT_DP() will return NULL, so we won't continue,
         * and neither should our consumers, but there won't be any error raised
         * even though the device path is junk.
         *
         * This keeps us from passing junk down back to our caller.
         */
        if (len < 4)
        {
            DBG ("WARNING: malformed EFI Device Path node.");
            return NULL;
        }
        total_size += len;
        if (END_ENTIRE_DP (p))
            break;
    }
    p = efi_malloc (total_size);
    if (! p)
        return 0;
    memcpy (p, dp, total_size);
    return p;
}

static struct efidisk_data *
make_devices (void)
{
    UINTN num_handles;
    EFI_HANDLE *handles;
    EFI_HANDLE *handle;
    struct efidisk_data *devices = 0;

    /* Find handles which support the disk io interface.  */
    handles = locate_handle (ByProtocol, &efi_block_io_protocol_guid,
                             0, &num_handles);
    if (! handles)
        return 0;

    for (handle = handles; num_handles--; handle++)
    {
        EFI_DEVICE_PATH_PROTOCOL *dp;
        EFI_DEVICE_PATH_PROTOCOL *ldp;
        EFI_BLOCK_IO_PROTOCOL *bio;
        struct efidisk_data *d;

        dp = get_device_path (*handle);
        if (! dp)
            continue;
        ldp = find_last_device_path (dp);
        if (! ldp)
            continue;

        bio = open_protocol (*handle, &efi_block_io_protocol_guid,
                             EFI_OPEN_PROTOCOL_GET_PROTOCOL);
        if (! bio || !bio->Media)
            continue;

        /* iPXE adds stub Block IO protocol to loaded image device handle. It is
         *       completely non-functional and simply returns an error for every method.
         *       So attempt to detect and skip it. Magic number is literal "iPXE" and
         *       check block size as well */
        if (bio->Media->MediaId == 0x69505845U && bio->Media->BlockSize == 1)
            continue;
        if (bio->Media->BlockSize & (bio->Media->BlockSize - 1) ||
            bio->Media->BlockSize < 512)
            continue;

        //print_device_path (dp);
        d = efi_malloc (sizeof (*d));
        d->handle = *handle;
        d->dp = dp;
        d->ldp = ldp;
        d->bio = bio;
        d->next = devices;
        devices = d;
    }

    efi_free (handles);

    return devices;
}

/* Find the parent device.  */
static struct efidisk_data *
find_parent_device (struct efidisk_data *devices, struct efidisk_data *d)
{
    EFI_DEVICE_PATH *dp, *ldp;
    struct efidisk_data *parent;
    dp = duplicate_device_path (d->dp);
    if (! dp)
        return 0;
    ldp = find_last_device_path (dp);
    if (! ldp)
        return 0;
    ldp->Type = END_DEVICE_PATH_TYPE;
    ldp->SubType = END_ENTIRE_DEVICE_PATH_SUBTYPE;
    ldp->Length[0] = sizeof (*ldp) & 0xff;
    ldp->Length[1] = sizeof (*ldp) >> 8;
    for (parent = devices; parent; parent = parent->next)
    {
        /* Ignore itself.  */
        if (parent == d)
            continue;
        if (compare_device_paths (parent->dp, dp) == 0)
            break;
    }
    efi_free (dp);
    return parent;
}

#define FOR_CHILDREN(p, dev) for (p = dev; p; p = p->next) if (is_child (p, d))

/* Add a device into a list of devices in an ascending order.  */
static void
add_device (struct efidisk_data **devices, struct efidisk_data *d)
{
    struct efidisk_data **p;
    struct efidisk_data *n;
    for (p = devices; *p; p = &((*p)->next))
    {
        int ret;
        ret = compare_device_paths (find_last_device_path ((*p)->dp),
                                    find_last_device_path (d->dp));
        if (ret == 0)
            ret = compare_device_paths ((*p)->dp, d->dp);
        if (ret == 0)
            return;
        else if (ret > 0)
            break;
    }
    n = efi_malloc (sizeof (*n));
    if (! n)
        return;
    memcpy (n, d, sizeof (*n));
    n->next = (*p);
    (*p) = n;
}

/* Name the devices.  */
static void
name_devices (struct efidisk_data *devices)
{
    struct efidisk_data *d;

    /* First, identify devices by media device paths.  */
    for (d = devices; d; d = d->next)
    {
        EFI_DEVICE_PATH *dp;
        dp = d->ldp;
        if (! dp)
            continue;
        if (DP_TYPE (dp) == MEDIA_DEVICE_PATH)
        {
            int is_hard_drive = 0;
            switch (DP_SUBTYPE (dp))
            {
                case MEDIA_HARDDRIVE_DP:
                    is_hard_drive = 1;
                    /* Intentionally fall through.  */
                    case MEDIA_CDROM_DP:
                    {
                        struct efidisk_data *parent, *parent2;
                        parent = find_parent_device (devices, d);
                        if (!parent)
                            break;
                        parent2 = find_parent_device (devices, parent);
                        if (parent2)
                        {
                            /* Mark itself as used.  */
                            d->ldp = 0;
                            break;
                        }
                        if (!parent->ldp)
                        {
                            d->ldp = 0;
                            break;
                        }
                        if (is_hard_drive)
                            add_device (&efi_hd, parent);
                        else
                            add_device (&efi_cd, parent);
                        /* Mark the parent as used.  */
                        parent->ldp = 0;
                        /* Mark itself as used.  */
                        d->ldp = 0;
                        break;
                    }
                    default:
                        break;
            }
        }
    }

    /* Let's see what can be added more.  */
    for (d = devices; d; d = d->next)
    {
        EFI_DEVICE_PATH *dp;
        EFI_BLOCK_IO_MEDIA *m;
        int is_floppy = 0;

        dp = d->ldp;
        if (! dp)
            continue;

        /* Ghosts proudly presented by Apple.  */
        if (DP_TYPE (dp) == MEDIA_DEVICE_PATH && DP_SUBTYPE (dp) == MEDIA_VENDOR_DP)
        {
            VENDOR_DEVICE_PATH *vendor = (VENDOR_DEVICE_PATH *) dp;
            const EFI_GUID apple = EFI_VENDOR_APPLE_GUID;

            if (DP_LENGTH (&vendor->Header) == sizeof (*vendor) &&
                memcmp (&vendor->Guid, &apple, sizeof (vendor->Guid)) == 0 &&
                find_parent_device (devices, d))
                continue;
        }

        m = d->bio->Media;
        if (DP_TYPE (dp) == ACPI_DEVICE_PATH && DP_SUBTYPE (dp) == ACPI_DP)
        {
            ACPI_HID_DEVICE_PATH *acpi = (ACPI_HID_DEVICE_PATH *) dp;
            /* Floppy EISA ID.  */
            if (acpi->HID == 0x60441d0 || acpi->HID == 0x70041d0 ||
                acpi->HID == 0x70141d1)
                is_floppy = 1;
        }
        if (is_floppy)
            add_device (&efi_hd, d);
        else if (m->ReadOnly && m->BlockSize > 512)
        {
            /* This check is too heuristic, but assume that this is a
             *         CDROM drive.  */
            add_device (&efi_cd, d);
        }
        else
        {
            /* The default is a hard drive.  */
            add_device (&efi_hd, d);
        }
    }
}

static void
free_devices (struct efidisk_data *devices)
{
    struct efidisk_data *p, *q;
    for (p = devices; p; p = q)
    {
        q = p->next;
        efi_free (p);
    }
}

int
efidisk_read (void *disk, uint64_t sector, size_t len, void *buf)
{
    struct efidisk_data *d = disk;
    EFI_BLOCK_IO_PROTOCOL *bio;
    EFI_STATUS status;
    UINTN pages = BYTES_TO_PAGES (len);
    void *mem = efi_allocate_pages (pages, EfiLoaderData);

    bio = d->bio;

    status = bio->ReadBlocks (bio, bio->Media->MediaId,
                              sector, PAGES_TO_BYTES (pages), mem);
    memcpy (buf, mem, len);
    efi_free_pages (mem, pages);

    if (status != EFI_SUCCESS)
    {
        DBG ("failure reading sector 0x%llx from %s\n", sector, d->name);
        return 0;
    }

    return 1;
}

void
efidisk_iterate (void)
{
    struct efidisk_data *d;
    int count;
    for (d = efi_hd, count = 0; d; d = d->next, count++)
    {
        snprintf (d->name, 16, "hd%d", count);
        DBG ("%s\n", d->name);
        if (check_msdos_partmap (d, efidisk_read))
            break;
        if (check_gpt_partmap (d, efidisk_read))
            break;
    }
}

void
efidisk_init (void)
{
    struct efidisk_data *devices;
    devices = make_devices ();
    if (! devices)
        return;
    name_devices (devices);
    free_devices (devices);
}

void
efidisk_fini (void)
{
    free_devices (efi_fd);
    free_devices (efi_hd);
    free_devices (efi_cd);
}
