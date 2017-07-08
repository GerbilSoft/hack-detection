set "CFLAGS=/O2 /MD /EHsc /Zi /W3"
set "LDFLAGS=/debug /opt:icf,ref /incremental:no"
CL %CFLAGS% /Fehack-detection.exe hack-detection.c /link %LDFLAGS%
