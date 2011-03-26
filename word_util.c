#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "word_file_fmt.h"
#include "word_util.h"

struct word_handle *word_file_open(const char *fname)
{
	int fd;
	struct stat st;
	struct word_handle *wh = calloc(1, sizeof(*wh));

	fd = open(fname, O_RDONLY);
	if (fd < 0) {
		free(wh);
		return NULL;
	}
	wh->fd = fd;

	fstat(fd, &st);
	wh->file_size = st.st_size;

	wh->base_addr = mmap(NULL, wh->file_size, PROT_READ, MAP_SHARED, wh->fd, 0);
	if (!wh->base_addr) {
		close(wh->fd);
		free(wh);
		return NULL;
	}

	return wh;
}
