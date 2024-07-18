m4_define([lib_major], [2])
m4_define([lib_minor], [7])
m4_define([lib_level], [1])
m4_define([lib_version], [lib_major.lib_minor.lib_level])

AC_PREREQ([2.62])
AC_INIT([libsidplayfp],[lib_version],[],[],[https://github.com/libsidplayfp/libsidplayfp/])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIRS([m4 src/builders/exsid-builder/driver/m4])
AC_CONFIG_SRCDIR([src/sidplayfp/sidplayfp.cpp])
AC_CONFIG_HEADERS([src/config.h])
AM_INIT_AUTOMAKE

LIB_MAJOR=lib_major
LIB_MINOR=lib_minor
LIB_LEVEL=lib_level
AC_SUBST([LIB_MAJOR])
AC_SUBST([LIB_MINOR])
AC_SUBST([LIB_LEVEL])

AC_CANONICAL_HOST

AS_CASE([$host_os],
        [mingw*], [MINGW32=yes],
        [darwin*], [MACOS=yes],
        [MINGW32=no MACOS=no]
)
AM_CONDITIONAL([MINGW32], [test "x$MINGW32" = "xyes"])
AM_CONDITIONAL([MACOS], [test "x$MACOS" = "xyes"])

dnl Initialize libtool.
LT_INIT([win32-dll])

dnl Checks for programs.
AC_PROG_CXX

dnl Use C++ for tests.
AC_LANG([C++])

AX_CXX_COMPILE_STDCXX_17([noext], [optional])
AS_IF([test $ax_cv_cxx_compile_cxx17__std_cpp17 != "yes"], [
    AX_CXX_COMPILE_STDCXX_14([noext], [optional])
    AS_IF([test $ax_cv_cxx_compile_cxx14__std_cpp14 != "yes"],
        AX_CXX_COMPILE_STDCXX_11([noext], [optional])
    )
])

dnl check for hidden visibility support
AX_CHECK_COMPILE_FLAG([-fvisibility=hidden -fvisibility-inlines-hidden], [has_visibility=yes], [has_visibility=no])
AM_CONDITIONAL([HAVE_VISIBILITY], [test "x$has_visibility" = xyes])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_BIGENDIAN

AC_CHECK_SIZEOF([short])
AC_CHECK_SIZEOF([int])

AS_IF([test $ac_cv_sizeof_short -ne 2],
  [AC_MSG_ERROR([size of short must be 16 bit])]
)

AS_IF([test $ac_cv_sizeof_int -lt 4],
  [AC_MSG_ERROR([only 32 bit or better CPUs are supported])]
)

AS_IF([test "x${ac_cv_header_stdint_h}" != "xyes"],
  [AC_MSG_ERROR([Required header stdint.h not found])]
)

AC_OPENMP

dnl Checks for non-standard functions.

AC_CHECK_DECL(
    [strcasecmp],
    [AC_CHECK_FUNCS([strcasecmp])]
)

AC_CHECK_DECL(
    [strncasecmp],
    [AC_CHECK_FUNCS([strncasecmp])]
)

AC_CHECK_PROGS([XA], [xa])

# od on macOS doesn't support the -w parameter
AC_CACHE_CHECK([for od that supports -w], [ac_cv_path_OD],
  [AC_PATH_PROGS_FEATURE_CHECK([OD], [od god],
    [[$ac_path_OD -w > /dev/null 2>&1 && ac_cv_path_OD=$ac_path_OD ac_path_OD_found=:]])]
)
AC_SUBST([OD], [$ac_cv_path_OD])

dnl libtool-style version-info number
#
# https://autotools.io/libtool/version.html
#
# Always increase the revision value.
#
# Increase the current value whenever an interface has been added, removed or changed.
#
# Increase the age value only if the changes made to the ABI are backward compatible.

LIBSIDPLAYCUR=10
LIBSIDPLAYREV=31
LIBSIDPLAYAGE=4
LIBSIDPLAYVERSION=$LIBSIDPLAYCUR:$LIBSIDPLAYREV:$LIBSIDPLAYAGE

LIBSTILVIEWCUR=0
LIBSTILVIEWREV=4
LIBSTILVIEWAGE=0
LIBSTILVIEWVERSION=$LIBSTILVIEWCUR:$LIBSTILVIEWREV:$LIBSTILVIEWAGE

AC_MSG_CHECKING([for debugging])
AC_ARG_ENABLE(debug, [AS_HELP_STRING([--enable-debug], [compile for debugging @<:@no/yes/full, default=no@:>@])],
 [], [enable_debug=no])

AS_IF([test "x$enable_debug" = "xno"],
  [AC_MSG_RESULT([Build without debugging messages]); debug_flags=-DNDEBUG],
  [AS_IF([test "x$enable_debug" = "xyes"],
    [AC_MSG_RESULT([Build with debugging messages]); debug_flags=-DDEBUG=1],
    [AC_MSG_RESULT([Build with all debugging messages]); debug_flags=-DDEBUG=10]
  )]
)

AC_SUBST([debug_flags])

AC_ARG_ENABLE([hardsid],
  AS_HELP_STRING([--enable-hardsid],[enable hardsid builder [default=no]])
)

AM_CONDITIONAL([HARDSID], [test "x$enable_hardsid" = "xyes"])


AC_ARG_ENABLE([inline],
  AS_HELP_STRING([--enable-inline],[enable inlining of functions [default=yes]])
)

AS_IF([test x"$enable_inline" != "xno"],
  [RESID_INLINE=inline; RESID_INLINING=1],
  [RESID_INLINE=""; RESID_INLINING=0]
)

AC_ARG_WITH(
    [simd],
    [AS_HELP_STRING([--with-simd], [Build with SIMD optimizations [@<:@sse2/mmx/neon/none, default=none@:>@]])],
    [],
    [with_simd=none]
)

AS_IF([test x"$with_simd" != xnone],
    # Testing for intrinsic headers requires compiler
    # support activated by specific flags.
    # These are most likely defined in CXXFLAGS
    # but AC_CHECK_HEADERS uses only CPPFLAGS
    # so we need a workaround.
    [saveCPPFLAGS=$CPPFLAGS
    CPPFLAGS="$CPPFLAGS $CXXFLAGS"

    AS_CASE([$with_simd],
        [sse2], [AC_CHECK_HEADERS([emmintrin.h])],
        [mmx],  [AC_CHECK_HEADERS([mmintrin.h])],
        [neon], [AC_CHECK_HEADERS([arm_neon.h])],
        [AC_MSG_ERROR([Unrecognized SIMD specified])]
    )

    CPPFLAGS=$saveCPPFLAGS]
)

AC_CACHE_CHECK([for working bool], ac_cv_cxx_bool,
[AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([],
    [[bool flag;]])],
  [ac_cv_cxx_bool=yes],
  [ac_cv_cxx_bool=no])]
)

