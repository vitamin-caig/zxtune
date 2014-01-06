/* Name of package */
#define PACKAGE "libsidplayfp"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME "libsidplayfp"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "libsidplayfp 1.2.1"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "libsidplayfp"

/* Define to the home page for this package. */
#define PACKAGE_URL "http://sourceforge.net/projects/sidplay-residfp/"

/* Define to the version of this package. */
#define PACKAGE_VERSION "1.2.1"

/* Path to Lorenz' testsuite. */
/* #undef PC64_TESTSUITE */

/* Version number of package */
#define VERSION "1.2.1"

#define LIBSIDPLAYFP_VERSION_MAJ 1
#define LIBSIDPLAYFP_VERSION_MIN 2
#define LIBSIDPLAYFP_VERSION_LEV 1

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif
