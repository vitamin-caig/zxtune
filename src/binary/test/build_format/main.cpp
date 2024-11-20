#include "binary/dump.h"
#include "binary/format_factories.h"
#include "debug/log.h"
#include "strings/format.h"

#include "pointers.h"
#include "types.h"

#include <algorithm>
#include <bitset>
#include <fstream>
#include <iostream>
#include <sstream>

namespace
{
  struct Range
  {
    uint_t Min = 255;
    uint_t Max = 0;

    Range() = default;

    void Apply(uint_t val)
    {
      Min = std::min(val, Min);
      Max = std::max(val, Max);
    }

    bool IsThis() const
    {
      return Min <= Max && (Max - Min < 255);
    }

    std::size_t CountCatches() const
    {
      return Max - Min + 1;
    }

    std::string ToString() const
    {
      if (Min < Max)
      {
        return Strings::Format("{0:02x}-{1:02x} ", Min, Max);
      }
      else if (Min == Max)
      {
        return Strings::Format("{:02x}", Min);
      }
      else
      {
        assert(!"Invalid case");
        return {};
      }
    }
  };

  char ToHex(uint_t val)
  {
    assert(val <= 15);
    return val >= 10 ? (val - 10 + 'a') : (val + '0');
  }

  struct BinaryMask
  {
    uint_t Zeroes = 0;
    uint_t Ones = 0;

    BinaryMask() = default;

    void Apply(uint_t val)
    {
      Zeroes |= ~val & 0xff;
      Ones |= val;
    }

    bool IsThis() const
    {
      const uint8_t fixed = Zeroes ^ Ones;
      return 0 != fixed;
    }

    std::size_t CountCatches() const
    {
      const uint_t mask = ~(Zeroes & Ones) & 0xff;
      const uint_t val = Ones & mask;
      std::size_t res = 0;
      for (std::size_t idx = 0; idx != 256; ++idx)
      {
        res += 0 != ((idx & mask) == val);
      }
      return res;
    }

    std::string ToString() const
    {
      if (IsNibbles())
      {
        return ToNibblesString();
      }
      std::string result(10, ' ');
      result[0] = '%';
      for (uint_t idx = 8; idx; --idx)
      {
        const uint_t mask = 1 << (idx - 1);
        const bool hasZero = 0 != (Zeroes & mask);
        const bool hasOne = 0 != (Ones & mask);
        const bool hasAny = hasZero && hasOne;
        assert(hasZero || hasOne);
        const std::size_t sympos = 9 - idx;
        result[sympos] = hasAny ? 'x' : (hasOne ? '1' : '0');
      }
      return result;
    }

  private:
    bool IsNibbles() const
    {
      const uint_t fixed = Zeroes ^ Ones;
      const uint_t hiNibble = fixed >> 4;
      const uint_t loNibble = fixed & 15;
      return (hiNibble == 15 || hiNibble == 0) && (loNibble == 15 || loNibble == 0);
    }

    std::string ToNibblesString() const
    {
      assert(IsNibbles());
      const uint_t fixed = Zeroes ^ Ones;
      const uint_t hiNibble = fixed >> 4;
      const uint_t loNibble = fixed & 15;
      std::string result(3, ' ');
      result[0] = hiNibble == 15 ? ToHex(Ones >> 4) : 'x';
      result[1] = loNibble == 15 ? ToHex(Ones & 15) : 'x';
      return result;
    }
  };

  struct Bitmask
  {
    std::bitset<256> Values;

    Bitmask() = default;

    void Apply(uint_t val)
    {
      Values[val] = true;
    }

    bool IsThis() const
    {
      const std::size_t count = Values.count();
      return count > 1 && count != 256;
    }

    std::string ToString() const
    {
      return Values.to_string<char, std::char_traits<char>, std::allocator<char> >() + "\n";
    }
  };

  const Debug::Stream Dbg("");

  class CumulativeFormat
  {
  public:
    CumulativeFormat() = default;

    bool Add(const Binary::Dump& data)
    {
      const std::size_t newSize = data.size();
      Resize(newSize);
      Apply(data);
      CutTail();
      return !Ranges.empty();
    }

    std::string ToString() const
    {
      Dbg("Result has {} entries", Ranges.size());
      if (std::all_of(Ranges.begin(), Ranges.end(), [](const Range& rng) { return rng.IsThis(); }))
      {
        Dbg("Using ranges format");
        return HomogeniousToString(Ranges);
      }
      if (std::all_of(Binaries.begin(), Binaries.end(), [](const BinaryMask& msk) { return msk.IsThis(); }))
      {
        Dbg("", "Using binary format");
        return HomogeniousToString(Binaries);
      }
      Dbg("Using mixed format");
      return HeterogeniousToString();
    }

