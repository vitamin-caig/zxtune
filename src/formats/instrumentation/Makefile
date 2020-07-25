binary_name := formats_fuzz
export compiler := clang
export cxx_flags := -fsanitize=fuzzer,address,undefined,integer, -fsanitize-coverage=trace-pc,trace-cmp,trace-div,trace-gep -fno-sanitize=alignment
export ld_flags := -fsanitize=fuzzer,address,undefined,integer
export platform := linux
export arch := fuzz_asan_ubsan
export tools.cxx := clang++-6.0
export tools.cc := clang-6.0
export tools.ld := clang++-6.0
export tools.strip = true

source_dirs := .
dirs.root := ../../..

libraries.common := binary binary_format binary_compression debug formats_packed formats_chiptune strings tools
libraries.3rdparty := lhasa zlib

include $(dirs.root)/makefile.mak
