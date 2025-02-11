/*
 * Copyright (C) 2012 Michael Brown <mbrown@fensystems.co.uk>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

/**
 * @file
 *
 * Main entry point
 *
 */

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "ntloader.h"
#include "peloader.h"
#include "int13.h"
#include "vdisk.h"
#include "payload.h"
#include "cmdline.h"
#include "paging.h"
#include "memmap.h"
#include "biosdisk.h"

/** Start of our image (defined by linker) */
extern char _start[];

/** End of our image (defined by linker) */
extern char _end[];

/** Command line */
char *cmdline;

/** initrd */
void *initrd;

/** Length of initrd */
size_t initrd_len;

/** Minimal length of embedded bootmgr.exe */
#define BOOTMGR_MIN_LEN 16384

/** 1MB memory threshold */
#define ADDR_1MB 0x00100000

/** 2GB memory threshold */
#define ADDR_2GB 0x80000000

/** Memory regions */
enum
{
    WIMBOOT_REGION = 0,
    PE_REGION,
    INITRD_REGION,
    NUM_REGIONS
};

/**
 * Wrap interrupt callback
 *
 * @v params		Parameters
 */
static void call_interrupt_wrapper (struct bootapp_callback_params *params)
{
    struct paging_state state;

    /* Handle/modify/pass-through interrupt as required */
    if (params->vector.interrupt == 0x13)
    {

        /* Enable paging */
        enable_paging (&state);

        /* Intercept INT 13 calls for the emulated drive */
        emulate_int13 (params);

        /* Disable paging */
        disable_paging (&state);

    }
    else
    {

        /* Pass through interrupt */
        call_interrupt (params);
    }
}

/** Real-mode callback functions */
static struct bootapp_callback_functions callback_fns =
{
    .call_interrupt = call_interrupt_wrapper,
    .call_real = call_real,
};

/** Real-mode callbacks */
static struct bootapp_callback callback =
{
    .fns = &callback_fns,
};

/** Boot application descriptor set */
static struct
{
    /** Boot application descriptor */
    struct bootapp_descriptor bootapp;
    /** Boot application memory descriptor */
    struct bootapp_memory_descriptor memory;
    /** Boot application memory descriptor regions */
    struct bootapp_memory_region regions[NUM_REGIONS];
    /** Boot application entry descriptor */
    struct bootapp_entry_descriptor entry;
    struct bootapp_entry_wtf1_descriptor wtf1;
    struct bootapp_entry_wtf2_descriptor wtf2;
    struct bootapp_entry_wtf3_descriptor wtf3;
    struct bootapp_entry_wtf3_descriptor wtf3_copy;
    /** Boot application callback descriptor */
    struct bootapp_callback_descriptor callback;
    /** Boot application pointless descriptor */
    struct bootapp_pointless_descriptor pointless;
} __attribute__ ((packed)) bootapps =
{
    .bootapp = {
        .signature = BOOTAPP_SIGNATURE,
        .version = BOOTAPP_VERSION,
        .len = sizeof (bootapps),
        .arch = BOOTAPP_ARCH_I386,
        .memory = offsetof (typeof (bootapps), memory),
        .entry = offsetof (typeof (bootapps), entry),
        .xxx = offsetof (typeof (bootapps), wtf3_copy),
        .callback = offsetof (typeof (bootapps), callback),
        .pointless = offsetof (typeof (bootapps), pointless),
    },
    .memory = {
        .version = BOOTAPP_MEMORY_VERSION,
        .len = sizeof (bootapps.memory),
        .num_regions = NUM_REGIONS,
        .region_len = sizeof (bootapps.regions[0]),
        .reserved_len = sizeof (bootapps.regions[0].reserved),
    },
    .entry = {
        .signature = BOOTAPP_ENTRY_SIGNATURE,
        .flags = BOOTAPP_ENTRY_FLAGS,
    },
    .wtf1 = {
        .flags = 0x11000001,
        .len = sizeof (bootapps.wtf1),
        .extra_len = (sizeof (bootapps.wtf2) +
                       sizeof (bootapps.wtf3)),
    },
    .wtf3 = {
        .flags = 0x00000006,
        .len = sizeof (bootapps.wtf3),
        .boot_partition_offset = (VDISK_VBR_LBA * VDISK_SECTOR_SIZE),
        .xxx = 0x01,
        .mbr_signature = VDISK_MBR_SIGNATURE,
    },
    .wtf3_copy = {
        .flags = 0x00000006,
        .len = sizeof (bootapps.wtf3),
        .boot_partition_offset = (VDISK_VBR_LBA * VDISK_SECTOR_SIZE),
        .xxx = 0x01,
        .mbr_signature = VDISK_MBR_SIGNATURE,
    },
    .callback = {
        .callback = &callback,
    },
    .pointless = {
        .version = BOOTAPP_POINTLESS_VERSION,
    },
};