AS_IF([test "x$ac_cv_cxx_bool" = "xno"],
  [HAVE_BOOL=0],
  [HAVE_BOOL=1]
)

dnl Enable branch prediction hints.
AC_ARG_ENABLE([branch-hints],
  [AS_HELP_STRING([--enable-branch-hints],
    [enable static branch prediction hints [default=yes]])],
  [],
  [enable_branch_hints=yes]
)

AS_IF([test "$enable_branch_hints" != no],
  [RESID_BRANCH_HINTS=1],
  [RESID_BRANCH_HINTS=0]
)

AC_CACHE_CHECK([for log1p], [resid_cv_log1p],
  [AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
      [ #include <math.h> ], [ log1p(1); ]
    )],
    [resid_cv_log1p=yes], [resid_cv_log1p=no]
  )]
)

AS_IF([test "$resid_cv_log1p" = no],
  [HAVE_LOG1P=0],
  [HAVE_LOG1P=1]
)

AC_CACHE_CHECK([for __builtin_expect], [resid_cv_builtin_expect],
  [AC_COMPILE_IFELSE([AC_LANG_SOURCE([int main() { __builtin_expect(0, 0); }])],
    [resid_cv_builtin_expect=yes], [resid_cv_builtin_expect=no])]
)

AS_IF([test "$resid_cv_builtin_expect" = no],
  [HAVE_BUILTIN_EXPECT=0],
  [HAVE_BUILTIN_EXPECT=1]
)


AC_CHECK_PROGS([DOXYGEN], [doxygen])
AS_IF([test -z "$DOXYGEN"],
  [AC_MSG_WARN([Doxygen not found - continuing without Doxygen support])],
  [AC_CHECK_PROG([DOT], [dot], [YES], [NO])]
)

AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])

AM_COND_IF([HAVE_DOXYGEN], [AC_CONFIG_FILES([Doxyfile])])


AC_ARG_ENABLE([testsuite],
  [AS_HELP_STRING([--enable-testsuite=PATH_TO_TESTSUITE],
    [enable VICE testsuite [default=no]]
  )],
  [],
  [enable_testsuite=no]
)

AS_IF([test "x$enable_testsuite" != xno],
  AC_DEFINE_UNQUOTED([VICE_TESTSUITE],
    ["$enable_testsuite"],
    [Path to VICE testsuite.]
  )
)

AM_CONDITIONAL([TESTSUITE], [test "x$enable_testsuite" != xno])


PKG_PROG_PKG_CONFIG

AC_ARG_ENABLE([tests],
  [AS_HELP_STRING([--enable-tests],
    [enable unit tests [default=no]]
  )],
  [],
  [enable_tests=no]
)

