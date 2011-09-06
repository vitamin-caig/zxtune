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
extern const Char FAILED_FIND_DYNAMIC_LIBRARY_SYMBOL[] = {
  'F','a','i','l','e','d',' ','t','o',' ','f','i','n','d',' ','s','y','m','b','o','l',' ','\'','%','1','%','\'',
  ' ','i','n',' ','d','y','n','a','m','i','c',' ','l','i','b','r','a','r','y','.',0
};
extern const Char FAILED_LOAD_DYNAMIC_LIBRARY[] = {
  'F','a','i','l','e','d',' ','t','o',' ','l','o','a','d',' ','d','y','n','a','m','i','c',' ','l','i','b','r',
  'a','r','y',' ','\'','%','1','%','\'',' ','(','%','2','%',')','.',0
};
extern const Char TIME_FORMAT[] = {
  '%','1','%',':','%','2','$','0','2','u','.','%','3','$','0','2','u',0
};
extern const Char TIME_FORMAT_HOURS[] = {
  '%','1','%',':','%','2','$','0','2','u',':','%','3','$','0','2','u','.','%','4','$','0','2','u',0
};
}//namespace Text
#endif //__TEXT_TOOLS_H_DEFINED__
