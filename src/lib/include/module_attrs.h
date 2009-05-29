#ifndef __MODULE_ATTRS_H_DEFINED__
#define __MODULE_ATTRS_H_DEFINED__

namespace ZXTune
{
  namespace Module
  {
    /// Module attributes
    const String::value_type ATTR_FILENAME[] = {'F', 'i', 'l', 'e', 'n', 'a', 'm', 'e', '\0'};
    const String::value_type ATTR_AUTHOR[] = {'A', 'u', 't', 'h', 'o', 'r', '\0'};
    const String::value_type ATTR_TITLE[] = {'T', 'i', 't', 'l', 'e', '\0'};
    const String::value_type ATTR_PROGRAM[] = {'P', 'r', 'o', 'g', 'r', 'a', 'm', '\0'};
    const String::value_type ATTR_TRACKER[] = {'T', 'r', 'a', 'c', 'k', 'e', 'r', '\0'};
    const String::value_type ATTR_COMPUTER[] = {'C', 'o', 'm', 'p', 'u', 't', 'e', 'r', '\0'};
    const String::value_type ATTR_DATE[] = {'D', 'a', 't', 'e', '\0'};
    const String::value_type ATTR_COMMENT[] = {'C', 'o', 'm', 'm', 'e', 'n', 't', '\0'};
    const String::value_type ATTR_SUBMODULES[] = {'S', 'u', 'b', 'm', 'o', 'd', 'u', 'l', 'e', 's', '\0'};
    const String::value_type ATTR_WARNINGS[] = {'W', 'a', 'r', 'n', 'i', 'n', 'g', 's', '\0'};
  }
}

#endif //__MODULE_ATTRS_H_DEFINED__
