/**
*
* @file
*
* @brief  Encoding-related implementation
*
* @author vitamin.caig@gmail.com
*
**/


//common includes
#include <types.h>
//library includes
#include <strings/encoding.h>
//std includes
#include <algorithm>
#include <cassert>
//boost includes
#include <boost/range/end.hpp>

namespace
{
  /*
    Bigram- or trigram-based detection is better but requires way too much memory for dictionary.
    So use the simpliest method
  */
  
  inline bool IsAscii(uint8_t s)
  {
    return s < 128;
  }
  
  class Codepage
  {
  public:
    virtual ~Codepage() {}
    
    virtual int_t GetWeight(uint8_t symbol) const = 0;
    virtual uint_t Translate(uint8_t symbol) const = 0;
  };
  
  /*
    CP866:
      no latins
      #80..#af => #410..#43f delta=#390
      #e0..#ef => #440..#44f delta=#360
      #f0 (YO) => #401
      #f1 (yo) => #451
  */
  class CP866Codepage : public Codepage
  {
  public:
    virtual int_t GetWeight(uint8_t s) const
    {
      return (0x20 <= s && s <= 0x40)
          || (0x80 <= s && s <= 0xaf)
          || (0xe0 <= s && s <= 0xf1)
      ;
    }

    virtual uint_t Translate(uint8_t s) const
    {
      assert(0 != GetWeight(s));
      if (0x80 <= s && s <= 0xaf)
      {
        return s += 0x390;
      }
      else if (0xe0 <= s && s <= 0xef)
      {
        return s += 0x360;
      }
      else if (s == 0xf0)
      {
        return 0x401;
      }
      else if (s == 0xf1)
      {
        return 0x451;
      }
      else
      {
        return s;
      }
    }
  };
  
  /*
    CP1251:
      no latins
      #c0..#ff => #410..#44f delta=#350
  */
  class CP1251Codepage : public Codepage
  {
  public:
    virtual int_t GetWeight(uint8_t s) const
    {
      return (0x20 <= s && s <= 0x40)
          || (0xc0 <= s)
      ;
    }
    
    virtual uint_t Translate(uint8_t s) const
    {
      assert(0 != GetWeight(s));
      return 0xc0 <= s
        ? s + 0x350
        : s;
    }
  };
  
  class CP1252Codepage : public Codepage
  {
  public:
    virtual int_t GetWeight(uint8_t s) const
    {
      return (0x20 <= s && s <= 0x7f)
          || Decoded(s) != 0;
    }
    
    virtual uint_t Translate(uint8_t s) const
    {
      if (const uint_t d = Decoded(s))
      {
        return d;
      }
      else
      {
        return s;
      }
    }
  private:
    static uint_t Decoded(uint8_t s)
    {
      static const uint_t DECODE[128] =
      {
        //       x0    x1    x2    x3    x4    x5    x6    x7    x8    x9    xa    xb    xc    xd    xe    xf
        /*8x*/ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x160, 0x00, 0x00, 0x00,0x17d, 0x00,
        /*9x*/ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x161, 0x00, 0x00, 0x00,0x17e,0x178,
        /*ax*/ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /*bx*/ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /*cx*/ 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0x00, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
        /*dx*/ 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0x00, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
        /*ex*/ 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0x00, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
        /*fx*/ 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0x00, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
      };
      return s >= 128
           ? DECODE[s - 128]
           : s;
    }
  };
  
  class CP1250Codepage : public Codepage
  {
  public:
    virtual int_t GetWeight(uint8_t s) const
    {
      return (0x20 <= s && s <= 0x7f)
          || Decoded(s) != 0;
    }
    
