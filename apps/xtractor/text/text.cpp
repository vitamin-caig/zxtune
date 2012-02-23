// Generated from '$Id
#ifndef __TEXT_TEXT_H_DEFINED__
#define __TEXT_TEXT_H_DEFINED__
#include <char_type.h>

namespace Text
{
extern const Char ANALYSIS_QUEUE_SIZE_DESC[] = {
  'q','u','e','u','e',' ','s','i','z','e',' ','f','o','r',' ','p','a','r','a','l','l','e','l',' ','a','n','a',
  'l','y','s','i','s','.',' ','V','a','l','u','a','b','l','e',' ','o','n','l','y',' ','w','h','e','n',' ','-',
  '-','a','n','a','l','y','s','i','s','-','t','h','r','e','a','d','s',' ','>',' ','0','.',' ','D','e','f','a',
  'u','l','t',' ','i','s',' ','1','0',0
};
extern const Char ANALYSIS_QUEUE_SIZE_KEY[] = {
  'a','n','a','l','y','s','i','s','-','q','u','e','u','e','-','s','i','z','e',0
};
extern const Char ANALYSIS_THREADS_DESC[] = {
  't','h','r','e','a','d','s',' ','c','o','u','n','t',' ','f','o','r',' ','p','a','r','a','l','l','e','l',' ',
  'a','n','a','l','y','s','i','s','.',' ','0',' ','t','o',' ','d','i','s','a','b','l','e',' ','p','a','r','a',
  'l','l','e','l','i','n','g','.',' ','D','e','f','a','u','l','t',' ','i','s',' ','1',0
};
extern const Char ANALYSIS_THREADS_KEY[] = {
  'a','n','a','l','y','s','i','s','-','t','h','r','e','a','d','s',0
};
extern const Char DEFAULT_TARGET_NAME_TEMPLATE[] = {
  'X','T','r','a','c','t','o','r','/','[','F','i','l','e','n','a','m','e',']','/','[','S','u','b','p','a','t',
  'h',']',0
};
extern const Char HELP_DESC[] = {
  's','h','o','w',' ','t','h','i','s',' ','m','e','s','s','a','g','e',0
};
extern const Char HELP_KEY[] = {
  'h','e','l','p',0
};
extern const Char IGNORE_EMPTY_DESC[] = {
  'd','o',' ','n','o','t',' ','s','t','o','r','e',' ','f','i','l','e','s',' ','f','i','l','l','e','d',' ','w',
  'i','t','h',' ','s','i','n','g','l','e',' ','b','y','t','e',0
};
extern const Char IGNORE_EMPTY_KEY[] = {
  'i','g','n','o','r','e','-','e','m','p','t','y',0
};
extern const Char INPUT_DESC[] = {
  's','o','u','r','c','e',' ','f','i','l','e','s',' ','a','n','d',' ','d','i','r','e','c','t','o','r','i','e',
  's',' ','t','o',' ','b','e',' ','p','r','o','c','e','s','s','e','d',0
};
extern const Char INPUT_KEY[] = {
  'i','n','p','u','t',0
};
extern const Char MINIMAL_SIZE_DESC[] = {
  'd','o',' ','n','o','t',' ','s','t','o','r','e',' ','f','i','l','e','s',' ','w','i','t','h',' ','l','e','s',
  's','e','r',' ','s','i','z','e','.',' ','D','e','f','a','u','l','t',' ','i','s',' ','0',0
};
extern const Char MINIMAL_SIZE_KEY[] = {
  'm','i','n','i','m','a','l','-','s','i','z','e',0
};
extern const Char PROGRAM_NAME[] = {
  'x','t','r','a','c','t','o','r',0
};
extern const Char SAVE_QUEUE_SIZE_DESC[] = {
  'q','u','e','u','e',' ','s','i','z','e',' ','f','o','r',' ','p','a','r','a','l','l','e','l',' ','d','a','t',
  'a',' ','s','a','v','i','n','g','.',' ','V','a','l','u','a','b','l','e',' ','o','n','l','y',' ','w','h','e',
  'n',' ','-','-','s','a','v','e','-','t','h','r','e','a','d','s',' ','>',' ','0','.',' ','D','e','f','a','u',
  'l','t',' ','i','s',' ','5','0','0',0
};
extern const Char SAVE_QUEUE_SIZE_KEY[] = {
  's','a','v','e','-','q','u','e','u','e','-','s','i','z','e',0
};
extern const Char SAVE_THREADS_DESC[] = {
  't','h','r','e','a','d','s',' ','c','o','u','n','t',' ','f','o','r',' ','p','a','r','a','l','l','e','l',' ',
  'd','a','t','a',' ','s','a','v','i','n','g','.',' ','0',' ','t','o',' ','d','i','s','a','b','l','e',' ','p',
  'a','r','a','l','l','e','l','i','n','g','.',' ','D','e','f','a','u','l','t',' ','i','s',' ','1',0
};
extern const Char SAVE_THREADS_KEY[] = {
  's','a','v','e','-','t','h','r','e','a','d','s',0
};
extern const Char TARGET_NAME_TEMPLATE_DESC[] = {
  't','a','r','g','e','t',' ','n','a','m','e',' ','t','e','m','p','l','a','t','e','.',' ','D','e','f','a','u',
  'l','t',' ','i','s',' ','X','T','r','a','c','t','o','r','/','[','F','i','l','e','n','a','m','e',']','/','[',
  'S','u','b','p','a','t','h',']','.',' ','A','p','p','l','i','c','a','b','l','e',' ','f','i','e','l','d','s',
  ':',' ','[','F','i','l','e','n','a','m','e',']',',','[','P','a','t','h',']',',','[','F','l','a','t','P','a',
  't','h',']',',','[','S','u','b','p','a','t','h',']',',','[','F','l','a','t','S','u','b','p','a','t','h',']',0
};
extern const Char TARGET_NAME_TEMPLATE_KEY[] = {
  't','a','r','g','e','t','-','n','a','m','e','-','t','e','m','p','l','a','t','e',0
};
extern const Char TARGET_SECTION[] = {
  'T','a','r','g','e','t',' ','o','p','t','i','o','n','s',0
};
extern const Char TEMPLATE_FIELD_FILENAME[] = {
  'F','i','l','e','n','a','m','e',0
};
extern const Char TEMPLATE_FIELD_FLATPATH[] = {
  'F','l','a','t','P','a','t','h',0
};
extern const Char TEMPLATE_FIELD_FLATSUBPATH[] = {
  'F','l','a','t','S','u','b','p','a','t','h',0
};
extern const Char TEMPLATE_FIELD_PATH[] = {
  'P','a','t','h',0
};
extern const Char TEMPLATE_FIELD_SUBPATH[] = {
  'S','u','b','p','a','t','h',0
};
extern const Char USAGE_SECTION[] = {
  'U','s','a','g','e',':','\n',
  '%','1','%',' ','[','o','p','t','i','o','n','s',']',' ','[','-','-','i','n','p','u','t',']',' ','<','i','n',
  'p','u','t',' ','p','a','t','h','s','>',0
};
extern const Char VERSION_DESC[] = {
  's','h','o','w',' ','a','p','p','l','i','c','a','t','i','o','n',' ','v','e','r','s','i','o','n',0
};
extern const Char VERSION_KEY[] = {
  'v','e','r','s','i','o','n',0
};
}//namespace Text
#endif //__TEXT_TEXT_H_DEFINED__
