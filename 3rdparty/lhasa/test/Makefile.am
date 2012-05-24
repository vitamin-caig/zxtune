
AM_CFLAGS=$(TEST_CFLAGS) -I$(top_builddir)/lib/public -I$(top_builddir) -g
LDADD=$(top_builddir)/lib/liblhasatest.a libtestframework.a

COMPILED_TESTS=                       \
	test-crc16                    \
	test-basic-reader             \
	test-decoder

UNCOMPILED_TESTS=                     \
	test-decompress               \
	test-file-headers             \
	test-list-output              \
	test-crc-output               \
	test-dry-run                  \
	test-extract

EXTRA_DIST=                           \
	archives                      \
	compressed                    \
	output                        \
	test_common.sh                \
	$(UNCOMPILED_TESTS)

TESTS=$(COMPILED_TESTS) $(UNCOMPILED_TESTS)

EXTRA_PROGRAMS=fuzzer
check_PROGRAMS=$(COMPILED_TESTS) dump-headers decompress-crc build-arch
check_LIBRARIES=libtestframework.a

libtestframework_a_SOURCES=     \
	crc32.c crc32.h

clean-local:
	rm -f *.gcno *.gcda *.c.gcov

if BUILD_COVERAGE

# Delete .gcda files before running tests.

check-am: check-prerun

check-prerun:
	@rm -f $(top_builddir)/lib/*.gcda   \
	       $(top_builddir)/src/*.gcda   \
	       $(top_builddir)/test/*.gcda

# Display coverage summary after running tests.

check: check-postrun

check-postrun:
	@cd $(top_builddir); ./gencov lib/*.c src/*.c

endif

fuzzer_SOURCES = fuzzer.c
build_arch_SOURCES = build-arch.c
dump_headers_SOURCES = dump-headers.c
decompress_crc_SOURCES = decompress-crc.c