    std::string ToBitString() const
    {
      return HomogeniousToString(Bitmasks);
    }

  private:
    void Resize(std::size_t len)
    {
      if (Ranges.empty() || len < Ranges.size())
      {
        Dbg("Set size to {}", len);
        Ranges.resize(len);
        Binaries.resize(len);
        Bitmasks.resize(len);
      }
    }

    void Apply(const Binary::Dump& data)
    {
      const std::size_t toApply = std::min(data.size(), Ranges.size());
      for (std::size_t idx = 0; idx < toApply; ++idx)
      {
        const uint8_t val = data[idx];
        Ranges[idx].Apply(val);
        Binaries[idx].Apply(val);
        Bitmasks[idx].Apply(val);
      }
    }

    void CutTail()
    {
      std::size_t size = Ranges.size();
      assert(size);
      for (; size; --size)
      {
        if (Ranges[size - 1].IsThis() || Binaries[size - 1].IsThis())
        {
          break;
        }
      }
      Resize(size);
    }

    template<class T>
    std::string HomogeniousToString(const std::vector<T>& vals) const
    {
      std::string result;
      for (auto it = vals.begin(), lim = vals.end(); it != lim; ++it)
      {
        result += it->ToString();
      }
      return result;
    }

    std::string HeterogeniousToString() const
    {
      std::string result;
      std::size_t lastAny = 0;
      for (std::size_t idx = 0; idx != Ranges.size(); ++idx)
      {
        const Range& rng = Ranges[idx];
        const BinaryMask& bin = Binaries[idx];
        Dbg("Range=({}..{}) Bin=({:02x}/{:02x})", rng.Min, rng.Max, bin.Zeroes, bin.Ones);
        if (!rng.IsThis() && !bin.IsThis())
        {
          Dbg(" any");
          ++lastAny;
        }
        else
        {
          if (lastAny)
          {
            if (lastAny > 3)
            {
              result += Strings::Format("+{}+ ", lastAny);
            }
            else
            {
              while (lastAny--)
              {
                result += '\?';
              }
              result += ' ';
            }
            lastAny = 0;
          }
          const std::size_t rngWeight = rng.CountCatches();
          const std::size_t binWeight = bin.CountCatches();
          Dbg(" range={} bytes bin={} bytes", rngWeight, binWeight);
          result += rngWeight <= binWeight ? rng.ToString() : bin.ToString();
        }
      }
      return result;
    }

  private:
    std::vector<Range> Ranges;
    std::vector<BinaryMask> Binaries;
    std::vector<Bitmask> Bitmasks;
  };

  Binary::Dump Read(const std::string& name)
  {
    std::ifstream stream(name.c_str(), std::ios::binary);
    if (!stream)
    {
      throw std::runtime_error("Failed to open " + name);
    }
    stream.seekg(0, std::ios_base::end);
    const std::size_t size = stream.tellg();
    stream.seekg(0);
    Binary::Dump tmp(size);
    stream.read(safe_ptr_cast<char*>(tmp.data()), tmp.size());
    return tmp;
  }
}  // namespace

int main(int argc, char* argv[])
{
  if (argc < 2)
  {
    return 0;
  }
  try
  {
    std::cout << "Collecting..." << std::endl;
    CumulativeFormat result;
    const bool bitMask = std::string(argv[1]) == "--bitmask";
    for (int idx = 1 + bitMask; idx < argc; ++idx)
    {
      const std::string filename = argv[idx];
      const auto& data = Read(filename);
      if (!result.Add(data))
      {
        throw std::runtime_error("Data is too different");
      }
    }
    if (bitMask)
    {
      std::string hi;
      std::string lo;
      for (int idx = 255; idx >= 0; --idx)
      {
        hi += ToHex(idx >> 4);
        lo += ToHex(idx & 15);
      }
      std::cout << hi << std::endl << lo << std::endl << result.ToBitString();
    }
    else
    {
      const std::string format = result.ToString();
      std::cout << "Result:\n" << format << std::endl;
      std::cout << "Checking..." << std::endl;
      const Binary::Format::Ptr check = Binary::CreateFormat(format);
      for (int idx = 1; idx < argc; ++idx)
      {
        const std::string filename = argv[idx];
        const auto& data = Read(filename);
        if (!check->Match(data))
        {
          throw std::runtime_error(Strings::Format("Not matched for {}", filename));
        }
      }
    }
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
  }
}