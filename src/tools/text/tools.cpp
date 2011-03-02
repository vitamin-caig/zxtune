// Generated from '$Id$'
#ifndef __TEXT_TOOLS_H_DEFINED__
#define __TEXT_TOOLS_H_DEFINED__
#include <char_type.h>

namespace Text
{
extern const Char ERROR_DEFAULT_FORMAT[] = {
  '%','1','%','\n',
  '\n',
  'C','o','d','e',':',' ','%','2','%','\n',
  'A','t',':',' ','%','3','%','\n',
  '-','-','-','-','-','-','-','-','\n',
  0
};
extern const Char ERROR_LOCATION_FORMAT[] = {
  '%','1','$','0','8','x',0
};
extern const Char ERROR_LOCATION_FORMAT_DEBUG[] = {
  '%','1','$','0','8','x',' ','(','%','2','%',':','%','3','%',',',' ','%','4','%',')',0
};
extern const Char TIME_FORMAT[] = {
  '%','1','%',':','%','2','$','0','2','u','.','%','3','$','0','2','u',0
};
extern const Char TIME_FORMAT_HOURS[] = {
  '%','1','%',':','%','2','$','0','2','u',':','%','3','$','0','2','u','.','%','4','$','0','2','u',0
};
}//namespace Text
#endif //__TEXT_TOOLS_H_DEFINED__
