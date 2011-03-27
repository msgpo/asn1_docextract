/* This program can extract the raw ASN.1 source from the MS Word for DOS
 * file of the MAP ASN.1 spec, such as 380-6.DOC which is part of
 * http://ftp.3gpp.org/specs/archive/09_series/09.02/0902-380.zip */

/* (C) 2011 by Harald Welte <laforge@gnumonks.org> */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "word_util.h"

enum fmt_block_type {
	FMT_CHARACTER,
	FMT_PARAGRAPH,
	FMT_SECTION,
};

enum extract_mode {
	MODE_3,		/* 3.8.x / 3.9.x / 3.10.x / 3.11.x */
	MODE_4,		/* 4.x.y */
};

static enum extract_mode g_mode = MODE_3;
static int g_type4_out_enable = 0;
static FILE *g_outfile;
static int g_debug = 0;

#define DBG(x, args ...) do {						\
		if (g_debug) fprintf(stderr, x, ## args); } while (0)

static void new_outfile(const char *name)
{
	char *tmp = calloc(1, strlen(name) + strlen(".asn1") + 1);

	strcpy(tmp, name);
	strcat(tmp, ".asn1");

	if (g_outfile != stdout)
		fclose(g_outfile);
	g_outfile = fopen(tmp, "w");
	if (!g_outfile) {
		perror("opening new outfile");
		exit(1);
	}
}

static void output_filter_text(struct word_handle *wh, uint32_t start_offs, uint32_t next_offs)
{
	uint8_t *cur;

	for (cur = wh->base_addr + start_offs; cur < wh->base_addr + next_offs; cur++) {
		switch (*cur) {
		case 0x01:
		case 0x02:
		case 0x03:
		case 0x04:	/* reserved */
		case 0x05:	/* footnote */
		case 0x0c:	/* form feed */
		case 0x1f:	/* hyphenation conditional */
		case 0xc4:	/* hyphenation protected */
		case 0xff:	/* sapce protected */
		case '\r':
			break;
		case 0x0b: /* line feed */
			fputc('\n', g_outfile);
			break;
		case 0x09: /* tab */
		case '\n':
		default:
			fputc(*cur, g_outfile);
		}
	}
}

/* handle paragraph formatting */
static void handle_par_fmt_desc(struct word_handle *wh, struct word_par_fmt *pfmt,
				uint32_t start_offs, uint32_t next_offs)
{
	DBG("Paragraph format (0x%08x-0x%08x):\n", start_offs, next_offs);
	if (!pfmt)
		return;
	DBG("\tFormat length: %u\n", pfmt->length);
	DBG("\tFormat code: 0x%02x\n", pfmt->fmt_code);
	DBG("\tAlignment: %d\n", pfmt->par_align);
	DBG("\tStd Par Fmt: 0x%02x\n", pfmt->std_par_fmt);

	switch (g_mode) {
	case MODE_3:
		/* Detect ASN.1 code based on the special formatting it uses */
		if (pfmt->fmt_code == 0x4c && pfmt->std_par_fmt == 0x26) {
			output_filter_text(wh, start_offs, next_offs);
		}
		break;
	}
}

static void handle_hidden_text(struct word_handle *wh, uint32_t start_offs, uint32_t next_offs)
{
	char *tmp, *found = NULL;
	uint32_t found_delta;

	uint32_t dump_start = 0, dump_end = 0;

restart:
	tmp = strndup(wh->base_addr + start_offs, next_offs - start_offs);

	if (!g_type4_out_enable) {
		/* output is not enable, scan for start / continue */
		found = strstr(tmp, ".$");
		if (found) {
			char *mod_name_tok, *mod_name_tmp;
			found_delta = found - tmp;
			mod_name_tmp = strndup(wh->base_addr + start_offs + found_delta + 2, 80);
			mod_name_tok = strtok(mod_name_tmp, " \r\n");
			if (mod_name_tok && strlen(mod_name_tok) > 1) {
				/* start */
				dump_start = start_offs + found_delta + 2;
				DBG("Found START (0x%x): '%s'\n", dump_start, found);
				new_outfile(mod_name_tok);
				fprintf(g_outfile, "\n-- MODULE '%s' START\n", mod_name_tok);
				free(mod_name_tmp);
			} else {
				/* continuation */
				dump_start = start_offs + found_delta + 2;
				DBG("Found CONT: (0x%x): '%s'\n", dump_start, found);
			}
			g_type4_out_enable = 1;
		}
	} else {
		/* output already enabled */
		dump_start = start_offs;
	}

	if (dump_start > 0) {
		/* scan for interrupt / end */
		int found_end = 0;

		/* default case: dump until end of format section */
		dump_end = next_offs;

		found = strstr(tmp, ".#");
		if (found) {
			found_delta = found - tmp;
			if (strlen(found) > 2 && !strncmp(found, ".#END", 5)) {
				/* end */
				dump_end = start_offs + found_delta;
				DBG("Found END (0x%x): '%s'\n", dump_end, found);
				found_end = 1;
			} else {
				/* interrupt */
				dump_end = start_offs + found_delta;
				DBG("Found INT (0x%x): '%s'\n", dump_end, found);
			}
			g_type4_out_enable = 0;
		}
		output_filter_text(wh, dump_start, dump_end);
		if (found_end)
			fprintf(g_outfile, "\nEND\n-- MODULE END\n");
	}

	free(tmp);

	/* ugly, ugly hack */
	if (dump_start && dump_end < next_offs) {
		start_offs = dump_end+2;
		dump_start = 0; dump_end = 0;
		goto restart;
	}
}

/* handle character formatting */
static void handle_char_fmt_desc(struct word_handle *wh, struct word_char_fmt *cfmt,
				 uint32_t start_offs, uint32_t next_offs)
{
	switch (g_mode) {
	case MODE_4:
		/* Detect ASN.1 code based on the hidden text */
		if (cfmt && cfmt->char_attr & 0x80)
			handle_hidden_text(wh, start_offs, next_offs);
		else {
			if (g_type4_out_enable)
				output_filter_text(wh, start_offs, next_offs);
		}
		break;
	}
}

static void handle_fmt_block(struct word_handle *wh, uint16_t block_nr,
			     enum fmt_block_type type)
{
	uint8_t *block_base = ((uint8_t *)wh->base_addr) + word_bptr2offset(block_nr);
	uint32_t offset = *((uint32_t *)block_base);
	uint32_t offset_next = *(uint32_t *)(block_base+WORD_BLOCK_SIZE);
	uint32_t num_fmts = *(block_base + 0x7f);
	struct word_fmt_entry *fmt_tbl = (struct word_fmt_entry *)(block_base + 4);
	void *pfmt;
	uint32_t last_fmt_start = offset;
	int i;
	
	DBG("Format block %u\n", block_nr);
	DBG("Offset of first Paragraph: %u (0x%x)\n", offset, offset);
	DBG("Number of format table entries: %u\n", num_fmts);

	for (i = 0; i < num_fmts; i++) {
		DBG("Format tbl entry Text Ptr: %u (0x%x), Fmt: %u\n",
			fmt_tbl[i].ptr_text, fmt_tbl[i].ptr_text, fmt_tbl[i].offset_fmt);
		if (fmt_tbl[i].offset_fmt != 0xffff)
			pfmt = block_base + 4 + fmt_tbl[i].offset_fmt;
		else
			pfmt = NULL;
		switch (type) {
		case FMT_PARAGRAPH:
			handle_par_fmt_desc(wh, pfmt, last_fmt_start, fmt_tbl[i].ptr_text);
			break;
		case FMT_CHARACTER:
			handle_char_fmt_desc(wh, pfmt, last_fmt_start, fmt_tbl[i].ptr_text);
			break;
		}
		last_fmt_start = fmt_tbl[i].ptr_text;

		if (i == num_fmts -1) {
			/* in the last entry, check if there is another block */
			if (fmt_tbl[i].ptr_text == offset_next) {
				handle_fmt_block(wh, block_nr+1, type);
			}
		}
	}
}

static void process(struct word_handle *wh)
{
	struct word_file_hdr *wfh = (struct word_file_hdr *) wh->base_addr;
	uint32_t char_fmt_ptr;

	DBG("Word file size: %u\n", wh->file_size);
	DBG("End of text PTR: %u (0x%x)\n", wfh->ptr_end_of_text, wfh->ptr_end_of_text);
	DBG("Paragraph fmt Block PTR: %u, offset = %u\n", wfh->bptr_fmt_para,
		word_bptr2offset(wfh->bptr_fmt_para));

	char_fmt_ptr = wfh->ptr_end_of_text;
	if (char_fmt_ptr % WORD_BLOCK_SIZE)
		char_fmt_ptr = (char_fmt_ptr - (char_fmt_ptr % WORD_BLOCK_SIZE)) + WORD_BLOCK_SIZE;
	
	handle_fmt_block(wh, wfh->bptr_fmt_para, FMT_PARAGRAPH);
	handle_fmt_block(wh, char_fmt_ptr/WORD_BLOCK_SIZE, FMT_CHARACTER);
}

static void usage(char **argv)
{
	fprintf(stderr, "Usage: %s [-3 | -4] [-d] file.doc\n", argv[0]);
}

int main(int argc, char **argv)
{
	struct word_handle *wh;
	int opt;

	g_outfile = stdout;

	while ((opt = getopt(argc, argv, "34do:")) != -1) {
		switch (opt) {
		case '3':
			g_mode = MODE_3;
			break;
		case '4':
			g_mode = MODE_4;
			break;
		case 'd':
			g_debug = 1;
			break;
		case 'o':
			g_outfile = fopen(optarg, "w");
			if (!g_outfile) {
				perror("open outfile");
				exit(1);
			}
			break;
		default:
			usage(argv);
			exit(2);
		}
	}

	if (optind >= argc) {
		usage(argv);
		exit(2);
	}

	DBG("Opening file name '%s'\n", argv[optind]);

	wh = word_file_open(argv[optind]);
	if (!wh)
		exit(1);

	process(wh);
}
