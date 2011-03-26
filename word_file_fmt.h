#ifndef _WORD_FILE_FMT_H
#define _WORD_FILE_FMT_H

#include <stdint.h>

/* Header file defining common data structures of MS Word for DOS file format
 * (C) 2011 by Harald Welte <laforge@gnumonks.org>
 * 
 * largely based on http://www.msxnet.org/word2rtf/formats/ffh-dosword5
 * Chapter 16 of Dr. Dobbs File Format Handbook */

struct word_file_hdr {
	uint8_t		magic[4];
	uint8_t		reserved[8];
	uint16_t	reserved2;
	uint32_t	ptr_end_of_text;
	uint16_t	bptr_fmt_para;
	uint16_t	bptr_footnote;
	uint16_t	bptr_fmt_sect;
	uint16_t	bptr_nation;
	uint16_t	bptr_page_breaks;
	uint16_t	bptr_file_mgr_info;
	uint8_t		print_format[66];
	uint16_t	win_write_flag;
	uint8_t		printer_drvr[8];
	uint16_t	num_blks_used;
	uint16_t	corrected_text;
	uint8_t		reserved3[18];
} __attribute__ ((packed));

struct word_fmt_entry {
	uint32_t	ptr_text;	/* pointer to first character in different fmt */
	uint16_t	offset_fmt;	/* pointer to format definition */
} __attribute__ ((packed));

struct word_char_fmt {
	uint8_t		length;
	uint8_t		coding_print_tmpl;
	uint8_t		fmt_code;	/* format code */
	uint8_t		font_size;	/* 1/2 point */
	uint8_t		char_attr;	/* character attributes */
	uint8_t		reserved;
	uint8_t		char_pos;	/* superscript, subscript, ... */
} __attribute__ ((packed));

struct word_par_fmt {
	uint8_t		length;
	uint8_t		fmt_code;
	uint8_t		par_align:2,
			par_same_page:1,
			par_next_same_page:1,
			par_two_columns:1,
			par_reserved:3;
	uint8_t		std_par_fmt;
	uint8_t		heading_lvl;
	uint16_t	indent_right;
	uint16_t	indent_left;
	uint16_t	indent_left_first;
	uint16_t	line_space;
	uint16_t	heading_space;
	uint16_t	end_space;
	uint8_t		frame_lines:2,
			frame_type:2,
			hdr_ftr_1st_page:1,
			hdr_ftr_even_page:1,
			hdr_ftr_odd_page:1,
			footer:1,
			header:1;
	uint32_t	line_position;
	uint32_t	reserved;
	uint8_t		tab_descr[80];
} __attribute__ ((packed));

extern const uint8_t word_file_magic[];

#define WORD_BLOCK_SIZE		0x80

#endif /* _WORD_FILE_FMT_H */
