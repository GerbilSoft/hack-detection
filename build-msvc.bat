set "CFLAGS=/nologo /O2 /MT /EHsc /Zi /W3"
set "RCFLAGS=/nologo"
set "LDFLAGS=/nologo /debug /opt:icf,ref /incremental:no"
CL /c %CFLAGS% /Fohack-detection.o hack-detection.c
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
RC %RCFLAGS% /foresource.res resource.rc
@IF ERRORLEVEL 1 EXIT /B %ERRORLEVEL%
LINK hack-detection.o resource.res %LDFLAGS%
