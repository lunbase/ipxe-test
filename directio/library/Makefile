#

CFLAGS=-O0 -g -I../driver/directio
LDFLAGS=-shared -Wl,--add-stdcall-alias

CC=gcc
RC=windres

all: directio.dll

directiolib.o: directiolib.c directiolib.h
	$(CC) -c -o $@ $(CFLAGS) $<

directio.dll: directiolib.o directiolib_rc.o
	$(CC) $^ $(LDFLAGS) -o $@ -Wl,--out-implib,$@.a

directiolib_rc.o: directiolib.rc
	windres -o $@ $<

clean:
	rm -f *.o *.dll *.a
