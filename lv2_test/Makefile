TEST_SRCS = $(wildcard tests/*.c)
TEST_TARGETS = $(TEST_SRCS:.c=)

all: liblv2_test.a $(TEST_TARGETS)

$(TEST_TARGETS): tests/% : tests/%.c
	gcc -std=c99 $< liblv2_test.a `pkg-config --libs lilv-0 aubio` -o $@

liblv2_test.a: lv2_test.c
	gcc -std=c99 -c `pkg-config --cflags lv2core lilv-0` -o lv2_test.o lv2_test.c
	ar rcs liblv2_test.a lv2_test.o

clean:
	rm -rf *.o *.a $(TEST_TARGETS)
