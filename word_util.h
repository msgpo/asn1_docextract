#ifndef _WORD_UTIL_H
#define _WORD_UTIL_H

#include <stdint.h>

#include "word_file_fmt.h"

struct word_handle {
	int fd;
	uint8_t *base_addr;
	uint32_t file_size;
};

static inline uint32_t word_bptr2offset(uint16_t bptr)
{
	return (bptr * WORD_BLOCK_SIZE);
}

struct word_handle *word_file_open(const char *fname);

#endif
