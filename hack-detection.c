/****************************************************************************
 * HACK DETECTION! HACK DETECTION! HACK DETECTION? HACK DETECTION!          *
 *                                                                          *
 * Copyright (c) 2017 by David Korth.                                       *
 *                                                                          *
 * This program is free software: you can redistribute it and/or modify     *
 * it under the terms of the GNU Affero General Public License as           *
 * published by the Free Software Foundation, either version 3 of the       *
 * License, or (at your option) any later version.                          *
 *                                                                          *
 * This program is distributed in the hope that it will be useful,          *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 * GNU Affero General Public License for more details.                      *
 *                                                                          *
 * You should have received a copy of the GNU Affero General Public License *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
 ****************************************************************************/

#ifdef _MSC_VER
// MSVC: Use 64-bit file offset functions.
# define fseek _fseeki64
# define ftell _ftelli64
#else
// MinGW/Unix: Use Large File Support if necessary.
# define _LARGEFILE_SOURCE 1
# define _LARGEFILE64_SOURCE 1
# define _FILE_OFFSET_BITS 64
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

int main(int argc, char *argv[])
{
	// Allocated memory.
	FILE *f_src = NULL, *f_hack = NULL;
	uint8_t *p_src = NULL, *p_hack = NULL;

	// Sizes.
	uint32_t sz_src, sz_hack;
	int64_t sz64_tmp;
	size_t sz;
	uint32_t sz_check;	// Size to check, after removing padding from the source ROM.
	uint32_t sz_common;	// Number of bytes the two ROMs have in common.
	int hack_detection;	// Non-zero if a binary hack is detected.

	// Temporary pointers within p_src/p_hack.
	const uint8_t *pc_src, *pc_hack;

	int ret = EXIT_FAILURE;

	if (argc >= 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
		// Usage information.
		fprintf(stderr,
			"HACK DETECTION!\n"
			"Check if a hacked ROM is a hex-edited hack.\n"
			"\n"
			"Syntax: %s [source ROM] [hacked ROM]\n"
			"- [source ROM]: The original ROM image.\n"
			"- [hacked ROM]: The hacked ROM image.\n"
			"\n"
			"Both ROM images must be at least 32 KB and less than 16 MB.\n",
			argv[0]);
		return EXIT_SUCCESS;
	}

	if (argc != 3) {
		fprintf(stderr, "Syntax: %s [source ROM] [hacked ROM]\n"
			"Try '%s --help' for more information.\n", argv[0], argv[0]);
		return EXIT_FAILURE;
	}

	// Open the source ROM.
	f_src = fopen(argv[1], "rb");
	if (!f_src) {
		fprintf(stderr, "*** ERROR opening source ROM '%s': %s\n", argv[1], strerror(errno));
		goto cleanup;
	}
	fseek(f_src, 0, SEEK_END);
	sz64_tmp = ftell(f_src);
	if (sz64_tmp < 0) {
		fprintf(stderr, "*** ERROR: Unable to determine the size of source ROM '%s'.\n", argv[1]);
		goto cleanup;
	} else if (sz64_tmp == 0) {
		fprintf(stderr, "*** ERROR: Source ROM '%s' is empty. (0 bytes)\n", argv[1]);
		goto cleanup;
	} else if (sz64_tmp < 32*1024) {
		fprintf(stderr, "*** ERROR: Source ROM '%s' is smaller than 32 KB.\n", argv[1]);
		goto cleanup;
	} else if (sz64_tmp > 16*1024*1024) {
		fprintf(stderr, "*** ERROR: Source ROM '%s' is larger than 16 MB.\n", argv[1]);
		goto cleanup;
	}
	sz_src = (uint32_t)sz64_tmp;

	// Open the hacked ROM.
	f_hack = fopen(argv[2], "rb");
	if (!f_hack) {
		fprintf(stderr, "*** ERROR opening hacked ROM '%s': %s\n", argv[2], strerror(errno));
		goto cleanup;
	}
	fseek(f_hack, 0, SEEK_END);
	sz64_tmp = ftell(f_hack);
	if (sz64_tmp < 0) {
		fprintf(stderr, "*** ERROR: Unable to determine the size of hacked ROM '%s'.\n", argv[2]);
		goto cleanup;
	} else if (sz64_tmp == 0) {
		fprintf(stderr, "*** ERROR: Hacked ROM '%s' is empty. (0 bytes)\n", argv[2]);
		goto cleanup;
	} else if (sz64_tmp < 32*1024) {
		fprintf(stderr, "*** ERROR: Hacked ROM '%s' is smaller than 32 KB.\n", argv[2]);
		goto cleanup;
	} else if (sz64_tmp > 16*1024*1024) {
		fprintf(stderr, "*** ERROR: Hacked ROM '%s' is larger than 16 MB.\n", argv[2]);
		goto cleanup;
	}
	sz_hack = (uint32_t)sz64_tmp;

	// Load the source ROM into memory.
	p_src = malloc(sz_src);
	if (!p_src) {
		fprintf(stderr, "*** ERROR: Unable to allocate %u bytes for source ROM '%s'.\n", sz_src, argv[1]);
		goto cleanup;
	}
	fseek(f_src, 0, SEEK_SET);
	sz = fread(p_src, 1, sz_src, f_src);
	if (sz != sz_src) {
		fprintf(stderr, "*** ERROR: Unable to read source ROM '%s' into memory.\n", argv[1]);
		goto cleanup;
	}

	// Load the hacked ROM into memory.
	p_hack = malloc(sz_hack);
	if (!p_hack) {
		fprintf(stderr, "*** ERROR: Unable to allocate %u bytes for hacked ROM '%s'.\n", sz_hack, argv[2]);
		goto cleanup;
	}
	fseek(f_hack, 0, SEEK_SET);
	sz = fread(p_hack, 1, sz_hack, f_hack);
	if (sz != sz_hack) {
		fprintf(stderr, "*** ERROR: Unable to read hacked ROM '%s' into memory.\n", argv[2]);
		goto cleanup;
	}

	// Find the end of the source ROM.
	// ROM images are typically rounded up to the nearest power of two,
	// so any remaining data is filled with 0xFF or 0x00.
	if (p_src[sz_src-1] == 0x00 || p_src[sz_src-1] == 0xFF) {
		const uint8_t pad_byte = p_src[sz_src-1];
		const uint8_t *p = &p_src[sz_src-2];
		while (p >= p_src && *p == pad_byte) {
			p--;
		}
		if (p < p_src) {
			fprintf(stderr, "*** ERROR: Source ROM '%s' is all padding. (all bytes are 0x%02X)\n", argv[1], pad_byte);
			goto cleanup;
		}
		sz_check = p - p_src;	// TODO: Verify that this isn't off by one.
		printf("Source ROM: %s (%u bytes, %u less padding)\n", argv[1], sz_src, sz_check);
	} else {
		// No padding.
		sz_check = sz_src;
		printf("Source ROM: %s (%u bytes)\n", argv[1], sz_src);
	}
	printf("Hacked ROM: %s (%u bytes)\n\n", argv[2], sz_hack);

	if (sz_hack < sz_check) {
		// Hacked ROM is smaller than the source ROM, less padding.
		// TODO: Check for padding in the hacked ROM?
		sz_check = sz_hack;
	}

	printf("Checking %u bytes...\n", sz_check);
	pc_src = p_src;
	pc_hack = p_hack;
	sz_common = 0;
	for (uint32_t i = sz_check; i > 0; i--, pc_src++, pc_hack++) {
		if (*pc_src == *pc_hack) {
			sz_common++;
		}
	}

	// If more than 50% of the bytes are common,
	// this is probably a binary hack.
	hack_detection = (sz_common >= (sz_check / 2));
	if (hack_detection) {
		// TODO: Colorize "* HACK DETECTION *" on Windows and Linux.
		printf("\n*** HACK DETECTION ***\n\n");
	} else {
		printf("\n");
	}

	printf("%u of %u bytes (%0.1f%%) are common between both the\n"
		"source ROM and the hacked ROM.\n\n",
		sz_common, sz_check, (((double)sz_common / (double)sz_check) * 100.0));
	if (hack_detection) {
		printf("The hacked ROM is most likely a hex-edited binary hack.\n");
	} else {
		printf("The hacked ROM is either a disassembly hack or not related\n"
			"to the source ROM at all.\n");
	}
	// TODO: Separate return codes for "not hacked", "HACK DETECTION", and "error"?
	ret = 0;

cleanup:
	if (f_src) {
		fclose(f_src);
	}
	if (f_hack) {
		fclose(f_hack);
	}
	free(p_src);
	free(p_hack);
	return ret;
}
