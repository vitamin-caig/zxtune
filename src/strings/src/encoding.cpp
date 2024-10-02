/**
 *
 * @file
 *
 * @brief  Encoding-related implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "strings/src/utf8.h"
// common includes
#include <byteorder.h>
#include <iterator.h>
#include <string_view.h>
#include <types.h>
// library includes
#include <math/bitops.h>
#include <strings/encoding.h>
// std includes
#include <algorithm>
#include <array>
#include <cassert>
#include <limits>
#include <vector>

namespace Strings
{
  struct CharTraits
  {
    enum
    {
      Undefined = 0,
      Control,
      Punctuation,
      Graphic,
      Numeric,
      Alphabetic,
      CategoriesCount,  // limiter

      // letter flags
      Capital = 8,
      Vowel = 16,
    };

    static bool IsAlphabetic(uint8_t trait)
    {
      return Alphabetic == GetCategory(trait);
    }

    static uint8_t GetCategory(uint8_t trait)
    {
      return trait & 7;
    }
  };

  struct Letter
  {
    uint16_t Upper;
    uint16_t Lower;
    uint32_t Traits;

    bool operator==(const Letter& rh) const
    {
      return Upper == rh.Upper && Lower == rh.Lower;
    }

    bool operator!=(const Letter& rh) const
    {
      return Upper != rh.Upper || Lower != rh.Lower;
    }
  };

  static const Letter LIMITER = {0, 0, 0};

  // http://www.geocities.ws/click2speak/languages.html

  // clang-format off
  //https://en.wikipedia.org/wiki/Spanish_orthography#Alphabet_in_Spanish
  static const Letter SPANISH[] =
  {
    {0x0041, 0x0061, CharTraits::Vowel},//LATIN LETTER A
    {0x0042, 0x0062, 0},                //LATIN LETTER B
    {0x0043, 0x0063, 0},                //LATIN LETTER C
    {0x0044, 0x0064, 0},                //LATIN LETTER D
    {0x0045, 0x0065, CharTraits::Vowel},//LATIN LETTER E
    {0x0046, 0x0066, 0},                //LATIN LETTER F
    {0x0047, 0x0067, 0},                //LATIN LETTER G
    {0x0048, 0x0068, 0},                //LATIN LETTER H
    {0x0049, 0x0069, CharTraits::Vowel},//LATIN LETTER I
    {0x004a, 0x006a, 0},                //LATIN LETTER J
    {0x004b, 0x006b, 0},                //LATIN LETTER K
    {0x004c, 0x006c, 0},                //LATIN LETTER L
    {0x004d, 0x006d, 0},                //LATIN LETTER M
    {0x004e, 0x006e, 0},                //LATIN LETTER N
    {0x00d1, 0x00f1, 0},                //latin letter n with tilde
    {0x004f, 0x006f, CharTraits::Vowel},//LATIN LETTER O
    {0x0050, 0x0070, 0},                //LATIN LETTER P
    {0x0051, 0x0071, 0},                //LATIN LETTER Q
    {0x0052, 0x0072, 0},                //LATIN LETTER R
    {0x0053, 0x0073, 0},                //LATIN LETTER S
    {0x0054, 0x0074, 0},                //LATIN LETTER T
    {0x0055, 0x0075, CharTraits::Vowel},//LATIN LETTER U
    {0x0056, 0x0076, 0},                //LATIN LETTER V
    {0x0057, 0x0077, 0},                //LATIN LETTER W
    {0x0058, 0x0078, 0},                //LATIN LETTER X
    {0x0059, 0x0079, 0},                //LATIN LETTER Y
    {0x005a, 0x007a, 0},                //LATIN LETTER Z
    LIMITER
  };
  
  //https://en.wikipedia.org/wiki/Danish_and_Norwegian_alphabet
  static const Letter DANISH_NORWAY[] =
  {
    {0x0041, 0x0061, CharTraits::Vowel},//LATIN LETTER A
    {0x0042, 0x0062, 0},                //LATIN LETTER B
    {0x0043, 0x0063, 0},                //LATIN LETTER C
    {0x0044, 0x0064, 0},                //LATIN LETTER D
    {0x0045, 0x0065, CharTraits::Vowel},//LATIN LETTER E
    {0x0046, 0x0066, 0},                //LATIN LETTER F
    {0x0047, 0x0067, 0},                //LATIN LETTER G
    {0x0048, 0x0068, 0},                //LATIN LETTER H
    {0x0049, 0x0069, CharTraits::Vowel},//LATIN LETTER I
    {0x004a, 0x006a, 0},                //LATIN LETTER J
    {0x004b, 0x006b, 0},                //LATIN LETTER K
    {0x004c, 0x006c, 0},                //LATIN LETTER L
    {0x004d, 0x006d, 0},                //LATIN LETTER M
    {0x004e, 0x006e, 0},                //LATIN LETTER N
    {0x004f, 0x006f, CharTraits::Vowel},//LATIN LETTER O
    {0x0050, 0x0070, 0},               //LATIN LETTER P
    {0x0051, 0x0071, 0},               //LATIN LETTER Q
    {0x0052, 0x0072, 0},               //LATIN LETTER R
    {0x0053, 0x0073, 0},               //LATIN LETTER S
    {0x0054, 0x0074, 0},               //LATIN LETTER T
    {0x0055, 0x0075, CharTraits::Vowel},//LATIN LETTER U
    {0x0056, 0x0076, 0},               //LATIN LETTER V
    {0x0057, 0x0077, 0},               //LATIN LETTER W
    {0x0058, 0x0078, 0},               //LATIN LETTER X
    {0x0059, 0x0079, 0},               //LATIN LETTER Y
    {0x005a, 0x007a, 0},               //LATIN LETTER Z
    {0x00c6, 0x00e6, CharTraits::Vowel},//latin letter ae
    {0x00d8, 0x00f8, CharTraits::Vowel},//latin letter o with stroke
    {0x00c5, 0x00e5, CharTraits::Vowel},//latin letter a with ring
    LIMITER
  };
  
  //https://en.wikipedia.org/wiki/Polish_language#Orthography
  static const Letter POLISH[] =
  {
    {0x0041, 0x0061, CharTraits::Vowel},//LATIN LETTER A
    {0x0104, 0x0105, CharTraits::Vowel},//LATIN LETTER A WITH OGONEK
    {0x0042, 0x0062, 0},               //LATIN LETTER B
    {0x0043, 0x0063, 0},               //LATIN LETTER C
    {0x0106, 0x0107, 0},               //latin letter c with acute
    {0x0044, 0x0064, 0},               //LATIN LETTER D
    {0x0045, 0x0065, CharTraits::Vowel},//LATIN LETTER E
    {0x0118, 0x0119, CharTraits::Vowel},//latin letter e with ogonek
    {0x0046, 0x0066, 0},               //LATIN LETTER F
    {0x0047, 0x0067, 0},               //LATIN LETTER G
    {0x0048, 0x0068, 0},               //LATIN LETTER H
    {0x0049, 0x0069, CharTraits::Vowel},//LATIN LETTER I
    {0x004a, 0x006a, 0},               //LATIN LETTER J
    {0x004b, 0x006b, 0},               //LATIN LETTER K
    {0x004c, 0x006c, 0},               //LATIN LETTER L
    {0x0141, 0x0142, 0},               //LATIN LETTER L WITH STROKE
    {0x004d, 0x006d, 0},               //LATIN LETTER M
    {0x004e, 0x006e, 0},               //LATIN LETTER N
    {0x0143, 0x0144, 0},               //latin letter n with acute
    {0x004f, 0x006f, CharTraits::Vowel},//LATIN LETTER O
    {0x00d3, 0x00f3, CharTraits::Vowel},//latin letter o with acute
    {0x0050, 0x0070, 0},               //LATIN LETTER P
    {0x0052, 0x0072, 0},               //LATIN LETTER R
    {0x0053, 0x0073, 0},               //LATIN LETTER S
    {0x015a, 0x015b, 0},               //Latin Letter S with acute
    {0x0054, 0x0074, 0},               //LATIN LETTER T
    {0x0055, 0x0075, CharTraits::Vowel},//LATIN LETTER U
    {0x0057, 0x0077, 0},               //LATIN LETTER W
    {0x0059, 0x0079, 0},               //LATIN LETTER Y
    {0x005a, 0x007a, 0},               //LATIN LETTER Z
    {0x0179, 0x017a, 0},               //latin letter z with acute
    {0x017b, 0x017c, 0},               //LATIN LETTER Z WITH DOT ABOVE
    LIMITER
  };
  
  //https://en.wikipedia.org/wiki/Czech_orthography
  static const Letter CZECH[] =
  {
    {0x0041, 0x0061, CharTraits::Vowel},//LATIN LETTER A
    {0x00c1, 0x00e1, CharTraits::Vowel},//latin letter a with acute
    {0x0042, 0x0062, 0},               //LATIN LETTER B
    {0x0043, 0x0063, 0},               //LATIN LETTER C
    {0x010c, 0x010d, 0},               //Latin Letter C with caron
    {0x0044, 0x0064, 0},               //LATIN LETTER D
    {0x010e, 0x010f, 0},               //Latin Letter D with caron
    {0x0045, 0x0065, CharTraits::Vowel},//LATIN LETTER E
    {0x00c9, 0x00e9, CharTraits::Vowel},//latin letter e with acute
    {0x011a, 0x011b, CharTraits::Vowel},//latin letter e with caron
    {0x0046, 0x0066, 0},               //LATIN LETTER F
    {0x0047, 0x0067, 0},               //LATIN LETTER G
    {0x0048, 0x0068, 0},               //LATIN LETTER H
    {0x0049, 0x0069, CharTraits::Vowel},//LATIN LETTER I
    {0x00cd, 0x00ed, CharTraits::Vowel},//LATIN LETTER I with acute
    {0x004a, 0x006a, 0},               //LATIN LETTER J
    {0x004b, 0x006b, 0},               //LATIN LETTER K
    {0x004c, 0x006c, 0},               //LATIN LETTER L
    {0x004d, 0x006d, 0},               //LATIN LETTER M
    {0x004e, 0x006e, 0},               //LATIN LETTER N
    {0x0147, 0x0148, 0},               //latin letter n with caron
    {0x004f, 0x006f, CharTraits::Vowel},//LATIN LETTER O
    {0x00d3, 0x00f3, CharTraits::Vowel},//latin letter o with acute
    {0x0050, 0x0070, 0},               //LATIN LETTER P
    {0x0051, 0x0071, 0},               //LATIN LETTER Q
    {0x0052, 0x0072, 0},               //LATIN LETTER R
    {0x0158, 0x0159, 0},               //latin letter r with caron
    {0x0053, 0x0073, 0},               //LATIN LETTER S
    {0x0160, 0x0161, 0},               //latin letter s with caron
    {0x0054, 0x0074, 0},               //LATIN LETTER T
    {0x0164, 0x0165, 0},               //latin letter t with caron
    {0x0055, 0x0075, CharTraits::Vowel},//LATIN LETTER U
    {0x00da, 0x00fa, CharTraits::Vowel},//latin letter u with acute
    {0x016e, 0x016f, CharTraits::Vowel},//latin letter u with ring above
    {0x0056, 0x0076, 0},               //LATIN LETTER V
    {0x0057, 0x0077, 0},               //LATIN LETTER W
    {0x0058, 0x0078, 0},               //LATIN LETTER X
    {0x0059, 0x0079, 0},               //LATIN LETTER Y
    {0x00dd, 0x00fd, CharTraits::Vowel},//latin letter y with acute
    {0x005a, 0x007a, 0},               //LATIN LETTER Z
    {0x017d, 0x017e, 0},               //latin letter z with caron
    LIMITER
  };
  
  //https://en.wikipedia.org/wiki/Slovak_orthography
  static const Letter SLOVAK[] =
  {
    {0x0041, 0x0061, CharTraits::Vowel},//LATIN LETTER A
    {0x00c1, 0x00e1, CharTraits::Vowel},//latin letter a with acute
    {0x00c4, 0x00e4, CharTraits::Vowel},//latin letter a with diaeresis
    {0x0042, 0x0062, 0},                //LATIN LETTER B
    {0x0043, 0x0063, 0},                //LATIN LETTER C
    {0x010c, 0x010d, 0},                //Latin Letter C with caron
    {0x0044, 0x0064, 0},                //LATIN LETTER D
    {0x010e, 0x010f, 0},                //Latin Letter D with caron
    {0x01f1, 0x01f3, 0},                //Latin Capital Letter DZ
    {0x01f2, 0x01f3, 0},                //Latin Capital Letter D with Small Letter Z
    {0x01c4, 0x01c6, 0},                //Latin Capital Letter DZ with caron
    {0x01c5, 0x01c6, 0},                //Latin Capital Letter D with Small Letter Z with caron
    {0x0045, 0x0065, CharTraits::Vowel},//LATIN LETTER E
    {0x00c9, 0x00e9, CharTraits::Vowel},//latin letter e with acute
    {0x0046, 0x0066, 0},                //LATIN LETTER F
    {0x0047, 0x0067, 0},                //LATIN LETTER G
    {0x0048, 0x0068, 0},                //LATIN LETTER H
    {0x0049, 0x0069, CharTraits::Vowel},//LATIN LETTER I
    {0x00cd, 0x00ed, CharTraits::Vowel},//LATIN LETTER I with acute
    {0x004a, 0x006a, 0},                //LATIN LETTER J
    {0x004b, 0x006b, 0},                //LATIN LETTER K
    {0x004c, 0x006c, 0},                //LATIN LETTER L
    {0x0139, 0x013a, 0},                //latin letter l with acute
    {0x013d, 0x01e3, 0},                //latin letter l with caron
    {0x004d, 0x006d, 0},                //LATIN LETTER M
    {0x004e, 0x006e, 0},                //LATIN LETTER N
    {0x0147, 0x0148, 0},                //latin letter n with caron
    {0x004f, 0x006f, CharTraits::Vowel},//LATIN LETTER O
    {0x00d3, 0x00f3, CharTraits::Vowel},//latin letter o with acute
    {0x00d4, 0x00f4, CharTraits::Vowel},//latin letter o with circumflex
    {0x0050, 0x0070, 0},                //LATIN LETTER P
    {0x0051, 0x0071, 0},                //LATIN LETTER Q
    {0x0052, 0x0072, 0},                //LATIN LETTER R
    {0x0154, 0x0155, 0},                //latin letter r with acute
    {0x0053, 0x0073, 0},                //LATIN LETTER S
    {0x0160, 0x0161, 0},                //latin letter s with caron
    {0x0054, 0x0074, 0},                //LATIN LETTER T
    {0x0164, 0x0165, 0},                //latin letter t with caron
    {0x0055, 0x0075, CharTraits::Vowel},//LATIN LETTER U
    {0x00da, 0x00fa, CharTraits::Vowel},//latin letter u with acute
    {0x0056, 0x0076, 0},                //LATIN LETTER V
    {0x0057, 0x0077, 0},                //LATIN LETTER W
    {0x0058, 0x0078, 0},                //LATIN LETTER X
    {0x0059, 0x0079, 0},                //LATIN LETTER Y
    {0x00dd, 0x00fd, CharTraits::Vowel},//latin letter y with acute
    {0x005a, 0x007a, 0},                //LATIN LETTER Z
    {0x017d, 0x017e, 0},                //latin letter z with caron
    LIMITER
  };
  
  //https://en.wikipedia.org/wiki/Hungarian_alphabet
  static const Letter HUNGARIAN[] =
  {
    {0x0041, 0x0061, CharTraits::Vowel},//LATIN LETTER A
    {0x00c1, 0x00e1, CharTraits::Vowel},//latin letter a with acute
    {0x0042, 0x0062, 0},                //LATIN LETTER B
    {0x0043, 0x0063, 0},                //LATIN LETTER C
    {0x0044, 0x0064, 0},                //LATIN LETTER D
    {0x01f1, 0x01f3, 0},                //Latin Capital Letter DZ
    {0x01f2, 0x01f3, 0},                //Latin Capital Letter D with Small Letter Z
    {0x0045, 0x0065, CharTraits::Vowel},//LATIN LETTER E
    {0x00c9, 0x00e9, CharTraits::Vowel},//latin letter e with acute
    {0x0046, 0x0066, 0},                //LATIN LETTER F
    {0x0047, 0x0067, 0},                //LATIN LETTER G
    {0x0048, 0x0068, 0},                //LATIN LETTER H
    {0x0049, 0x0069, CharTraits::Vowel},//LATIN LETTER I
    {0x00cd, 0x00ed, CharTraits::Vowel},//LATIN LETTER I with acute
    {0x004a, 0x006a, 0},                //LATIN LETTER J
    {0x004b, 0x006b, 0},                //LATIN LETTER K
    {0x004c, 0x006c, 0},                //LATIN LETTER L
    {0x004d, 0x006d, 0},                //LATIN LETTER M
    {0x004e, 0x006e, 0},                //LATIN LETTER N
    {0x004f, 0x006f, CharTraits::Vowel},//LATIN LETTER O
    {0x00d3, 0x00f3, CharTraits::Vowel},//latin letter o with acute
    {0x00d6, 0x00f6, CharTraits::Vowel},//latin letter o with diaeresis
    {0x0150, 0x0151, CharTraits::Vowel},//latin letter o with double acute
    {0x0050, 0x0070, 0},                //LATIN LETTER P
    {0x0052, 0x0072, 0},                //LATIN LETTER R
    {0x0053, 0x0073, 0},                //LATIN LETTER S
    {0x0054, 0x0074, 0},                //LATIN LETTER T
    {0x0055, 0x0075, CharTraits::Vowel},//LATIN LETTER U
    {0x00da, 0x00fa, CharTraits::Vowel},//latin letter u with acute
    {0x00dc, 0x00fc, CharTraits::Vowel},//latin letter u with diaeresis
    {0x0170, 0x0171, CharTraits::Vowel},//latin letter u with double acute
    {0x0056, 0x0076, 0},                //LATIN LETTER V
    //{0x0057, 0x0077, 0},                //LATIN LETTER W
    //{0x0058, 0x0078, 0},                //LATIN LETTER X
    {0x0059, 0x0079, 0},                //LATIN LETTER Y
    {0x005a, 0x007a, 0},                //LATIN LETTER Z
    LIMITER
  };
  
  //https://en.wikipedia.org/wiki/Slovene_alphabet
  static const Letter SLOVENE[] =
  {
    {0x0041, 0x0061, CharTraits::Vowel},//LATIN LETTER A
    {0x0042, 0x0062, 0},                //LATIN LETTER B
    {0x0043, 0x0063, 0},                //LATIN LETTER C
    {0x010c, 0x010d, 0},                //Latin Letter C with caron
    {0x0044, 0x0064, 0},                //LATIN LETTER D
    {0x0045, 0x0065, CharTraits::Vowel},//LATIN LETTER E
    {0x0046, 0x0066, 0},                //LATIN LETTER F
    {0x0047, 0x0067, 0},                //LATIN LETTER G
    {0x0048, 0x0068, 0},                //LATIN LETTER H
    {0x0049, 0x0069, CharTraits::Vowel},//LATIN LETTER I
    {0x004a, 0x006a, 0},                //LATIN LETTER J
    {0x004b, 0x006b, 0},                //LATIN LETTER K
    {0x004c, 0x006c, 0},                //LATIN LETTER L
    {0x004d, 0x006d, 0},                //LATIN LETTER M
    {0x004e, 0x006e, 0},                //LATIN LETTER N
    {0x004f, 0x006f, CharTraits::Vowel},//LATIN LETTER O
    {0x0050, 0x0070, 0},                //LATIN LETTER P
    {0x0052, 0x0072, 0},                //LATIN LETTER R
    {0x0053, 0x0073, 0},                //LATIN LETTER S
    {0x0160, 0x0161, 0},                //latin letter s with caron
    {0x0054, 0x0074, 0},                //LATIN LETTER T
    {0x0055, 0x0075, CharTraits::Vowel},//LATIN LETTER U
    {0x0056, 0x0076, 0},                //LATIN LETTER V
    {0x005a, 0x007a, 0},                //LATIN LETTER Z
    {0x017d, 0x017e, 0},                //latin letter z with caron
    LIMITER
  };

  //https://en.wikipedia.org/wiki/German_orthography#Alphabet
  static const Letter GERMAN[] =
  {
    {0x0041, 0x0061, CharTraits::Vowel},//LATIN LETTER A
    {0x00c4, 0x00e4, CharTraits::Vowel},//latin letter a with diaeresis
    {0x0042, 0x0062, 0},                //LATIN LETTER B
    {0x0043, 0x0063, 0},                //LATIN LETTER C
    {0x0044, 0x0064, 0},                //LATIN LETTER D
    {0x0045, 0x0065, CharTraits::Vowel},//LATIN LETTER E
    {0x0046, 0x0066, 0},                //LATIN LETTER F
    {0x0047, 0x0067, 0},                //LATIN LETTER G
    {0x0048, 0x0068, 0},                //LATIN LETTER H
    {0x0049, 0x0069, CharTraits::Vowel},//LATIN LETTER I
    {0x004a, 0x006a, 0},                //LATIN LETTER J
    {0x004b, 0x006b, 0},                //LATIN LETTER K
    {0x004c, 0x006c, 0},                //LATIN LETTER L
    {0x004d, 0x006d, 0},                //LATIN LETTER M
    {0x004e, 0x006e, 0},                //LATIN LETTER N
    {0x004f, 0x006f, CharTraits::Vowel},//LATIN LETTER O
    {0x00d6, 0x00f6, CharTraits::Vowel},//latin letter o with diaeresis
    {0x0050, 0x0070, 0},                //LATIN LETTER P
    {0x0051, 0x0071, 0},                //LATIN LETTER Q
    {0x0052, 0x0072, 0},                //LATIN LETTER R
    {0x0053, 0x0073, 0},                //LATIN LETTER S
    {0x0054, 0x0074, 0},                //LATIN LETTER T
    {0x0055, 0x0075, CharTraits::Vowel},//LATIN LETTER U
    {0x00dc, 0x00fc, CharTraits::Vowel},//LATIN LETTER U with diaeresis
    {0x0056, 0x0076, 0},                //LATIN LETTER V
    {0x0057, 0x0077, 0},                //LATIN LETTER W
    {0x0058, 0x0078, 0},                //LATIN LETTER X
    {0x0059, 0x0079, 0},                //LATIN LETTER Y
    {0x005a, 0x007a, 0},                //LATIN LETTER Z
    {0x1e9e, 0x00df, 0},                //latin letter sharp s
    LIMITER
  };

  static const Letter RUSSIAN[] =
  {
    {0x0410, 0x0430, CharTraits::Vowel},//A
    {0x0411, 0x0431, 0},                //Be
    {0x0412, 0x0432, 0},                //Ve
    {0x0413, 0x0433, 0},                //Ge
    {0x0414, 0x0434, 0},                //De
    {0x0415, 0x0435, CharTraits::Vowel},//E
    {0x0401, 0x0451, CharTraits::Vowel},//Yo
    {0x0416, 0x0436, 0},                //Zh
    {0x0417, 0x0437, 0},                //Ze
    {0x0418, 0x0438, CharTraits::Vowel},//I
    {0x0419, 0x0439, 0},                //Ij
    {0x041a, 0x043a, 0},                //Ka
    {0x041b, 0x043b, 0},                //eL
    {0x041c, 0x043c, 0},                //eM
    {0x041d, 0x043d, 0},                //eN
    {0x041e, 0x043e, CharTraits::Vowel},//O
    {0x041f, 0x043f, 0},                //Pe
    {0x0420, 0x0440, 0},                //eR
    {0x0421, 0x0441, 0},                //eS
    {0x0422, 0x0442, 0},                //Te
    {0x0423, 0x0443, CharTraits::Vowel},//U
    {0x0424, 0x0444, 0},                //eF
    {0x0425, 0x0445, 0},                //Ha
    {0x0426, 0x0446, 0},                //Tse
    {0x0427, 0x0447, 0},                //Cha
    {0x0428, 0x0448, 0},                //Scha
    {0x0429, 0x0449, 0},                //Sha
    {0x042a, 0x044a, 0},                //TvZnak
    {0x042b, 0x044b, CharTraits::Vowel},//Y
    {0x042c, 0x044c, 0},                //MZnak
    {0x042d, 0x044d, CharTraits::Vowel},//E
    {0x042e, 0x044e, CharTraits::Vowel},//Ju
    {0x042f, 0x044f, CharTraits::Vowel},//Ja
    LIMITER
  };
  
  static const Letter ENGLISH[] =
  {
    {0x0041, 0x0061, CharTraits::Vowel},//LATIN LETTER A
    {0x0042, 0x0062, 0},                //LATIN LETTER B
    {0x0043, 0x0063, 0},                //LATIN LETTER C
    {0x0044, 0x0064, 0},                //LATIN LETTER D
    {0x0045, 0x0065, CharTraits::Vowel},//LATIN LETTER E
    {0x0046, 0x0066, 0},                //LATIN LETTER F
    {0x0047, 0x0067, 0},                //LATIN LETTER G
    {0x0048, 0x0068, 0},                //LATIN LETTER H
    {0x0049, 0x0069, CharTraits::Vowel},//LATIN LETTER I
    {0x004a, 0x006a, 0},                //LATIN LETTER J
    {0x004b, 0x006b, 0},                //LATIN LETTER K
    {0x004c, 0x006c, 0},                //LATIN LETTER L
    {0x004d, 0x006d, 0},                //LATIN LETTER M
    {0x004e, 0x006e, 0},                //LATIN LETTER N
    {0x004f, 0x006f, CharTraits::Vowel},//LATIN LETTER O
    {0x0050, 0x0070, 0},                //LATIN LETTER P
    {0x0051, 0x0071, 0},                //LATIN LETTER Q
    {0x0052, 0x0072, 0},                //LATIN LETTER R
    {0x0053, 0x0073, 0},                //LATIN LETTER S
    {0x0054, 0x0074, 0},                //LATIN LETTER T
    {0x0055, 0x0075, CharTraits::Vowel},//LATIN LETTER U
    {0x0056, 0x0076, 0},                //LATIN LETTER V
    {0x0057, 0x0077, 0},                //LATIN LETTER W
    {0x0058, 0x0078, 0},                //LATIN LETTER X
    {0x0059, 0x0079, 0},                //LATIN LETTER Y
    {0x005a, 0x007a, 0},                //LATIN LETTER Z
    LIMITER
  };
  // clang-format on

  class UnicodeTraits
  {
  public:
    enum LanguagesMask
    {
      Unknown = 0,
      English = 1,
      Russian = 2,
      German = 4,
      Slovene = 8,
      Hungarian = 16,
      Slovak = 32,
      Czech = 64,
      Polish = 128,
      DanishNorway = 256,
      Spanish = 512,
      Japanese = 1024
    };

    uint8_t GetTraits(uint32_t sym) const
    {
      if (sym < Traits.size())
      {
        return Traits[sym];
      }
      else if (sym < 0x2070)
      {
        return CharTraits::Punctuation;
      }
      else if (sym < 0x20a0)
      {
        return CharTraits::Numeric;
      }
      else if (sym < 0x2500)
      {
        return CharTraits::Punctuation;
      }
      else if (sym < 0x2c00)
      {
        return CharTraits::Graphic;
      }
      else
      {
        return CharTraits::Alphabetic;
      }
    }

    uint_t GetLanguages(uint32_t sym) const
    {
      if (sym < Languages.size())
      {
        return Languages[sym];
      }
      else if (sym >= 0x2e80 && sym <= 0x9fff)
      {
        return Japanese;
      }
      else
      {
        return Unknown;
      }
    }

    static const UnicodeTraits& Instance()
    {
      static const UnicodeTraits instance;
      return instance;
    }

  private:
    static const std::size_t TOTAL_SYMBOLS = 0x2000;
    static const std::size_t TOTAL_ALPHABETIC = 0x2000;

    UnicodeTraits()
    {
      AddTrait(CharTraits::Control, 0x00, 0x1f);
      AddTrait(CharTraits::Punctuation, 0x20, 0x2f);
      AddTrait(CharTraits::Numeric, 0x30, 0x39);
      AddTrait(CharTraits::Punctuation, 0x3a, 0x40);
      AddTrait(CharTraits::Punctuation, 0x5b, 0x60);
      AddTrait(CharTraits::Punctuation, 0x7b, 0x7e);
      AddTrait(CharTraits::Control, 0x7f, 0x9f);
      AddTrait(CharTraits::Punctuation, 0xa0, 0xbf);
      AddTrait(CharTraits::Alphabetic, 0xc0, 0x2af);
      AddTrait(CharTraits::Punctuation, 0x2b0, 0x36f);
      AddTrait(CharTraits::Alphabetic, 0x370, 0x1fff);
      AddLanguage(English, ENGLISH);
      AddLanguage(Russian, RUSSIAN);
      AddLanguage(German, GERMAN);
      AddLanguage(Slovene, SLOVENE);
      AddLanguage(Hungarian, HUNGARIAN);
      AddLanguage(Slovak, SLOVAK);
      AddLanguage(Czech, CZECH);
      AddLanguage(Polish, POLISH);
      AddLanguage(DanishNorway, DANISH_NORWAY);
      AddLanguage(Spanish, SPANISH);
    }

    void AddTrait(uint_t trait, uint_t first, uint_t last)
    {
      for (uint_t idx = first; idx <= last; ++idx)
      {
        Traits[idx] = trait;
      }
    }

    void AddLanguage(LanguagesMask lang, const Letter* alphabet)
    {
      for (const auto* it = alphabet; *it != LIMITER; ++it)
      {
        const auto letter = *it;
        Traits.at(letter.Upper) = CharTraits::Alphabetic | letter.Traits | CharTraits::Capital;
        Traits.at(letter.Lower) = CharTraits::Alphabetic | letter.Traits;
        Languages.at(letter.Upper) |= lang;
        Languages.at(letter.Lower) |= lang;
      }
    }

  private:
    std::array<uint8_t, TOTAL_SYMBOLS> Traits;
    std::array<uint16_t, TOTAL_ALPHABETIC> Languages;
  };

  // clang-format off
  
  /*
    CP866:
      no latins
      #80..#af => #410..#43f delta=#390
      #e0..#ef => #440..#44f delta=#360
      #f0 (YO) => #401
      #f1 (yo) => #451
  */
  struct CP866
  {
    static uint32_t GetUnicode(uint8_t sym)
    {
      static const uint32_t UNICODES[128] =
      {
        // x0       x1      x2      x3      x4      x5      x6      x7      x8      x9      xA      xB      xC      xD      xE      xF
        0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417, 0x0418, 0x0419, 0x041a, 0x041b, 0x041c, 0x041d, 0x041e, 0x041f, //8x
        0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427, 0x0428, 0x0429, 0x042a, 0x042b, 0x042c, 0x042d, 0x042e, 0x042f, //9x
        0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437, 0x0438, 0x0439, 0x043a, 0x043b, 0x043c, 0x043d, 0x043e, 0x043f, //Ax
        0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556, 0x2555, 0x2563, 0x2551, 0x2557, 0x255d, 0x255c, 0x255b, 0x2510, //Bx
        0x2514, 0x2534, 0x252c, 0x251c, 0x2500, 0x253c, 0x255e, 0x255f, 0x255a, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256c, 0x2567, //Cx
        0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256b, 0x256a, 0x2518, 0x250c, 0x2588, 0x2584, 0x258c, 0x2590, 0x2580, //Dx
        0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447, 0x0448, 0x0449, 0x044a, 0x044b, 0x044c, 0x044d, 0x044e, 0x044f, //Ex
        0x0401, 0x0451, 0x0404, 0x0454, 0x0407, 0x0457, 0x040e, 0x045e, 0x00b0, 0x2219, 0x00b7, 0x221a, 0x2116, 0x00a4, 0x25a0, 0x00a0, //Fx
      };
      return sym < 128 ? sym : UNICODES[sym - 128];
    }
  };
  
  /*
    CP1251:
      no latins
      #c0..#ff => #410..#44f delta=#350
  */
  struct CP1251
  {
    static uint32_t GetUnicode(uint8_t sym)
    {
      static const uint32_t UNICODES[128] =
      {
        // x0       x1      x2      x3      x4      x5      x6      x7      x8      x9      xA      xB      xC      xD      xE      xF
        0x0402, 0x0403, 0x201a, 0x0453, 0x201e, 0x2026, 0x2020, 0x2021, 0x20ac, 0x2030, 0x0409, 0x2039, 0x040a, 0x040c, 0x040b, 0x040f, //8x
        0x0452, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, 0x0000, 0x2122, 0x0459, 0x203a, 0x045a, 0x045c, 0x045b, 0x045f, //9x
        0x00a0, 0x040e, 0x045e, 0x0408, 0x00a4, 0x0490, 0x00a6, 0x00a7, 0x0401, 0x00a9, 0x0404, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x0407, //Ax
        0x00b0, 0x00b1, 0x0406, 0x0456, 0x0491, 0x00b5, 0x00b6, 0x00b7, 0x0451, 0x2116, 0x0454, 0x00bb, 0x0458, 0x0405, 0x0455, 0x0457, //Bx
        0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417, 0x0418, 0x0419, 0x041a, 0x041b, 0x041c, 0x041d, 0x041e, 0x041f, //Cx
        0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427, 0x0428, 0x0429, 0x042a, 0x042b, 0x042c, 0x042d, 0x042e, 0x042f, //Dx
        0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437, 0x0438, 0x0439, 0x043a, 0x043b, 0x043c, 0x043d, 0x043e, 0x043f, //Ex
        0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447, 0x0448, 0x0449, 0x044a, 0x044b, 0x044c, 0x044d, 0x044e, 0x044f, //Fx
      };
      return sym < 128 ? sym : UNICODES[sym - 128];
    }
  };
  
  struct CP1252
  {
    static uint32_t GetUnicode(uint8_t sym)
    {
      static const uint32_t UNICODES[32] =
      {
        // x0       x1      x2      x3      x4      x5      x6      x7      x8      x9      xA      xB      xC      xD      xE      xF
        0x20ac, 0x0000, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021, 0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0x0000, 0x017d, 0x0000, //8x
        0x0000, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, 0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0x0000, 0x017e, 0x0178, //9x
      };
      return sym < 0x80 || sym >= 0xa0 ? sym : UNICODES[sym - 0x80];
    }
  };
  
  struct CP1250
  {
    static uint32_t GetUnicode(uint8_t sym)
    {
      static const uint32_t UNICODES[128] =
      {
        // x0       x1      x2      x3      x4      x5      x6      x7      x8      x9      xA      xB      xC      xD      xE      xF
        0x20ac, 0x0000, 0x201a, 0x0000, 0x201e, 0x2026, 0x2020, 0x2021, 0x0000, 0x2030, 0x0160, 0x2039, 0x015a, 0x0164, 0x017d, 0x0179, //8x
        0x0000, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, 0x0000, 0x2122, 0x0161, 0x203a, 0x015b, 0x0165, 0x017e, 0x017a, //9x
        0x00a0, 0x02c7, 0x02d8, 0x0141, 0x00a4, 0x0104, 0x00a6, 0x00a7, 0x00a8, 0x00a9, 0x015e, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x017b, //Ax
        0x00b0, 0x00b1, 0x02db, 0x0142, 0x00b4, 0x00b5, 0x00b6, 0x00b7, 0x00b8, 0x0105, 0x015f, 0x00bb, 0x013d, 0x02dd, 0x013e, 0x017c, //Bx
        0x0154, 0x00c1, 0x00c2, 0x0102, 0x00c4, 0x0139, 0x0106, 0x00c7, 0x010c, 0x00c9, 0x0118, 0x00cb, 0x011a, 0x00cd, 0x00ce, 0x010e, //Cx
        0x0110, 0x0143, 0x0147, 0x00d3, 0x00d4, 0x0150, 0x00d6, 0x00d7, 0x0158, 0x016e, 0x00da, 0x0170, 0x00dc, 0x00dd, 0x0162, 0x00df, //Dx
        0x0155, 0x00e1, 0x00e2, 0x0103, 0x00e4, 0x013a, 0x0107, 0x00e7, 0x010d, 0x00e9, 0x0119, 0x00eb, 0x011b, 0x00ed, 0x00ee, 0x010f, //Ex
        0x0111, 0x0144, 0x0148, 0x00f3, 0x00f4, 0x0151, 0x00f6, 0x00f7, 0x0159, 0x016f, 0x00fa, 0x0171, 0x00fc, 0x00fd, 0x0163, 0x02d9, //Fx
      };
      return sym < 0x80 ? sym : UNICODES[sym - 0x80];
    }
  };
  // clang-format on

  uint_t GetPenalty(const std::vector<uint32_t>& symbols)
  {
    enum
    {
      PrevIsVowel = 1,
      CurrIsVowel = 2,

      ConsCons = 0,
      VowCons = PrevIsVowel,
      ConsVow = CurrIsVowel,
      VowVow = PrevIsVowel + CurrIsVowel,

      PairsTypes,
    };
    uint_t categories[CharTraits::CategoriesCount] = {0};
    uint_t pairs[PairsTypes] = {0};  // 2*curIsVowel + 1*prevIsVowel;
    uint_t languagesStrong = ~0;
    uint_t languagesWeak = 0;
    uint8_t prev = CharTraits::Undefined;
    const auto& unicode = UnicodeTraits::Instance();
    for (const auto sym : symbols)
    {
      const auto curr = unicode.GetTraits(sym);
      if (!curr)
      {
        return std::numeric_limits<uint_t>::max();  // don't known how to recode
      }
      const auto cat = CharTraits::GetCategory(curr);
      ++categories[cat];
      const bool isAlphabetic = CharTraits::IsAlphabetic(curr);
      if (CharTraits::IsAlphabetic(prev) && isAlphabetic)
      {
        const bool prevIsVowel = prev & CharTraits::Vowel;
        const bool currIsVowel = curr & CharTraits::Vowel;
        ++pairs[CurrIsVowel * currIsVowel + PrevIsVowel * prevIsVowel];
      }
      if (isAlphabetic)
      {
        const auto langs = unicode.GetLanguages(sym);
        languagesStrong &= langs;
        languagesWeak |= langs;
      }
      prev = curr;
    }
    const auto strongLangsCount = Math::CountBits(languagesStrong);
    const auto weakLangsCount = Math::CountBits(languagesWeak);
    const auto strongLangsPenalty = strongLangsCount > 1 ? strongLangsCount * 8 : (strongLangsCount == 1 ? 0 : 1024);
    const auto weakLangsPenalty = weakLangsCount > 1 ? weakLangsCount * 4 : (weakLangsCount == 1 ? 0 : 2);
    const auto ctrlPenalty = categories[CharTraits::Control] * 512;
    const auto graphPenalty = categories[CharTraits::Graphic] * 256;
    const auto punctPenalty = categories[CharTraits::Punctuation] * 128;
    const auto pairsPenalty = (pairs[ConsCons] + pairs[VowVow]) * 64;
    return ctrlPenalty + graphPenalty + punctPenalty + pairsPenalty + strongLangsPenalty + weakLangsPenalty;
  }

  class Codepage
  {
  public:
    virtual ~Codepage() = default;

    virtual bool Check(StringView str) const = 0;
    virtual std::vector<uint32_t> Translate(StringView str) const = 0;
  };

  template<class Traits>
  class Codepage8Bit : public Codepage
  {
  public:
    bool Check(StringView) const override
    {
      return true;
    }

    std::vector<uint32_t> Translate(StringView str) const override
    {
      std::vector<uint32_t> result(str.size());
      std::transform(str.begin(), str.end(), result.begin(), &Traits::GetUnicode);
      return result;
    }

    static const Codepage& Instance()
    {
      static const Codepage8Bit<Traits> instance;
      return instance;
    }

  private:
    Codepage8Bit() = default;
  };

  // https://en.wikipedia.org/wiki/Shift_JIS
  class ShiftJIS : public Codepage
  {
  public:
    bool Check(StringView str) const override
    {
      for (const auto* it = str.begin(); it != str.end(); ++it)
      {
        const uint8_t s1 = *it;
        if (s1 == 0x80 || s1 == 0xa0 || s1 >= 0xf0)
        {
          // unused as first byte
          return false;
        }
        else if (s1 < 0x80)
        {
          // non-altered
          continue;
        }
        else if (s1 > 0xa0 && s1 < 0xe0)
        {
          // do not support half-width katakana due to detection problem
          return false;
        }
        else if (++it == str.end())
        {
          return false;
        }
        const uint8_t s2 = *it;
        if (s2 < 0x40 || s2 == 0x7f || s2 > 0xfc)
        {
          // unused as second byte
          return false;
        }
      }
      return true;
    }

    std::vector<uint32_t> Translate(StringView str) const override
    {
      std::vector<uint32_t> result;
      result.reserve(str.size());
      for (const auto* it = str.begin(); it != str.end(); ++it)
      {
        const uint8_t s1 = *it;
        if (s1 == 0x5c)
        {
          result.push_back(0x00a5);
        }
        else if (s1 == 0x7e)
        {
          result.push_back(0x203e);
        }
        else if (s1 < 0x80)
        {
          result.push_back(s1);
        }
        else if (s1 > 0xa0 && s1 < 0xe0)
        {
          result.push_back(0xff60 + (s1 - 0xa0));
        }
        else
        {
          const uint8_t s2 = *++it;
          result.push_back(GetUnicode(s1, s2));
        }
      }
      return result;
    }

    static const Codepage& Instance()
    {
      static const ShiftJIS instance;
      return instance;
    }

  private:
    ShiftJIS() = default;

    static uint32_t GetUnicode(uint_t s1, uint_t s2)
    {
#include "strings/src/sjis2unicode.inc"
      const auto lo = s2 - MIN_LO;
      if (s1 >= 0xe0)
      {
        const auto hi = s1 - 0xe0;
        return SJIS2UNICODE_E0[(256 - MIN_LO) * hi + lo];
      }
      else
      {
        const auto hi = s1 - 0x81;
        return SJIS2UNICODE_81[(256 - MIN_LO) * hi + lo];
      }
    }
  };

  String Decode(StringView str)
  {
    static const Codepage* CODEPAGES[] = {&Codepage8Bit<CP866>::Instance(), &Codepage8Bit<CP1251>::Instance(),
                                          &Codepage8Bit<CP1250>::Instance(), &Codepage8Bit<CP1252>::Instance(),
                                          &ShiftJIS::Instance()};

    std::vector<uint32_t> bestUnicode;
    uint_t minPenalty = std::numeric_limits<uint_t>::max();
    for (const auto* const cp : CODEPAGES)
    {
      if (!cp->Check(str))
      {
        continue;
      }
      auto unicode = cp->Translate(str);
      const auto penalty = GetPenalty(unicode);
      if (penalty <= minPenalty)
      {
        minPenalty = penalty;
        bestUnicode.swap(unicode);
      }
      if (0 == penalty)
      {
        break;
      }
    }
    return Utf8Builder(bestUnicode.begin(), bestUnicode.end()).GetResult();
  }
}  // namespace Strings

