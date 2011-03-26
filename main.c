/* This program can extract the raw ASN.1 source from the MS Word for DOS
 * file of the MAP ASN.1 spec, such as 380-6.DOC which is part of
 * http://ftp.3gpp.org/specs/archive/09_series/09.02/0902-380.zip */

/* (C) 2011 by Harald Welte <laforge@gnumonks.org> */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "word_util.h"

static int handle_par_fmt_desc(struct word_handle *wh, struct word_par_fmt *pfmt,
				uint32_t start_offs, uint32_t next_offs)
{
	fprintf(stderr, "Paragraph format (0x%08x-0x%08x):\n", start_offs, next_offs);
	fprintf(stderr, "\tFormat length: %u\n", pfmt->length);
	fprintf(stderr, "\tFormat code: 0x%02x\n", pfmt->fmt_code);
	fprintf(stderr, "\tAlignment: %d\n", pfmt->par_align);
	fprintf(stderr, "\tStd Par Fmt: 0x%02x\n", pfmt->std_par_fmt);

	if (pfmt->fmt_code == 0x4c && pfmt->std_par_fmt == 0x26) {
		char *tmp = strndup(wh->base_addr + start_offs, next_offs-start_offs);
		if (tmp) {
			fprintf(stdout, tmp);
			//fprintf(stderr, tmp);
			free(tmp);
		}
	}

	return 0;
}

static void handle_fmt_block(struct word_handle *wh, uint16_t block_nr)
{
	uint8_t *block_base = ((uint8_t *)wh->base_addr) + word_bptr2offset(block_nr);
	uint32_t offset = *((uint32_t *)block_base);
	uint32_t offset_next = *(uint32_t *)(block_base+WORD_BLOCK_SIZE);
	uint32_t num_fmts = *(block_base + 0x7f);
	struct word_fmt_entry *fmt_tbl = block_base + 4;
	struct word_par_fmt *pfmt;
	uint32_t last_fmt_start = offset;
	int i;
	
	fprintf(stderr, "Format block %u\n", block_nr);
	fprintf(stderr, "Offset of first Paragraph: %u (0x%x)\n", offset, offset);
	fprintf(stderr, "Number of format table entries: %u\n", num_fmts);

	for (i = 0; i < num_fmts; i++) {
		if (i == num_fmts -1) {
			/* in the last entry, check if there is another block */
			if (fmt_tbl[i].ptr_text == offset_next) {
				handle_fmt_block(wh, block_nr+1);
				continue;
			}
		}
		fprintf(stderr, "Format tbl entry Text Ptr: %u (0x%x), Fmt: %u\n",
			fmt_tbl[i].ptr_text, fmt_tbl[i].ptr_text, fmt_tbl[i].offset_fmt);
		if (fmt_tbl[i].offset_fmt != 0xffff) {
			pfmt = block_base + 4 + fmt_tbl[i].offset_fmt;
		 	handle_par_fmt_desc(wh, pfmt, last_fmt_start, fmt_tbl[i].ptr_text);
		}
		last_fmt_start = fmt_tbl[i].ptr_text;
	}
}

static void process(struct word_handle *wh)
{
	struct word_file_hdr *wfh = wh->base_addr;

	fprintf(stderr, "Word file size: %u\n", wh->file_size);
	fprintf(stderr, "Paragraph fmt Block PTR: %u, offset = %u\n", wfh->bptr_fmt_para,
		word_bptr2offset(wfh->bptr_fmt_para));
	
	handle_fmt_block(wh, wfh->bptr_fmt_para);
}

int main(int argc, char **argv)
{
	struct word_handle *wh;

	if (argc < 2) {
		fprintf(stderr, "You need to specify the file name of the DOC file\n");
		exit(2);
	}

	wh = word_file_open(argv[1]);
	if (!wh)
		exit(1);

	process(wh);
}
