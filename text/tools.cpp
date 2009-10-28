// Generated from '$Id$'
#ifndef __TEXT_TOOLS_H_DEFINED__
#define __TEXT_TOOLS_H_DEFINED__
#include <char_type.h>

extern const Char TEXT_ERROR_DEFAULT_FORMAT[] = {
  '%','1','%','\n',
  '\n',
  'C','o','d','e',':',' ','%','2','%','(','%','2','$','#','x',')','\n',
  'A','t',':',' ','%','3','%','\n',
  '-','-','-','-','-','-','-','-','\n',
  0
};
extern const Char TEXT_ERROR_LOCATION_FORMAT[] = {
  '%','1','$','0','8','x',0
};
extern const Char TEXT_ERROR_LOCATION_FORMAT_DEBUG[] = {
  '%','1','$','0','8','x',' ','(','%','2','%',':','%','3','%',',',' ','%','4','%',')',0
};
#endif //__TEXT_TOOLS_H_DEFINED__
