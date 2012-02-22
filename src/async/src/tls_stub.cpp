/*
Abstract:
  TLS stub workaround for some boost envirionments

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#if defined(BOOST_THREAD_USE_LIB)
namespace boost
{
  void tss_cleanup_implemented() { }
}
#endif