    virtual uint_t Translate(uint8_t s) const
    {
      if (const uint_t d = Decoded(s))
      {
        return d;
      }
      else
      {
        return s;
      }
    }
  private:
    static uint_t Decoded(uint8_t s)
    {
      static const uint_t DECODE[128] =
      {
        //       x0    x1    x2    x3    x4    x5    x6    x7    x8    x9    xa    xb    xc    xd    xe    xf
        /*8x*/ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x160, 0x00,0x15a,0x164,0x17d,0x179,
        /*9x*/ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x161, 0x00,0x15b,0x165,0x17e,0x17a,
        /*ax*/ 0x00, 0x00, 0x00,0x141, 0x00,0x104, 0x00, 0x00, 0x00, 0x00,0x15e, 0x00, 0x00, 0x00, 0x00,0x17b,
        /*bx*/ 0x00, 0x00, 0x00,0x142, 0x00, 0x00, 0x00, 0x00, 0x00,0x105,0x15f, 0x00,0x13d, 0x00,0x13e,0x17c,
        /*cx*/0x154, 0xc1, 0xc2,0x102, 0xc4,0x139,0x106, 0xc7,0x10c, 0xc9,0x118, 0xcb,0x11a, 0xcd, 0xce,0x10e,
        /*dx*/0x110,0x143,0x147, 0xd3, 0xd4,0x150, 0xd6, 0x00,0x158,0x16e, 0xda,0x170, 0xdc, 0xdd,0x162, 0xdf,
        /*ex*/0x155, 0xe1, 0xe2,0x103, 0xe4,0x13a,0x107, 0xe7,0x10d, 0xe9,0x119, 0xeb,0x11b, 0xed, 0xee,0x10f,
        /*fx*/0x111,0x144,0x148, 0xf3, 0xf4,0x151, 0xf6, 0x00,0x159,0x16f, 0xfa,0x171, 0xfc, 0xfd,0x163, 0x00,
      };
      return s >= 128
        ? DECODE[s - 128]
        : s;
    }
  };
  
  struct Weight
  {
    int_t Straight;
    int_t Loose;
    
    Weight()
      : Straight(1)
      , Loose(0)
    {
    }
    
    void Add(int_t weight)
    {
      Straight *= weight;
      Loose += weight;
    }
  };
  
  Weight GetCodepageWeight(const Codepage& cp, const std::string& str)
  {
    assert(!str.empty());
    Weight result;
    for (std::string::const_iterator it = str.begin(), lim = str.end(); it != lim; ++it)
    {
      result.Add(cp.GetWeight(*it));
    }
    return result;
  }

  const Codepage* GuessCodepage(const std::string& str)
  {
    static const CP866Codepage CP866;
    static const CP1251Codepage CP1251;
    static const CP1252Codepage CP1252;
    static const CP1250Codepage CP1250;
    static const std::size_t CODEPAGES_COUNT = 4;
    //assume cp1252 has higher priority
    static const Codepage* const CODEPAGES[CODEPAGES_COUNT] = {&CP866, &CP1251, &CP1250, &CP1252};
    
    std::size_t straights[CODEPAGES_COUNT];
    std::size_t looses[CODEPAGES_COUNT];
    for (std::size_t cp = 0; cp != CODEPAGES_COUNT; ++cp)
    {
      const Weight weight = GetCodepageWeight(*CODEPAGES[cp], str);
      if (weight.Straight)
      {
        straights[cp] = weight.Straight * 256 + cp;
      }
      else
      {
        straights[cp] = 0;
      }
      if (weight.Loose)
      {
        looses[cp] = weight.Loose * 256 + cp;
      }
      else
      {
        looses[cp] = 0;
      }
    }
    if (const std::size_t maxStraight = *std::max_element(straights, boost::end(straights)))
    {
      return CODEPAGES[maxStraight & 0xff];
    }
    else if (const std::size_t maxLoose = *std::max_element(looses, boost::end(looses)))
    {
      return CODEPAGES[maxLoose & 0xff];
    }
    else
    {
      return 0;
    }
  }
  
  std::string Decode(const Codepage& cp, const std::string& local)
  {
    std::string result;
    result.reserve(local.size());
    for (std::string::const_iterator it = local.begin(), lim = local.end(); it != lim; ++it)
    {
      const uint8_t sym = *it;
      if (IsAscii(sym))
      {
        result += static_cast<std::string::value_type>(sym);
      }
      else
      {
        const uint_t utf = cp.Translate(sym);
        //bbbbbaaaaaa
        //110bbbbb 10aaaaaa
        result += static_cast<std::string::value_type>(0xc0 | ((utf & 0x3c0) >> 6));
        result += static_cast<std::string::value_type>(0x80 | (utf & 0x3f));
      }
    }
    return result;
  }
}

namespace Strings
{
  std::string ToAutoUtf8(const std::string& local)
  {
    if (local.empty())
    {
      return local;
    }
    if (const Codepage* codepage = GuessCodepage(local))
    {
      return Decode(*codepage, local);
    }
    else
    {
      return local;
    }
  }
}
