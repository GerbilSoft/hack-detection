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

#ifdef _WIN32
// Enable Unicode functions.
#define UNICODE 1
#define _UNICODE 1
#else
// Enable POSIX functions. [fileno()]
#define _POSIX_C_SOURCE 1
#endif

#ifdef _MSC_VER
// MSVC: Disable "secure" CRT warnings.
#define _CRT_SECURE_NO_WARNINGS 1
#define _CRT_SECURE_NO_DEPRECATE 1
#define _CRT_NONSTDC_NO_DEPRECATE 1
#else /* !_MSC_VER */
// MinGW/Unix: Use Large File Support if necessary.
# define _LARGEFILE_SOURCE 1
# define _LARGEFILE64_SOURCE 1
# define _FILE_OFFSET_BITS 64
#endif

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
// Windows headers are needed for console attributes.
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#define FOREGROUND_WHITE (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#define FOREGROUND_YELLOW (FOREGROUND_RED | FOREGROUND_GREEN)
#else /* !_WIN32 */
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#ifdef _MSC_VER
// MSVC: Use 64-bit file offset functions.
# define fseek _fseeki64
# define ftell _ftelli64
#endif

// TCHAR definitions for non-Windows systems.
#include "tchar_xplt.h"

/**
 * Is this a valid TTY?
 * @return 0 if not a TTY; non-zero if it is.
 */
static int is_a_tty(void)
{
	// Check if the output stream supports color.
	// (Based on Google Test.)
#ifdef _WIN32
	// FIXME: _isatty() might not work properly on Win8+ with MinGW.
	// Reference: https://lists.gnu.org/archive/html/bug-gnulib/2013-01/msg00007.html
	if (_isatty(_fileno(stdout))) {
		// This is a TTY.
		return 1;
	}
#else /* !_WIN32 */
	if (isatty(fileno(stdout))) {
		// On Unix/Linux, check the terminal
		// to see if it actually supports color.
		const char *const term = getenv("TERM");
		if (!term)
			return 0;

		if (!strcmp(term, "xterm") ||
		    !strcmp(term, "xterm-color") ||
		    !strcmp(term, "xterm-256color") ||
		    !strcmp(term, "screen") ||
		    !strcmp(term, "screen-256color") ||
		    !strcmp(term, "linux") ||
		    !strcmp(term, "cygwin"))
		{
			// Terminal supports color.
			return 1;
		}
	}
#endif /* _WIN32 */

	// No colors...
	return 0;
}

/**
 * Print the "HACK DETECTION" banner.
 */
static void print_hack_detection(void)
{
	int lbside, rbside;
#ifdef _WIN32
	// Needed because MSVC prior to 2015 doesn't support C99.
	HANDLE hStdOut;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	WORD old_color_attrs;
	BOOL bIsEnd = FALSE;
#endif /* _WIN32 */

	// Is this a TTY?
	if (!is_a_tty()) {
		// Not a TTY. Print a generic banner.
		_tprintf(_T("*** HACK DETECTION ***\n"));
		return;
	}

#ifdef _WIN32
	// Get the window size.
	hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (!hStdOut || hStdOut == INVALID_HANDLE_VALUE ||
	    !GetConsoleScreenBufferInfo(hStdOut, &csbi) ||
	    csbi.dwSize.X < 4 || csbi.dwSize.Y < 24)
	{
		// Failed to get the console screen buffer info.
		// Print a generic banner.
		_tprintf(_T("*** HACK DETECTION ***\n"));
	}

	// Print "*** HACK DETECTION ***".
	// - Background: Dark green
	// - Asterisks: Bright white
	// - Text: Bright yellow
	old_color_attrs = csbi.wAttributes;
	lbside = (csbi.dwSize.X - 22) / 2;
	rbside = lbside + (csbi.dwSize.X % 2);
	SetConsoleTextAttribute(hStdOut, BACKGROUND_GREEN | FOREGROUND_WHITE | FOREGROUND_INTENSITY);
	_tprintf(_T("%*s%*s*** "), csbi.dwSize.X, "", lbside, "");
	SetConsoleTextAttribute(hStdOut, BACKGROUND_GREEN | FOREGROUND_YELLOW | FOREGROUND_INTENSITY);
	_tprintf(_T("HACK DETECTION"));
	SetConsoleTextAttribute(hStdOut, BACKGROUND_GREEN | FOREGROUND_WHITE | FOREGROUND_INTENSITY);
	// NOTE: If printing past the end of the console buffer, we have to
	// subtract 1 from csbi.dwSize.X in order to not print an extra line
	// with a green background.
	if (GetConsoleScreenBufferInfo(hStdOut, &csbi) &&
	    (csbi.dwCursorPosition.Y + 1) >= csbi.dwSize.Y)
	{
		csbi.dwSize.X--;
		bIsEnd = TRUE;
	}
	_tprintf(_T(" ***%*s%*s"), rbside, "", csbi.dwSize.X, "");
	SetConsoleTextAttribute(hStdOut, old_color_attrs);
	// We need to print a newline here if we're at the end of the buffer.
	if (bIsEnd) {
		_tprintf(_T("\n"));
	}
#else /* !_WIN32 */
	// Linux terminal. Use ANSI escape sequences.

	// Get the terminal size.
	struct winsize sz;
	if (ioctl(0, TIOCGWINSZ, &sz) != 0 ||
	    sz.ws_row < 4 || sz.ws_col < 24)
	{
		// Could not get the terminal size,
		// or the terminal is too small.
		// Fall back to the standard banner.
		_tprintf(_T("*** HACK DETECTION ***\n"));
		return;
	}

	// Print "*** HACK DETECTION ***".
	// - Background: Dark green
	// - Asterisks: Bright white
	// - Text: Bright yellow
	lbside = (sz.ws_col - 22) / 2;
	rbside = lbside + (sz.ws_col % 2);
	_tprintf(_T("\x1B[0m")
		_T("\x1B[37;1m\x1B[42m%*s")
		_T("%*s*** \x1B[33;1mHACK DETECTION\x1B[37;1m ***%*s")
		_T("%*s")
		_T("\x1B[0m\n")
		, sz.ws_col, "", lbside, "", rbside, "", sz.ws_col, "");
#endif
}

