CFLAGS = -O0 -g

map_asn1_extract: main.o word_util.o word_file_fmt.o
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	@rm -f *.o map_asn1_extract
