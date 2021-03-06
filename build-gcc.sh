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
if [ -z "${OBJCOPY}" ]; then
	OBJCOPY=objcopy
fi
if [ -z "${STRIP}" ]; then
	STRIP=strip
fi
if [ -z "${RC}" ]; then
	RC=windres
fi
if [ ! -z "${TARGET}" ]; then
	CC=${TARGET}-${CC}
	OBJCOPY=${TARGET}-${OBJCOPY}
	STRIP=${TARGET}-${STRIP}
	RC=${TARGET}-${RC}
fi

CFLAGS="-O2 -Wall -Wextra -std=c99"
LDFLAGS="-lm"
EXE=hack-detection
DEBUG=hack-detection.debug
if [ "${ISWIN}" == "1" ]; then
	LDFLAGS="${LDFLAGS} -municode"
	EXE="${EXE}.exe"
fi

set -e
if [ "${ISWIN}" == "1" ]; then
	# Compile in multiple steps, since we have a resource file.
	echo ${CC} -c ${CFLAGS} -o hack-detection.o hack-detection.c
	${CC} -c ${CFLAGS} -o hack-detection.o hack-detection.c
	echo ${RC} resource.rc resource.o
	${RC} resource.rc resource.o
	echo ${CC} ${CFLAGS} ${LDFLAGS} -o ${EXE} hack-detection.o resource.o
	${CC} ${CFLAGS} ${LDFLAGS} -o ${EXE} hack-detection.o resource.o
else
	# Compile and link in one step.
	echo ${CC} ${CFLAGS} -o ${EXE} hack-detection.c ${LDFLAGS}
	${CC} ${CFLAGS} -o ${EXE} hack-detection.c ${LDFLAGS}
fi

set -v
${OBJCOPY} --only-keep-debug ${EXE} ${DEBUG}
${STRIP} ${EXE}
${OBJCOPY} --add-gnu-debuglink=${DEBUG} ${EXE}
