binary_name := formats_fuzz
export distro := any
export boost.version := 1.58.0
export compiler := clang
export cxx_flags := -fsanitize=fuzzer,address,undefined,integer, -fsanitize-coverage=trace-pc,trace-cmp,trace-div,trace-gep -fno-sanitize=alignment
export ld_flags := -fsanitize=fuzzer,address,undefined,integer
export platform := linux
export arch := fuzz_asan_ubsan
export tools.cxx := clang++-6.0
export tools.cc := clang-6.0
export tools.ld := clang++-6.0

source_dirs := .
path_step := ../../..

libraries.common := binary binary_format binary_compression debug formats_packed formats_chiptune strings tools
libraries.3rdparty := lhasa zlib

include $(path_step)/makefile.mak