AS_IF([test "x$enable_tests" != "xno"],
  [[echo Enabling unit tests]
    PKG_CHECK_MODULES(UNITTEST, [UnitTest++])
  ]
)

AM_CONDITIONAL([ENABLE_TEST], [test x$enable_tests = xyes])


AC_ARG_WITH([gcrypt], AS_HELP_STRING([--with-gcrypt], [Build with the gcrypt library]))
AS_IF([test "x$with_gcrypt" != "xno"],
  [PKG_CHECK_MODULES(LIBGCRYPT,
    [libgcrypt >= 1.8],
    [USE_LIBGCRYPT=yes
    AC_DEFINE([HAVE_LIBGCRYPT], 1, [Define to 1 if you have libgcrypt (-lgcrypt).])],
    [AS_IF([test "x$with_gcrypt" = "xyes"],
      [AC_MSG_ERROR([gcrypt requested but not found])]
    )]
  )]
)
AM_CONDITIONAL([LIBGCRYPT], [test x$USE_LIBGCRYPT = xyes])


NEW_8580_FILTER=1
AM_CONDITIONAL([NEW_8580_FILTER], [test $NEW_8580_FILTER != 0])

AC_SUBST(HAVE_BOOL)
AC_SUBST(HAVE_LOG1P)
AC_SUBST(HAVE_BUILTIN_EXPECT)
AC_SUBST(RESID_INLINING)
AC_SUBST(RESID_INLINE)
AC_SUBST(RESID_BRANCH_HINTS)
AC_SUBST(NEW_8580_FILTER)

AC_SUBST(LIBSIDPLAYVERSION)
AC_SUBST(LIBSTILVIEWVERSION)

# exSID support
AC_ARG_WITH(
    [exsid],
    [AS_HELP_STRING([--with-exsid], [Build with exsid support [@<:@yes/no/check, default=check@:>@]])],
    [],
    [with_exsid=check]
)

# exSID support requires either libexsid or one of libfdti1 or libftd2xx
AS_IF(
    [test "x$with_exsid" != "xno"],
    [PKG_CHECK_MODULES([EXSID],
        [libexsid >= 2.0],
        [AC_DEFINE([HAVE_EXSID], 1, [Define to 1 if you have libexsid (-lexsid).])
        build_exsid_builder=yes],
        [AC_MSG_WARN([$EXSID_PKG_ERRORS] [- using internal driver])
            AS_IF([test "x$MINGW32" != "xyes"],
                AC_SEARCH_LIBS([dlopen], [dl dld], [],
                  [AC_MSG_ERROR([unable to find the dlopen() function])]
                )
            )
            AC_CHECK_HEADERS([ftd2xx.h],
                [AC_DEFINE([HAVE_FTD2XX], 1, [Define to 1 if you have ftd2xx.h])
                build_exsid_builder=yes
                build_exsid_driver=yes],
                [AC_MSG_WARN([ftd2xx.h not found, support will not be available])]
            )
            AC_MSG_CHECKING([for libftdi1])
            PKG_CHECK_EXISTS([libftdi1 >= 1.0],
                [AC_DEFINE([HAVE_FTDI], 1, [Define to 1 if you have ftdi.h])
                build_exsid_builder=yes
                build_exsid_driver=yes
                FTDI_CFLAGS=`$PKG_CONFIG --cflags libftdi1`
                AC_MSG_RESULT([yes])],
                [AC_MSG_WARN([libftdi1 not found, support will not be available])]
            )]
    )]
)

AS_IF(
    [test "x$with_exsid" = "xyes" -a "x$build_exsid_builder" != xyes],
    [AC_MSG_ERROR([Neither libexsid nor ftd2xx nor libftdi1 found])]
)

AC_SUBST([FTDI_CFLAGS])

AM_CONDITIONAL([EXSID_SUPPORT], [ test "x${build_exsid_builder}" = xyes])
AM_CONDITIONAL([EXSID_DRIVER], [ test "x${build_exsid_driver}" = xyes])

# check for thread model if available
AS_IF([test "x$build_exsid_driver" = xyes],
        [AX_PTHREAD(
                [AC_DEFINE([EXSID_THREADED], 1, [Define for threaded driver])]
                [AC_DEFINE([HAVE_PTHREAD_H], 1, [Define to 1 if you have pthread.h])]
        )]
)

eval loadable_shrext=$shrext_cmds
AC_DEFINE_UNQUOTED([SHLIBEXT], ["${loadable_shrext}"], [Shared library extension])


AC_CONFIG_FILES([
Makefile
libsidplayfp.pc
libstilview.pc
src/sidplayfp/sidversion.h
src/builders/residfp-builder/residfp/siddefs-fp.h
src/builders/resid-builder/resid/siddefs.h
tests/Makefile
])

AC_OUTPUT
