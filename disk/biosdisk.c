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
#include <stdint.h>
#include <string.h>
#include <bootapp.h>
#include <cmdline.h>
#include <biosdisk.h>
#include <ntloader.h>
#include <msdos.h>
#include <gpt.h>
#include "int13.h"

#if defined(__i386__) || defined(__x86_64__)

/* The scratch buffer used in real mode code.  */
#define SCRATCH_ADDR  0x30000
#define SCRATCH_SEG   (SCRATCH_ADDR >> 4)
#define SCRATCH_SIZE  0x10000

static struct biosdisk_data disk_data;

static void
call_int13h (struct bootapp_callback_params *params)
{
    unsigned long eflags;
    __asm__ ("pushf\n\t"
             "pop %0\n\t"
             : "=r" (eflags));
    params->eflags = (eflags & ~CF) | IF;
    params->vector.interrupt = 0x13;
    call_interrupt (params);
}

static uint64_t
divmod64 (uint64_t n, uint64_t d, uint64_t *r)
{
    /* This algorithm is typically implemented by hardware. The idea
     *     is to get the highest bit in N, 64 times, by keeping
     *     upper(N * 2^i) = (Q * D + M), where upper
     *     represents the high 64 bits in 128-bits space.  */
    unsigned bits = 64;
    uint64_t q = 0;
    uint64_t m = 0;
    while (bits--)
    {
        m <<= 1;
        if (n & (1ULL << 63))
            m |= 1;
        q <<= 1;
        n <<= 1;
        if (m >= d)
        {
            q |= 1;
            m -= d;
        }
    }
    if (r)
        *r = m;
    return q;
}

/*
 *   Call IBM/MS INT13 Extensions (int 13 %ah=AH) for DRIVE. DAP
 *   is passed for disk address packet. If an error occurs, return
 *   non-zero, otherwise zero.
 */
static int
biosdisk_rw_int13_ext (int ah, int drive, void *dap)
{
    struct bootapp_callback_params regs;

    regs.eax = ah << 8;
    /* compute the address of disk_address_packet */
    regs.ds = (((size_t) dap) & 0xffff0000) >> 4;
    regs.esi = (((size_t) dap) & 0xffff);
    regs.edx = drive;
    call_int13h (&regs);
    return (regs.eax >> 8) & 0xff;
}

/*
 *   Call standard and old INT13 (int 13 %ah=AH) for DRIVE. Read/write
 *   NSEC sectors from COFF/HOFF/SOFF into SEGMENT. If an error occurs,
 *   return non-zero, otherwise zero.
 */
static int 
biosdisk_rw_std (int ah, int drive, int coff, int hoff,
                 int soff, int nsec, int segment)
{
    int ret, i;
    /* Try 3 times.  */
    for (i = 0; i < 3; i++)
    {
        struct bootapp_callback_params regs;

        /* set up CHS information */
        /* set %ch to low eight bits of cylinder */
        regs.ecx = (coff << 8) & 0xff00;
        /* set bits 6-7 of %cl to high two bits of cylinder */
        regs.ecx |= (coff >> 2) & 0xc0;
        /* set bits 0-5 of %cl to sector */
        regs.ecx |= soff & 0x3f;
        /* set %dh to head and %dl to drive */
        regs.edx = (drive & 0xff) | ((hoff << 8) & 0xff00);
        /* set %ah to AH */
        regs.eax = (ah << 8) & 0xff00;
        /* set %al to NSEC */
        regs.eax |= nsec & 0xff;
        regs.ebx = 0;
        regs.es = segment;
        call_int13h (&regs);
        /* check if successful */
        if (!(regs.eflags & CF))
            return 0;
        /* save return value */
        ret = regs.eax >> 8;
        /* if fail, reset the disk system */
        regs.eax = 0;
        regs.edx = (drive & 0xff);
        call_int13h (&regs);
    }
    return ret;
}

/*
 *   Check if LBA is supported for DRIVE. If it is supported, then return
 *   the major version of extensions, otherwise zero.
 */
static int
biosdisk_check_int13_ext (int drive)
{
    struct bootapp_callback_params regs;

    regs.edx = drive & 0xff;
    regs.eax = 0x4100;
    regs.ebx = 0x55aa;
    call_int13h (&regs);
    if (regs.eflags & CF)
        return 0;
    if ((regs.ebx & 0xffff) != 0xaa55)
        return 0;
    /* check if AH=0x42 is supported */
    if (!(regs.ecx & 1))
        return 0;
    return (regs.eax >> 8) & 0xff;
}

