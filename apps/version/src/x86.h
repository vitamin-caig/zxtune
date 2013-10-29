/**
* 
* @file
*
* @brief x86 architecture detection
*
* @author vitamin.caig@gmail.com
*
**/


#pragma once

  const std::string ARCH("x86");
#if _M_IX86 == 600 || defined(__i686__)
  const std::string ARCH_VERSION("i686");
#elif _M_IX86 == 500 || defined(__i586__)
  const std::string ARCH_VERSION("i586");
#elif _M_IX86 == 400 || defined(__i486__)
  const std::string ARCH_VERSION("i486");
#elif _M_IX86 == 300 || defined(__i386__)
  const std::string ARCH_VERSION("i386");
#elif defined(_M_IX86)
  const std::string ARCH_VERSION("blend");
#else
  #error "Not an x86 architecture"
#endif
