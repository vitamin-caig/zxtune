// Generated from '$Id$'
#ifndef __TEXT_SOUND_H_DEFINED__
#define __TEXT_SOUND_H_DEFINED__
#include <char_type.h>

namespace Text
{
extern const Char SOUND_ERROR_BACKEND_FAILED[] = {
  'F','a','i','l','e','d',' ','t','o',' ','c','r','e','a','t','e',' ','b','a','c','k','e','n','d',' ','\'','%',
  '1','%','\'','.',0
};
extern const Char SOUND_ERROR_BACKEND_INVALID_CHANNELS[] = {
  'S','p','e','c','i','f','i','e','d',' ','m','o','d','u','l','e',' ','o','r',' ','m','i','x','e','r',' ','c',
  'h','a','n','n','e','l','s',' ','(','%','1','%',')',' ','i','s',' ','o','u','t',' ','o','f',' ','r','a','n',
  'g','e',' ','(','%','2','%','.','.','.','%','3','%',')','.',0
};
extern const Char SOUND_ERROR_BACKEND_INVALID_FILTER[] = {
  'I','n','v','a','l','i','d',' ','f','i','l','t','e','r',' ','s','p','e','c','i','f','i','e','d',' ','f','o',
  'r',' ','b','a','c','k','e','n','d','.',0
};
extern const Char SOUND_ERROR_BACKEND_INVALID_GAIN[] = {
  'F','a','i','l','e','d',' ','t','o',' ','s','e','t',' ','v','o','l','u','m','e','-',' ','g','a','i','n',' ',
  'i','s',' ','o','u','t',' ','o','f',' ','r','a','n','g','e','.',0
};
extern const Char SOUND_ERROR_BACKEND_INVALID_MODULE[] = {
  'I','n','v','a','l','i','d',' ','m','o','d','u','l','e',' ','s','p','e','c','i','f','i','e','d',' ','f','o',
  'r',' ','b','a','c','k','e','n','d','.',0
};
extern const Char SOUND_ERROR_BACKEND_INVALID_STATE[] = {
  'F','a','i','l','e','d',' ','t','o',' ','p','e','r','f','o','r','m',' ','o','p','e','r','a','t','i','o','n',
  ' ','i','n',' ','i','n','v','a','l','i','d',' ','s','t','a','t','e','.',0
};
extern const Char SOUND_ERROR_BACKEND_NOT_FOUND[] = {
  'R','e','q','u','e','s','t','e','d',' ','b','a','c','k','e','n','d',' ','i','s',' ','n','o','t',' ','s','u',
  'p','p','o','r','t','e','d','.',0
};
extern const Char SOUND_ERROR_BACKEND_NO_DEVICES[] = {
  'N','o',' ','s','u','i','t','a','b','l','e',' ','o','u','t','p','u','t',' ','d','e','v','i','c','e','s',' ',
  'f','o','u','n','d',0
};
extern const Char SOUND_ERROR_BACKEND_NO_MEMORY[] = {
  'F','a','i','l','e','d',' ','t','o',' ','w','o','r','k',' ','w','i','t','h',' ','b','a','c','k','e','n','d',
  '-',' ','n','o',' ','m','e','m','o','r','y','.',0
};
extern const Char SOUND_ERROR_BACKEND_PAUSE[] = {
  'F','a','i','l','e','d',' ','t','o',' ','p','a','u','s','e',' ','p','l','a','y','b','a','c','k','.',0
};
extern const Char SOUND_ERROR_BACKEND_PLAYBACK[] = {
  'F','a','i','l','e','d',' ','t','o',' ','s','t','a','r','t','/','c','o','n','t','i','n','u','e',' ','p','l',
  'a','y','b','a','c','k','.',0
};
extern const Char SOUND_ERROR_BACKEND_SEEK[] = {
  'F','a','i','l','e','d',' ','t','o',' ','s','e','t',' ','p','l','a','y','b','a','c','k',' ','p','o','s','i',
  't','i','o','n','.',0
};
extern const Char SOUND_ERROR_BACKEND_SETUP_BACKEND[] = {
  'F','a','i','l','e','d',' ','t','o',' ','s','e','t','u','p',' ','b','a','c','k','e','n','d','.',0
};
extern const Char SOUND_ERROR_BACKEND_STOP[] = {
  'F','a','i','l','e','d',' ','t','o',' ','s','t','o','p',' ','p','l','a','y','b','a','c','k','.',0
};
extern const Char SOUND_ERROR_DISABLED_BACKEND[] = {
  'N','o','t',' ','a','v','a','i','l','a','b','l','e',0
};
extern const Char SOUND_ERROR_FILTER_HIGH_CUTOFF[] = {
  'S','p','e','c','i','f','i','e','d',' ','f','i','l','t','e','r',' ','h','i','g','h',' ','c','u','t','o','f',
  'f',' ','f','r','e','q','u','e','n','c','y',' ','(','%','1','%','H','z',')',' ','i','s',' ','o','u','t',' ',
  'o','f',' ','r','a','n','g','e',' ','(','%','2','%','.','.','.','%','3','%','H','z',')','.',0
};
extern const Char SOUND_ERROR_FILTER_LOW_CUTOFF[] = {
  'S','p','e','c','i','f','i','e','d',' ','f','i','l','t','e','r',' ','l','o','w',' ','c','u','t','o','f','f',
  ' ','f','r','e','q','u','e','n','c','y',' ','(','%','1','%','H','z',')',' ','i','s',' ','o','u','t',' ','o',
  'f',' ','r','a','n','g','e',' ','(','%','2','%','.','.','.','%','3','%','H','z',')','.',0
};
extern const Char SOUND_ERROR_FILTER_ORDER[] = {
  'S','p','e','c','i','f','i','e','d',' ','f','i','l','t','e','r',' ','o','r','d','e','r',' ','(','%','1','%',
  ')',' ','i','s',' ','o','u','t',' ','o','f',' ','r','a','n','g','e',' ','(','%','2','%','.','.','.','%','3',
  '%',')','.',0
};
extern const Char SOUND_ERROR_MIXER_INVALID_MATRIX_CHANNELS[] = {
  'F','a','i','l','e','d',' ','t','o',' ','s','e','t',' ','m','i','x','e','r',' ','m','a','t','r','i','x','-',
  ' ','i','n','v','a','l','i','d',' ','c','h','a','n','n','e','l','s',' ','c','o','u','n','t',' ','s','p','e',
  'c','i','f','i','e','d','.',0
};
extern const Char SOUND_ERROR_MIXER_INVALID_MATRIX_GAIN[] = {
  'F','a','i','l','e','d',' ','t','o',' ','s','e','t',' ','m','i','x','e','r',' ','m','a','t','r','i','x','-',
  ' ','g','a','i','n',' ','i','s',' ','o','u','t',' ','o','f',' ','r','a','n','g','e','.',0
};
extern const Char SOUND_ERROR_MIXER_UNSUPPORTED[] = {
  'F','a','i','l','e','d',' ','t','o',' ','c','r','e','a','t','e',' ','u','n','s','u','p','p','o','r','t','e',
  'd',' ','m','i','x','e','r',' ','w','i','t','h',' ','%','1','%',' ','c','h','a','n','n','e','l','s','.',0
};
}//namespace Text
#endif //__TEXT_SOUND_H_DEFINED__