/*
 *   Return the geometry of DRIVE in CYLINDERS, HEADS and SECTORS. If an
 *   error occurs, then return non-zero, otherwise zero.
 */
static int
biosdisk_get_diskinfo_std (int drive, unsigned long *cylinders,
                           unsigned long *heads,
                           unsigned long *sectors)
{
    struct bootapp_callback_params regs;

    regs.eax = 0x0800;
    regs.ecx = 0;
    regs.edx = drive & 0xff;
    call_int13h (&regs);
    /* Check if unsuccessful. Ignore return value if carry isn't set to
     *     workaround some buggy BIOSes. */
    if ((regs.eflags & CF) && ((regs.eax & 0xff00) != 0))
        return (regs.eax & 0xff00) >> 8;
    /* bogus BIOSes may not return an error number */
    /* 0 sectors means no disk */
    if (!(regs.ecx & 0x3f))
        /* XXX 0x60 is one of the unused error numbers */
        return 0x60;
    /* the number of heads is counted from zero */
    *heads = ((regs.edx >> 8) & 0xff) + 1;
    *cylinders = (((regs.ecx >> 8) & 0xff) | ((regs.ecx << 2) & 0x0300)) + 1;
    *sectors = regs.ecx & 0x3f;
    return 0;
}

static int
biosdisk_get_diskinfo_real (int drive, void *drp, uint16_t ax)
{
    struct bootapp_callback_params regs;

    regs.eax = ax;
    /* compute the address of drive parameters */
    regs.esi = ((size_t) drp) & 0xf;
    regs.ds = ((size_t) drp) >> 4;
    regs.edx = drive & 0xff;
    call_int13h (&regs);
    /* Check if unsuccessful. Ignore return value if carry isn't set to
     *     workaround some buggy BIOSes. */
    if ((regs.eflags & CF) && ((regs.eax & 0xff00) != 0))
        return (regs.eax & 0xff00) >> 8;
    return 0;
}

/*
 *   Return the geometry of DRIVE in a drive parameters, DRP. If an error
 *   occurs, then return non-zero, otherwise zero.
 */
static inline int
biosdisk_get_diskinfo_int13_ext (int drive, void *drp)
{
    return biosdisk_get_diskinfo_real (drive, drp, 0x4800);
}

static int
biosdisk_read_real (uint64_t sector, size_t size, unsigned segment)
{
    if (disk_data.flags & BIOSDISK_FLAG_LBA)
    {
        struct biosdisk_dap *dap;
        dap = ABS_PTR (SCRATCH_ADDR +
                (disk_data.sectors << disk_data.log_sector_size));
        dap->length = sizeof (*dap);
        dap->reserved = 0;
        dap->blocks = size;
        dap->buffer = segment << 16; /* The format SEGMENT:ADDRESS. */
        dap->block = sector;

        if (biosdisk_rw_int13_ext (0x42, disk_data.drive, dap))
        {
            /* Fall back to the CHS mode.  */
            disk_data.flags &= ~BIOSDISK_FLAG_LBA;
            disk_data.total_sectors =
            disk_data.cylinders * disk_data.heads * disk_data.sectors;
            return biosdisk_read_real (sector, size, segment);
        }
    }
    else
    {
        unsigned coff, hoff, soff;
        unsigned head;
        /* It is impossible to reach over 8064 MiB
         * (a bit less than LBA24)
         * with  the traditional CHS access.  */
        if (sector > 1024 /* cylinders */
            * 256 /* heads */
            * 63 /* spt */)
        {
            DBG ("attempt to read outside of hd%2x\n", disk_data.drive);
            return 0;
        }

        soff = ((uint32_t) sector) % disk_data.sectors + 1;
        head = ((uint32_t) sector) / disk_data.sectors;
        hoff = head % disk_data.heads;
        coff = head / disk_data.heads;

        if (coff >= disk_data.cylinders)
        {
            DBG ("attempt to read outside of hd%2x\n", disk_data.drive);
            return 0;
        }

        if (biosdisk_rw_std (0x02, disk_data.drive,
                             coff, hoff, soff, size, segment))
        {
            DBG ("failure reading sector 0x%llx from hd%2x\n",
                 sector, disk_data.drive);
            return 0;
        }
    }
    return 1;
}

