sigrok2ega: sigrok2ega.c
	$(CC) -O2 -W -Wall sigrok2ega.c -o sigrok2ega -lSDL2

test: sigrok2ega
	cat data/keen5b.bin.gz | gunzip | ./sigrok2ega

clean:
	$(RM) sigrok2ega