namespace Strings
{
  String ToAutoUtf8(StringView str)
  {
    static_assert(sizeof(str[0]) == 1, "8-bit encodings only supported");
    if (str.empty())
    {
      return {};
    }
    else if (IsUtf8(str))
    {
      return String{CutUtf8BOM(str)};
    }
    else if (IsUtf16(str))
    {
      std::vector<uint16_t> aligned(str.size() / 2);
      auto* target = aligned.data();
      std::memcpy(target, str.data(), aligned.size() * sizeof(*target));
      return Utf16ToUtf8(MakeStringView(target, target + aligned.size()));
    }
    else
    {
      return Decode(str);
    }
  }

  String Utf16ToUtf8(std::basic_string_view<uint16_t> str)
  {
    static const uint16_t BOM = 0xfeff;
    Strings::Utf8Builder builder;
    builder.Reserve(str.size());
    bool needSwap = false;
    for (const auto* it = str.begin(); it != str.end();)
    {
      const uint32_t sym = needSwap ? swapBytes(*it) : (*it);
      ++it;
      if (sym == swapBytes(BOM))
      {
        needSwap = true;
        continue;
      }
      else if (sym == BOM)
      {
        continue;
      }
      else if (sym >= 0xd800 && sym <= 0xdfff && it != str.end())
      {
        // surrogate pairs
        const uint32_t addon = needSwap ? swapBytes(*it) : (*it);
        if (addon >= 0xdc00 && addon <= 0xdfff)
        {
          ++it;
          builder.Add(((sym - 0xd800) << 10) + (addon - 0xdc00) + 0x10000);
          continue;
        }
      }
      builder.Add(sym);
    }
    return builder.GetResult();
  }
}  // namespace Strings