int _tmain(int argc, TCHAR *argv[])
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
	int pct;		// Percentage, times 10.
	int hack_detection;	// Non-zero if a binary hack is detected.

	// Temporary pointers within p_src/p_hack.
	const uint8_t *pc_src, *pc_hack;

	// Other temporaries.
	uint32_t ui32;
	int ret = EXIT_FAILURE;

	if (argc >= 2 && (!_tcscmp(argv[1], _T("-h")) || !_tcscmp(argv[1], _T("--help")))) {
		// Usage information.
		_ftprintf(stderr,
			_T("*** HACK DETECTION ***\n")
			_T("Check if a hacked ROM is a hex-edited binary hack.\n")
			_T("\n")
			_T("Copyright (c) 2017 by David Korth.\n")
			_T("Licensed under the GNU AGPLv3 or later.\n")
			_T("https://github.com/GerbilSoft/hack-detection\n")
			_T("\n")
			_T("Syntax: %s [source ROM] [hacked ROM]\n")
			_T("- [source ROM]: The original ROM image.\n")
			_T("- [hacked ROM]: The hacked ROM image.\n")
			_T("\n")
			_T("Both ROM images must be at least 32 KB and less than 16 MB.\n"),
			argv[0]);
		return EXIT_SUCCESS;
	}

	if (argc != 3) {
		_ftprintf(stderr, _T("Syntax: %s [source ROM] [hacked ROM]\n")
			_T("Try '%s --help' for more information.\n"), argv[0], argv[0]);
		return EXIT_FAILURE;
	}

	// Open the source ROM.
	f_src = _tfopen(argv[1], _T("rb"));
	if (!f_src) {
		_ftprintf(stderr, _T("*** ERROR opening source ROM '%s': %s\n"), argv[1], _tcserror(errno));
		goto cleanup;
	}
	fseek(f_src, 0, SEEK_END);
	sz64_tmp = ftell(f_src);
	if (sz64_tmp < 0) {
		_ftprintf(stderr, _T("*** ERROR: Unable to determine the size of source ROM '%s'.\n"), argv[1]);
		goto cleanup;
	} else if (sz64_tmp == 0) {
		_ftprintf(stderr, _T("*** ERROR: Source ROM '%s' is empty. (0 bytes)\n"), argv[1]);
		goto cleanup;
	} else if (sz64_tmp < 32*1024) {
		_ftprintf(stderr, _T("*** ERROR: Source ROM '%s' is smaller than 32 KB.\n"), argv[1]);
		goto cleanup;
	} else if (sz64_tmp > 16*1024*1024) {
		_ftprintf(stderr, _T("*** ERROR: Source ROM '%s' is larger than 16 MB.\n"), argv[1]);
		goto cleanup;
	}
	sz_src = (uint32_t)sz64_tmp;

	// Open the hacked ROM.
	f_hack = _tfopen(argv[2], _T("rb"));
	if (!f_hack) {
		_ftprintf(stderr, _T("*** ERROR opening hacked ROM '%s': %s\n"), argv[2], _tcserror(errno));
		goto cleanup;
	}
	fseek(f_hack, 0, SEEK_END);
	sz64_tmp = ftell(f_hack);
	if (sz64_tmp < 0) {
		_ftprintf(stderr, _T("*** ERROR: Unable to determine the size of hacked ROM '%s'.\n"), argv[2]);
		goto cleanup;
	} else if (sz64_tmp == 0) {
		_ftprintf(stderr, _T("*** ERROR: Hacked ROM '%s' is empty. (0 bytes)\n"), argv[2]);
		goto cleanup;
	} else if (sz64_tmp < 32*1024) {
		_ftprintf(stderr, _T("*** ERROR: Hacked ROM '%s' is smaller than 32 KB.\n"), argv[2]);
		goto cleanup;
	} else if (sz64_tmp > 16*1024*1024) {
		_ftprintf(stderr, _T("*** ERROR: Hacked ROM '%s' is larger than 16 MB.\n"), argv[2]);
		goto cleanup;
	}
	sz_hack = (uint32_t)sz64_tmp;

	// Load the source ROM into memory.
	p_src = malloc(sz_src);
	if (!p_src) {
		_ftprintf(stderr, _T("*** ERROR: Unable to allocate %u bytes for source ROM '%s'.\n"), sz_src, argv[1]);
		goto cleanup;
	}
	fseek(f_src, 0, SEEK_SET);
	sz = fread(p_src, 1, sz_src, f_src);
	if (sz != sz_src) {
		_ftprintf(stderr, _T("*** ERROR: Unable to read source ROM '%s' into memory.\n"), argv[1]);
		goto cleanup;
	}

	// Load the hacked ROM into memory.
	p_hack = malloc(sz_hack);
	if (!p_hack) {
		_ftprintf(stderr, _T("*** ERROR: Unable to allocate %u bytes for hacked ROM '%s'.\n"), sz_hack, argv[2]);
		goto cleanup;
	}
	fseek(f_hack, 0, SEEK_SET);
	sz = fread(p_hack, 1, sz_hack, f_hack);
	if (sz != sz_hack) {
		_ftprintf(stderr, _T("*** ERROR: Unable to read hacked ROM '%s' into memory.\n"), argv[2]);
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
			_ftprintf(stderr, _T("*** ERROR: Source ROM '%s' is all padding. (all bytes are 0x%02X)\n"), argv[1], pad_byte);
			goto cleanup;
		}
		sz_check = p - p_src;	// TODO: Verify that this isn't off by one.
		_tprintf(_T("Source ROM: %s (%u bytes, %u less padding)\n"), argv[1], sz_src, sz_check);
	} else {
		// No padding.
		sz_check = sz_src;
		_tprintf(_T("Source ROM: %s (%u bytes)\n"), argv[1], sz_src);
	}
	_tprintf(_T("Hacked ROM: %s (%u bytes)\n\n"), argv[2], sz_hack);

	if (sz_hack < sz_check) {
		// Hacked ROM is smaller than the source ROM, less padding.
		// TODO: Check for padding in the hacked ROM?
		sz_check = sz_hack;
	}

	_tprintf(_T("Checking %u bytes...\n"), sz_check);
	pc_src = p_src;
	pc_hack = p_hack;
	sz_common = 0;
	for (ui32 = sz_check; ui32 > 0; ui32--, pc_src++, pc_hack++) {
		if (*pc_src == *pc_hack) {
			sz_common++;
		}
	}

	// If more than 50% of the bytes are common,
	// this is probably a binary hack.
	hack_detection = (sz_common >= (sz_check / 2));
	if (hack_detection) {
		_tprintf(_T("\n"));
		print_hack_detection();
		_tprintf(_T("\n"));
	} else {
		_tprintf(_T("\n"));
	}

	if (sz_common == sz_check) {
		// All bytes match.
		pct = 1000;
	} else {
		// Calculate the percentage.
		pct = (int)floor(((double)sz_common / (double)sz_check) * 1000);
	}

	_tprintf(_T("- Checked bytes:   %u\n")
		_T("- Identical bytes: %u\n")
		_T("- Differing bytes: %u\n")
		_T("- Matching percentage: %01d.%01d%%\n\n"),
		sz_check, sz_common, sz_check-sz_common, pct/10, pct%10);
	if (hack_detection) {
		_tprintf(_T("The hacked ROM is most likely a hex-edited binary hack.\n"));
	} else {
		_tprintf(_T("The hacked ROM is either a disassembly hack or not related\n")
			_T("to the source ROM at all.\n"));
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
