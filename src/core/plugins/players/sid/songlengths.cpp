/**
* 
* @file
*
* @brief  Song length database implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "songlengths.h"
//common includes
#include <contract.h>
//std includes
#include <cstring>
#include <cctype>
//boost includes
#include <boost/array.hpp>
#include <boost/range/end.hpp>

namespace Module
{
namespace Sid
{
  typedef boost::array<uint8_t, 16> MD5Hash;
  
  struct SongEntry
  {
    const MD5Hash Hash;
    const unsigned Seconds;
    
    bool operator < (const MD5Hash& hash) const
    {
      return Hash < hash;
    }
  };
  
  const SongEntry SONGS[] =
  {
#include "songlengths_db.inc"
  };

  typedef Time::Stamp<uint_t, 1> Seconds;

  const Seconds DEFAULT_SONG_LENGTH(120);

  uint8_t Hex2Bin(char val)
  {
    Require(std::isxdigit(val));
    return std::isdigit(val)
      ? val - '0'
      : std::toupper(val) - 'A' + 10;
  }

  uint8_t Hex2Bin(char hi, char lo)
  {
    return Hex2Bin(hi) * 16 + Hex2Bin(lo);
  }

  MD5Hash HashFromString(const char* str)
  {
    MD5Hash result;
    Require(std::strlen(str) == 2 * result.size());
    for (MD5Hash::iterator it = result.begin(), lim = result.end(); it != lim; ++it)
    {
      *it = Hex2Bin(str[0], str[1]);
      str += 2;
    }
    return result;
  }

  TimeType GetSongLength(const char* md5digest, uint_t idx)
  {
    const MD5Hash hash = HashFromString(md5digest);
    const SongEntry* const end = boost::end(SONGS);
    const SongEntry* const lower = std::lower_bound(SONGS, end, hash);
    if (lower + idx < end && lower->Hash == hash)
    {
      const SongEntry* const entry = lower + idx;
      if (lower->Hash == entry->Hash)
      {
        return Seconds(entry->Seconds);
      }
    }
    return DEFAULT_SONG_LENGTH;
  }
}//namespace Sid
}//namespace Module