/**
 * Relocate data between 1MB and 2GB if possible
 *
 * @v data		Start of data
 * @v len		Length of data
 * @ret start		Start address
 */
static void *relocate_memory_low (void *data, size_t len)
{
    struct e820_entry *e820 = NULL;
    uint64_t end;
    intptr_t start;

    /* Read system memory map */
    while ((e820 = memmap_next (e820)) != NULL)
    {

        /* Find highest compatible placement within this region */
        end = (e820->start + e820->len);
        start = ((end > ADDR_2GB) ? ADDR_2GB : end);
        if (start < len)
            continue;
        start -= len;
        start &= ~(PAGE_SIZE - 1);
        if (start < e820->start)
            continue;
        if (start < ADDR_1MB)
            continue;

        /* Relocate to this region */
        memmove ((void *) start, data, len);
        return ((void *) start);
    }

    /* Leave at original location */
    return data;
}

/**
 * Main entry point
 *
 */
int main (void)
{
    struct loaded_pe pe;
    struct paging_state state;
    uint64_t initrd_phys;

    /* Initialise stack cookie */
    init_cookie();

    /* Print welcome banner */
    printf ("\n\nntloader " VERSION "\n\n");

    /* Process command line */
    process_cmdline (cmdline);

    biosdisk_init ();
    biosdisk_iterate ();

    /* Initialise paging */
    init_paging();

    /* Enable paging */
    enable_paging (&state);

    /* Relocate initrd below 2GB if possible, to avoid collisions */
    DBG ("Found initrd at [%p,%p)\n", initrd, (initrd + initrd_len));
    initrd = relocate_memory_low (initrd, initrd_len);
    DBG ("Placing initrd at [%p,%p)\n", initrd, (initrd + initrd_len));

    /* Extract files from initrd */
    extract_initrd (initrd, initrd_len);

    /* Add INT 13 drive */
    callback.drive = initialise_int13();

    /* Load bootmgr.exe into memory */
    if (load_pe (nt_cmdline->bootmgr, nt_cmdline->bootmgr_length, &pe) != 0)
        die ("FATAL: Could not load bootmgr.exe\n");

    /* Relocate initrd above 4GB if possible, to free up 32-bit memory */
    initrd_phys = relocate_memory_high (initrd, initrd_len);
    DBG ("Placing initrd at physical [%#llx,%#llx)\n",
          initrd_phys, (initrd_phys + initrd_len));

    /* Complete boot application descriptor set */
    bootapps.bootapp.pe_base = pe.base;
    bootapps.bootapp.pe_len = pe.len;
    bootapps.regions[WIMBOOT_REGION].start_page = page_start (_start);
    bootapps.regions[WIMBOOT_REGION].num_pages = page_len (_start, _end);
    bootapps.regions[PE_REGION].start_page = page_start (pe.base);
    bootapps.regions[PE_REGION].num_pages =
        page_len (pe.base, (pe.base + pe.len));
    bootapps.regions[INITRD_REGION].start_page =
        (initrd_phys / PAGE_SIZE);
    bootapps.regions[INITRD_REGION].num_pages =
        page_len (initrd, initrd + initrd_len);

    /* Omit initrd region descriptor if located above 4GB */
    if (initrd_phys >= ADDR_4GB)
        bootapps.memory.num_regions--;

    /* Disable paging */
    disable_paging (&state);

    /* Jump to PE image */
    DBG ("Entering bootmgr.exe with parameters at %p\n", &bootapps);
    pe.entry (&bootapps.bootapp);
    die ("FATAL: bootmgr.exe returned\n");
}
