CFLAGS += -Os -Wunused

build: qr

clean:
	$(RM) qr qr_gen.o qr.o main.o bitfield.o

qr: qr_gen.o qr.o main.o bitfield.o
	$(CC) -pie -o $@ $^ -lm -lpng -lz -ljpeg

qr_gen.o: qr_gen.c qr.h
	$(CC) $(CFLAGS) -c qr_gen.c


qr.o: qr.c qr.h
	$(CC) $(CFLAGS) -c qr.c

bitfield.o: bitfield.c
	$(CC) -c bitfield.c

main.o: main.c qr.h
	$(CC) $(CFLAGS) -c main.c

.PHONY: clean build