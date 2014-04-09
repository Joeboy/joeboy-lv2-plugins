all: example

liblv2_test.a: lv2_test.c
	gcc -std=c99 -c `pkg-config --cflags lv2core lilv-0` -o lv2_test.o lv2_test.c
	ar rcs liblv2_test.a lv2_test.o

example: liblv2_test.a example.c
	gcc -std=c99 example.c liblv2_test.a `pkg-config --libs lilv-0 aubio` -o example

clean:
	rm -rf *.o *.a example