int
biosdisk_read (void *disk __attribute__ ((unused)),
               uint64_t sector, size_t len, void *buf)
{
    uint32_t len_lba = (len + 511) >> 9;
    if (!biosdisk_read_real (sector, len_lba, SCRATCH_SEG))
        return 0;
    memcpy (buf, ABS_PTR (SCRATCH_ADDR), len);

    return 1;
}

static int
biosdisk_fill_data (int drive)
{
    uint64_t total_sectors = 0;
    int version;

    memset (&disk_data, 0, sizeof (struct biosdisk_data));
    disk_data.drive = drive;
    disk_data.log_sector_size = 9;
    version = biosdisk_check_int13_ext (drive);
    if (version)
    {
        struct biosdisk_drp *drp = ABS_PTR (SCRATCH_ADDR);
        /* Clear out the DRP.  */
        memset (drp, 0, sizeof (*drp));
        drp->size = sizeof (*drp);
        if (! biosdisk_get_diskinfo_int13_ext (drive, drp))
        {
            disk_data.flags = BIOSDISK_FLAG_LBA;
            if (drp->total_sectors)
                total_sectors = drp->total_sectors;
            else
            {
                /* Some buggy BIOSes doesn't return the total sectors
                 * correctly but returns zero.
                 * So if it is zero, compute
                 * it by C/H/S returned by the LBA BIOS call.  */
                total_sectors = ((uint64_t) drp->cylinders)
                                * drp->heads * drp->sectors;
            }
            if (drp->bytes_per_sector
                && !(drp->bytes_per_sector
                     & (drp->bytes_per_sector - 1))
                && drp->bytes_per_sector >= 512
                && drp->bytes_per_sector <= 16384)
            {
                SECTOR_LOG2ULL (disk_data.log_sector_size,
                                drp->bytes_per_sector);
            }
        }
    }

    if (biosdisk_get_diskinfo_std (drive,
                                   &disk_data.cylinders,
                                   &disk_data.heads,
                                   &disk_data.sectors) != 0)
    {
        if (total_sectors && (disk_data.flags & BIOSDISK_FLAG_LBA))
        {
            disk_data.sectors = 63;
            disk_data.heads = 255;
            disk_data.cylinders = divmod64 (total_sectors +
                disk_data.heads * disk_data.sectors - 1,
                disk_data.heads * disk_data.sectors, 0);
        }
        else
        {
            DBG ("hd%2x cannot get C/H/S values.\n", drive);
            return 0;
        }
    }
    if (disk_data.sectors == 0)
        disk_data.sectors = 63;
    if (disk_data.heads == 0)
        disk_data.heads = 255;
    if (! total_sectors)
        total_sectors = ((uint64_t) disk_data.cylinders) *
        disk_data.heads * disk_data.sectors;

    disk_data.total_sectors = total_sectors;
    return 1;
}

#endif /* defined(__i386__) || defined(__x86_64__) */

void
biosdisk_iterate (void)
{
#if defined(__i386__) || defined(__x86_64__)
    int drive;
    struct biosdisk_data *d = &disk_data;

    /* For hard disks, attempt to read the MBR.  */
    for (drive = 0x80; drive < 0x90; drive++)
    {
        if (biosdisk_rw_std (0x02, drive, 0, 0, 1, 1, SCRATCH_SEG) != 0)
        {
            DBG ("read error when probing drive 0x%2x\n", drive);
            break;
        }
        if (!biosdisk_fill_data (drive))
            continue;
        DBG ("hd%2x\n", d->drive);
        if (check_msdos_partmap (d, biosdisk_read))
            break;
        if (check_gpt_partmap (d, biosdisk_read))
            break;
    }
#endif /* defined(__i386__) || defined(__x86_64__) */
}

void
biosdisk_init (void)
{
#if defined(__i386__) || defined(__x86_64__)
    DBG ("scratch addr 0x%x\n", SCRATCH_ADDR);
    memset (ABS_PTR (SCRATCH_ADDR), 0, sizeof (SCRATCH_SIZE));
#endif /* defined(__i386__) || defined(__x86_64__) */
}

void
biosdisk_fini (void)
{
}
