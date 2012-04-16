CFLAGS=-O2 -Wall -Wextra -c
LDFLAGS=

bin/vkconcli: obj/main.o obj/md5.o
	cc ${LDFLAGS} obj/main.o obj/md5.o -o bin/vkconcli

obj/main.o: src/main.c
	cc ${CFLAGS} src/main.c -o obj/main.o

obj/md5.o: src/md5.c
	cc ${CFLAGS} src/md5.c -o obj/md5.o
