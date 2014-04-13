TEST_SRCS = $(wildcard tests/*.c)
TEST_TARGETS = $(TEST_SRCS:.c=)

all: liblv2_test.a

liblv2_test.a: lv2_test.c audio_io.c
	gcc -std=c99 -c `pkg-config --cflags lv2core lilv-0` -o lv2_test.o lv2_test.c
	gcc -std=c99 -c `pkg-config --cflags gstreamer-0.10` -lm -o audio_io.o audio_io.c
	ar rcs liblv2_test.a lv2_test.o audio_io.o

$(TEST_TARGETS): tests/% : tests/%.c
	gcc -std=c99 $< liblv2_test.a `pkg-config --libs lilv-0 aubio gio-2.0 gstreamer-0.10` -lm -o $@

tests: liblv2_test.a $(TEST_TARGETS)

run_tests: tests
	for target in $(TEST_TARGETS); do echo $${target}; $${target}; done;

clean:
	rm -rf *.o *.a $(TEST_TARGETS)
