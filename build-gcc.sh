#!/bin/sh
if [ -z "${ISWIN}" ]; then
	ISWIN=0
	case "$OSTYPE" in
		cygwin)
			ISWIN=1
			;;
		msys)
			ISWIN=1
			;;
		win32)
			ISWIN=1
			;;
		*)
			;;
	esac
fi
if [ ! -z "${TARGET}" ]; then
	CC=${TARGET}-gcc
	OBJCOPY=${TARGET}-objcopy
	STRIP=${TARGET}-strip
else
	CC=gcc
	OBJCOPY=objcopy
	STRIP=strip
fi

CFLAGS="-O2 -Wall -Wextra -std=c99"
LDFLAGS="-lm"
EXE=hack-detection
DEBUG=hack-detection.debug
if [ "${ISWIN}" == "1" ]; then
	LDFLAGS="${LDFLAGS} -municode"
	EXE="${EXE}.exe"
fi

echo CC=${CC}
echo CFLAGS=${CFLAGS}
echo LDFLAGS=${LDFLAGS}
echo EXE=${EXE}
echo DEBUG=${DEBUG}
set -e
set -v
${CC} ${CFLAGS} -o ${EXE} hack-detection.c ${LDFLAGS}
${OBJCOPY} --only-keep-debug ${EXE} ${DEBUG}
${STRIP} ${EXE}
${OBJCOPY} --add-gnu-debuglink=${DEBUG} ${EXE}
