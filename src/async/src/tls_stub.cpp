/**
* 
* @file
*
* @brief TSL stub workaround for some boost implementations
*
* @author vitamin.caig@gmail.com
*
**/

#if defined(BOOST_THREAD_USE_LIB)
namespace boost
{
  void tss_cleanup_implemented() { }
}
#endif
