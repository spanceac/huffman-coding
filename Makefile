all: huf-encode-file huf-decode-file

huf-encode-file: huf-encode-file.c
	gcc -Wall -g -o huf-encode-file huf-encode-file.c

huf-decode-file: huf-decode-file.c
	gcc -Wall -g -o huf-decode-file huf-decode-file.c
