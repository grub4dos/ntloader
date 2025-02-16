ENTRY ( efi_main )

/* x86 Memory Map
 * start		end			size		description	type
 * 640 KiB		Low memory
 * 0x00000000	0x000003FF	1 KiB		IVT
 * 0x00000400	0x000004FF	256 bytes	BDA
 * 0x00000500	0x00007BFF	29.75 KiB	Conventional memory
 * 0x00007C00	0x00007DFF	512 bytes	BootSector
 * 0x00007E00	0x0007FFFF	480.5 KiB	Conventional memory
 * 0x00080000	0x0009FFFF	~128 KiB	EBDA
 * 384 KiB		Upper Memory
 * 0x000A0000	0x000BFFFF	128 KiB		Video display memory
 * 0x000C0000	0x000C7FFF	~32 KiB		Video BIOS
 * 0x000C8000	0x000EFFFF	~160 KiB	BIOS Expansions
 * 0x000F0000	0x000FFFFF	64 KiB		Motherboard BIOS
 * > 1 MiB		High Memory
 * 0x00100000	0x00EFFFFF	14 MiB		Extended memory
 * 0x00F00000	0x00FFFFFF	1 MiB		ISA Memory Hole
 */

SECTIONS {

	/* Align sections to allow for page-level runtime protection */
	alignment = 0x1000;

	/* Virtual addresses start at 0x20000 */
	. = 0x20000;
	_start = .;

	/* bootmgr.exe hardcodes the address 0x30000 for use as a
	 * buffer accessible by real-mode code.  We can't fit
	 * everything below this region, so we explicitly verify that
	 * all BIOS-mode code and initialised data fits below this
	 * region, and choose to place .bss above it.
	 */
	_forbidden_start = 0x30000;
	_forbidden_end = 0x40000;

	/* Extended BIOS Data Area */
	_ebda_start = 0x80000;

	/* bzImage prefix */
	_prefix_pos = 0;
	.prefix : AT ( _prefix_pos ) {
		_prefix = .;
		*(.prefix)
		*(.prefix.*)
		_eprefix = .;
	}
	_prefix_len = ABSOLUTE ( _eprefix ) - ABSOLUTE ( _prefix );

	/* Real-mode uninitialised data section */
	.bss16 ( NOLOAD ) : {
		_bss16 = .;
		*(.stack16)
		*(.stack16.*)
		*(.bss16)
		*(.bss16.*)
		. = ALIGN ( alignment );
		_ebss16 = .;
	}
	_bss16_len = ABSOLUTE ( _ebss16 ) - ABSOLUTE ( _bss16 );

	/* Data (and 16-bit BIOS text) section */
	_payload_pos = ( _prefix_pos + _prefix_len );
	_data_pos = _payload_pos;
	.data : AT ( _data_pos ) {
		_data = .;
		_payload = .;
		/* Portions that must be accessible in 16-bit modes */
		*(.text16)
		*(.text16.*)
		*(.rodata16)
		*(.rodata16.*)
		*(.data16)
		*(.data16.*)
		/* Portions that need not be accessible in 16-bit modes */
		*(.rodata)
		*(.rodata.*)
		*(.data)
		*(.data.*)
		*(.got)
		*(.got.*)
		. = ALIGN ( alignment );
		_edata = .;
	}
	_data_len = ABSOLUTE ( _edata ) - ABSOLUTE ( _data );

	/* Text (excluding 16-bit BIOS text) section */
	_text_pos = ( _data_pos + _data_len );
	.text : AT ( _text_pos ) {
		_text = .;
		*.i386.*(.text)
		*.i386.*(.text.*)
		ASSERT ( ABSOLUTE ( . ) <= ABSOLUTE ( _forbidden_start ),
			 "Binary is too large (overlap the bootmgr buffer)" );
		*(.text)
		*(.text.*)
		. = ALIGN ( alignment );
		_epayload = .;
		_etext = .;
		ASSERT ( ABSOLUTE ( . ) <= ABSOLUTE ( _ebda_start ),
			 "Binary is too large (overlap the EBDA)" );
	}
	_text_len = ABSOLUTE ( _etext ) - ABSOLUTE ( _text );
	_payload_len = ABSOLUTE ( _epayload ) - ABSOLUTE ( _payload );

	/* Uninitialised data section */
	.bss ( NOLOAD ) : {
		_bss = .;
		. = ABSOLUTE ( _forbidden_end );
		*(.bss)
		*(.bss.*)
		*(COMMON)
		*(.stack)
		*(.stack.*)
		. = ALIGN ( alignment );
		_ebss = .;
		ASSERT ( ABSOLUTE ( . ) <= ABSOLUTE ( _ebda_start ),
			 "Binary is too large (overlap the EBDA)" );
	}
	_bss_len = ABSOLUTE ( _ebss ) - ABSOLUTE ( _bss );

	/* Secure Boot Advanced Targeting (SBAT) section */
	_sbat_pos = ( _text_pos + _text_len );
	.sbat : AT ( _sbat_pos ) {
		_sbat = .;
		*(.sbat)
		*(.sbat.*)
		_esbat = .;
		ASSERT ( ABSOLUTE ( . ) <= ABSOLUTE ( _ebda_start ),
			 "Binary is too large (overlap the EBDA)" );
	}
	_sbat_len = ABSOLUTE ( _esbat ) - ABSOLUTE ( _sbat );

	_end = .;

	/* Symbols required by i386.x86_64 objects */
	__i386__start = _start;
	__i386__end = _end;

	/DISCARD/ : {
		*(.comment)
		*(.comment.*)
		*(.note)
		*(.note.*)
		*(.eh_frame)
		*(.eh_frame.*)
		*(.rel)
		*(.rel.*)
	}
}
