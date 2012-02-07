#include <format.h>
#include <logging.h>
#include <tools.h>
#include <types.h>
#include <binary/format.h>
#include <iostream>
#include <fstream>
#include <sstream>

namespace
{
  struct Range
  {
    uint_t Min;
    uint_t Max;
    
    Range() : Min(255), Max(0) 
    {
    }
    
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
        return Strings::Format("%1$02x-%2$02x ", Min, Max);
      }
      else if (Min == Max)
      {
        return Strings::Format("%1$02x", Min);
      }
      else
      {
        assert(!"Invalid case");
        return std::string();
      }
    }
  };
  
  struct Binary
  {
    uint_t Zeroes;
    uint_t Ones;
    
    Binary() : Zeroes(), Ones()
    {
    }
    
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
        result[sympos] = hasAny
          ? 'x'
          : (hasOne ? '1' : '0');
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
    
    char ToHex(uint_t val) const
    {
      assert(val <= 15);
      return val >= 10 ? (val - 10 + 'a') : (val + '0');
    }
  };
  
  class CumulativeFormat
  {
  public:
    CumulativeFormat()
      : Ranges()
      , Binaries()
    {
    }

    bool Add(const Dump& data)
    {
      const std::size_t newSize = data.size();
      Resize(newSize);
      Apply(data);
      CutTail();
      return !Ranges.empty();
    }
    
    std::string ToString() const
    {
      Log::Debug("", "Result has %1% entries", Ranges.size());
      const std::size_t ranges = std::count_if(Ranges.begin(), Ranges.end(), std::mem_fun_ref(&Range::IsThis));
      if (ranges == Ranges.size())
      {
        Log::Debug("", "Using ranges format");
        return HomogeniousToString(Ranges);
      }
      const std::size_t binaries = std::count_if(Binaries.begin(), Binaries.end(), std::mem_fun_ref(&Binary::IsThis));
      if (binaries == Binaries.size())
      {
        Log::Debug("", "Using binary format");
        return HomogeniousToString(Binaries);
      }
      Log::Debug("", "Using mixed format");
      return HeterogeniousToString();
    }
  private:
    void Resize(std::size_t len)
    {
      if (Ranges.empty() || len < Ranges.size())
      {
        Log::Debug("", "Set size to %1%", len);
        Ranges.resize(len);
        Binaries.resize(len);
      }
    }
    
    void Apply(const Dump& data)
    {
      const std::size_t toApply = std::min(data.size(), Ranges.size());
      for (std::size_t idx = 0; idx < toApply; ++idx)
      {
        const uint8_t val = data[idx];
        Ranges[idx].Apply(val);
        Binaries[idx].Apply(val);
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
      for (typename std::vector<T>::const_iterator it = vals.begin(), lim = vals.end(); it != lim; ++it)
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
        const Binary& bin = Binaries[idx];
        Log::Debug("", "Range=(%1%..%2%) Bin=(%3$02x/%4$02x)", rng.Min, rng.Max, bin.Zeroes, bin.Ones);
        if (!rng.IsThis() && !bin.IsThis())
        {
          Log::Debug("", " any");
          ++lastAny;
        }
        else
        {
          if (lastAny)
          {
            if (lastAny > 3)
            {
              result += Strings::Format("+%1%+ ", lastAny);
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
          Log::Debug("", " range=%1% bytes bin=%2% bytes", rngWeight, binWeight);
          result += rngWeight <= binWeight
            ? rng.ToString()
            : bin.ToString();
        }
      }
      return result;
    }
  private:
    std::vector<Range> Ranges;
    std::vector<Binary> Binaries;
  };
  
  Dump Read(const std::string& name)
  {
    std::ifstream stream(name.c_str(), std::ios::binary);
    if (!stream)
    {
      throw std::runtime_error("Failed to open " + name);
    }
    stream.seekg(0, std::ios_base::end);
    const std::size_t size = stream.tellg();
    stream.seekg(0);
    Dump tmp(size);
    stream.read(safe_ptr_cast<char*>(&tmp[0]), tmp.size());
    return tmp;
  }
}

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
    for (int idx = 1; idx < argc; ++idx)
    {
      const std::string filename = argv[idx];
      const Dump& data = Read(filename);
      if (!result.Add(data))
      {
        throw std::runtime_error("Data is too different");
      }
    }
    const std::string format = result.ToString();
    std::cout << "Result:\n" << format << std::endl;
    std::cout << "Checking..." << std::endl;
    const Binary::Format::Ptr check = Binary::Format::Create(format);
    for (int idx = 1; idx < argc; ++idx)
    {
      const std::string filename = argv[idx];
      const Dump& data = Read(filename);
      if (!check->Match(&data[0], data.size()))
      {
        throw std::runtime_error(Strings::Format("Not matched for %1%", filename));
      }
    }
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
  }
}