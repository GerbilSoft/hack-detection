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
if [ -z "${CC}" ]; then
	CC=gcc
fi

CFLAGS="-O2 -Wall -Wextra -std=c99"
EXE=hack-detection
if [ "${ISWIN}" == "1" ]; then
	EXE=hack-detection.exe
fi

echo CC=${CC}
echo CFLAGS=${CFLAGS}
echo EXE=${EXE}
set -v
${CC} ${CFLAGS} -o ${EXE} hack-detection.c
