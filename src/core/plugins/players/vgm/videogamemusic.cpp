/**
 *
 * @file
 *
 * @brief  VGM format support tools implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "core/plugins/players/vgm/videogamemusic.h"
// common includes
#include <byteorder.h>
// library includes
#include <binary/input_stream.h>
#include <module/players/platforms.h>
// std includes
#include <map>

namespace Module::VideoGameMusic
{
  enum DeviceType
  {
    UNUSED,
    SN76489,
    YM2413,
    YM2612,
    YM2151,
    SEGA_PCM,
    RF5C68,
    YM2203,
    YM2608,
    YM2610,
    YM3812,
    YM3526,
    Y8950,
    YMF262,
    YMF278B,
    YMF271,
    YMZ280B,
    RF5C164,
    PWM,
    AY8910,
    LR35902,
    N2A03,
    MULTI_PCM,
    UPD7759,
    OKIM6258,
    OKIM6295,
    K051649,
    K054539,
    HUC6280,
    C140,
    K053260,
    POKEY,
    QSOUND,
    SCSP,
    WONDER_SWAN,
    VSU,
    SAA1099,
    ES5503,
    ES5506,
    X1_010,
    C352,
    GA20,

    GAMEGEAR_STEREO,

    DUAL = 0x8000000,
  };

  struct DeviceTraits
  {
    uint_t Clock = 0;
    bool HasCommands = false;
  };

  struct MatchResult
  {
    uint_t AllDevices = 0;
    uint_t Declared = 0;
    uint_t ClockMatched = 0;
    uint_t HasCommands = 0;
  };

  struct PlatformTrait
  {
    const StringView Id;

    struct DeviceTrait
    {
      uint_t Id;
      uint_t Clocks[6];
    };
    DeviceTrait Devices[4];

    bool HasDevice(uint_t id, uint_t clock) const
    {
      for (const auto& dev : Devices)
      {
        if (dev.Id == UNUSED)
        {
          break;
        }
        else if (dev.Id == id)
        {
          for (const auto& cl : dev.Clocks)
          {
            if (cl == clock)
            {
              return true;
            }
            else if (!cl)
            {
              break;
            }
          }
        }
      }
      return false;
    }

    bool HasDevice(uint_t id) const
    {
      for (const auto& dev : Devices)
      {
        if (dev.Id == UNUSED)
        {
          break;
        }
        else if (dev.Id == id)
        {
          return true;
        }
      }
      return false;
    }
  };

  // clang-format off
    static const PlatformTrait::DeviceTrait SEGA_SN76489 =
    {
      SN76489,
      { //ntsc
        3579545,
        //pal
        3546895,
        //fixed ntsc
        3579540,
        //fixed pal
        3546893
      }
    };
    
    //http://www.progettoemma.net/mess/sysset.php
    const PlatformTrait PLATFORMS[] = 
    {
      //SEGA family
      //http://www.progettoemma.net/mess/system.php?machine=sg1000
      {
        Platforms::SG_1000,
        {
          SEGA_SN76489
        }
      },
      //http://www.progettoemma.net/mess/system.php?machine=smsj
      //http://www.progettoemma.net/mess/system.php?machine=sg1000m3
      {
        Platforms::SEGA_MASTER_SYSTEM,
        {
          SEGA_SN76489,
          {YM2413, {3579545, 3579540}},
        }
      },
      //fork of SMS
      //http://www.progettoemma.net/mess/system.php?machine=gamegear
      {
        Platforms::GAME_GEAR,
        {
          {SN76489, {3579545, 3579540}},
          {GAMEGEAR_STEREO, {0}},
        }
      },
      //http://www.progettoemma.net/mess/system.php?machine=genesis
      //TODO: http://www.progettoemma.net/mess/system.php?machine=32x_scd
      {
        Platforms::SEGA_GENESIS,
        {
          {SN76489,
            { //32x/ntsc
              3579545,
              //megadrive/pal
              3546894,
            }
          },
          {YM2612,
            { //32x/ntsc
              7670453,
              //megadrive/pal
              7600489,
            }
          },
        }
      },
      //http://www.progettoemma.net/mess/system.php?machine=ngp
      {
        Platforms::NEO_GEO_POCKET,
        {
          {SN76489, {3072000}},
          {SN76489 | DUAL, {3072000}},
        }
      },
      {
        Platforms::NEO_GEO,
        {
          {YM2610,
            { //cdz us/cd/cdz ja/aes
              8000000,
            }
          },
        }
      },
      //Nintendo Family
      //http://www.progettoemma.net/mess/system.php?machine=nes
      {
        Platforms::NINTENDO_ENTERTAINMENT_SYSTEM,
        {
          {N2A03,
            { //ntsc
              1789772, 1789773,
              //pal
              1662607,
              //dendy
              1773447, 1773448,
            }
          },
        }
      },
      //http://www.progettoemma.net/mess/system.php?machine=pce
      {
        Platforms::PC_ENGINE,
        {
          {HUC6280, {3579545}},
        }
      },
      //http://www.progettoemma.net/mess/system.php?machine=gameboy
      {
        Platforms::GAME_BOY,
        {
          {LR35902,
            { //gb
              4194304,
              //subergb
              4295454,
            }
          },
        }
      },
      //http://www.progettoemma.net/mess/system.php?machine=spec128
      {
        Platforms::ZX_SPECTRUM,
        {
          {AY8910,
            { //original
              1773400,
              //pentagon/1024
              1750000,
            }
          },
        }
      },
      //http://www.progettoemma.net/mess/system.php?machine=cpc464
      {
        Platforms::AMSTRAD_CPC,
        {
          {AY8910, {1000000}},
        }
      },
      //http://www.progettoemma.net/mess/system.php?machine=a5200
      {
        Platforms::ATARI,
        {
          {POKEY, {1789790}},
        }
      },
      {
        Platforms::MSX,
        {
          {AY8910, {1789772}},
          {K051649, {1789772}},
          //single chip versions
          {YM2413, {3579545}},
          {YMF278B, {33868800}},
        }
      },
      //http://www.progettoemma.net/mess/system.php?machine=bbcb
      {
        Platforms::BBC_MICRO,
        {
          {SN76489, {4000000}},
        }
      },
      //http://www.progettoemma.net/mess/system.php?machine=vectrex
      {
        Platforms::VECTREX,
        {
          {AY8910, {1500000}},
        }
      }
    };
  // clang-format on

  class PlatformsSet
  {
  public:
    void Intersect(PlatformsSet rh)
    {
      Value &= rh.Value;
    }

    PlatformsSet Intersect(PlatformsSet rh) const
    {
      return PlatformsSet(Value & rh.Value);
    }

    bool IsEmpty() const
    {
      return Value == 0;
    }

    const StringView FindSingleName() const
    {
      uint_t mask = 1;
      for (const auto& plat : PLATFORMS)
      {
        if (Value & mask)
        {
          return Value == mask ? plat.Id : StringView();
        }
        mask <<= 1;
      }
      return {};
    }

    StringView GetBaseName() const
    {
      uint_t mask = 1;
      for (const auto& plat : PLATFORMS)
      {
        if (Value & mask)
        {
          return plat.Id;
        }
        mask <<= 1;
      }
      return {};
    }

    static PlatformsSet All()
    {
      return PlatformsSet(~uint_t(0));
    }

    static PlatformsSet ForDevice(uint_t id, uint_t clock)
    {
      uint_t mask = 1;
      uint_t val = 0;
      for (const auto& plat : PLATFORMS)
      {
        if (plat.HasDevice(id, clock))
        {
          val |= mask;
        }
        mask <<= 1;
      }
      return PlatformsSet(val);
    }

    static PlatformsSet ForDevice(uint_t id)
    {
      uint_t mask = 1;
      uint_t val = 0;
      for (const auto& plat : PLATFORMS)
      {
        if (plat.HasDevice(id))
        {
          val |= mask;
        }
        mask <<= 1;
      }
      return PlatformsSet(val);
    }

  private:
    PlatformsSet() = default;
    explicit PlatformsSet(uint_t val)
      : Value(val)
    {}

  private:
    uint_t Value = 0;
  };

  class PlatformDetector
  {
  public:
    explicit PlatformDetector(Binary::View data)
      : Input(data)
    {
      Input.Seek(0x8);
      const auto vers = Input.PeekRawData(4);
      Version = (vers[0] & 15) + 10 * (vers[0] >> 4) + 100 * (vers[1] & 15) + 1000 * (vers[1] >> 4);

      {
        const std::size_t offsetPos = 0x34;
        Input.Seek(offsetPos);
        const auto vgmOffset = ReadDword();
        DataOffset = vgmOffset ? vgmOffset + offsetPos : 0x40;
      }
      {
        const std::size_t offsetPos = 0x14;
        Input.Seek(offsetPos);
        const auto gd3Offset = ReadDword();
        TagsOffset = gd3Offset ? gd3Offset + offsetPos : 0;
      }
    }

    StringView GetResult()
    {
      const auto fromTags = GetFromTags();
      if (fromTags.size())
      {
        return fromTags;
      }
      else
      {
        const auto byClocks = GuessByClocks();
        const auto singleName = byClocks.FindSingleName();
        if (singleName.size())
        {
          return singleName;
        }
        else
        {
          const auto byCommands = GuessByCommands();
          if (byCommands.IsEmpty())
          {
            return byClocks.GetBaseName();
          }
          const auto singleName = byCommands.FindSingleName();
          if (singleName.size())
          {
            return singleName;
          }
          else
          {
            const auto match = byClocks.Intersect(byCommands);
            if (match.IsEmpty())
            {
              return byCommands.GetBaseName();
            }
            else
            {
              return match.GetBaseName();
            }
          }
        }
      }
    }

  private:
    StringView GetFromTags()
    {
      if (!TagsOffset)
      {
        return {};
      }
      try
      {
        Input.Seek(TagsOffset);
        Require(ReadDword() == 0x20336447);
        Input.Skip(8);
        ReadUtf16();  // title en
        ReadUtf16();  // title ja
        ReadUtf16();  // game en
        ReadUtf16();  // game ja
        const auto sys = ReadUtf16();
        return ConvertPlatform(String(sys.begin(), sys.end()));
      }
      catch (const std::exception&)
      {}
      return {};
    }

    uint8_t ReadByte()
    {
      return Input.ReadByte();
    }

    uint32_t ReadDword()
    {
      return Input.Read<le_uint32_t>();
    }

    basic_string_view<le_uint16_t> ReadUtf16()
    {
      const auto symbolsAvailable = Input.GetRestSize() / sizeof(le_uint16_t);
      const auto* begin = safe_ptr_cast<const le_uint16_t*>(Input.PeekRawData(symbolsAvailable * sizeof(le_uint16_t)));
      auto end = std::find(begin, begin + symbolsAvailable, 0);
      Require(end != begin + symbolsAvailable);
      Input.Skip((end + 1 - begin) * sizeof(*begin));
      return basic_string_view<le_uint16_t>(begin, end);
    }

    static StringView ConvertPlatform(StringView str)
    {
      struct PlatformName
      {
        const StringView Name;
        const StringView Id;
      };

      // clang-format off
        static const PlatformName PLATFORMS[] =
        {
          {"Sega Master System"_sv, Platforms::SEGA_MASTER_SYSTEM},
          {"Sega Game Gear"_sv, Platforms::GAME_GEAR},
          {"Sega Mega Drive"_sv, Platforms::SEGA_GENESIS},
          {"Sega Genesis"_sv, Platforms::SEGA_GENESIS},
          {"Sega Game 1000"_sv, Platforms::SG_1000},
          {"Sega Computer 3000"_sv, Platforms::SEGA_MASTER_SYSTEM},
          {"Sega System 16"_sv, Platforms::SEGA_MASTER_SYSTEM},
          {"Coleco"_sv, Platforms::COLECOVISION},
          {"BBC M"_sv, Platforms::BBC_MICRO},
        };
      // clang-format on
      for (const auto& pair : PLATFORMS)
      {
        const auto prefixSize = pair.Name.size();
        if (0 == str.compare(0, prefixSize, pair.Name))
        {
          return pair.Id;
        }
      }
      return {};
    }

    PlatformsSet GuessByClocks()
    {
      AnalyzeClocks();
      auto result = PlatformsSet::All();
      for (const auto& dev : Traits)
      {
        result.Intersect(PlatformsSet::ForDevice(dev.first, dev.second.Clock));
      }
      return result;
    }

    PlatformsSet GuessByCommands()
    {
      AnalyzeCommands();
      auto result = PlatformsSet::All();
      for (const auto& dev : Traits)
      {
        if (dev.second.HasCommands)
        {
          result.Intersect(PlatformsSet::ForDevice(dev.first));
        }
      }
      return result;
    }

    void AnalyzeClocks()
    {
      struct DeviceDesc
      {
        DeviceType Id;
        std::size_t ClockOffset;
      };

      // clang-format off
        static const DeviceDesc SIMPLE_DEVICES[] =
        {
          {SEGA_PCM, 0x38},
          {RF5C68, 0x40},
          {RF5C164, 0x6c},
          {PWM, 0x70},
          {QSOUND, 0xb4},
        };
        
        static const DeviceDesc DUAL_DEVICES[] =
        {
          {SN76489, 0x0c},
          {YM2413, 0x10},
          {YM2612, 0x2c},
          {YM2151, 0x30},
          {YM2203, 0x44},
          {YM2608, 0x48},
          {YM2610, 0x4c},
          {YM3812, 0x50},
          {YM3526, 0x54},
          {Y8950, 0x58},
          {YMZ280B, 0x68},
          {YMF262, 0x5c},
          {YMF278B, 0x60},
          {YMF271, 0x64},
          {AY8910, 0x74},
          {LR35902, 0x80},
          {N2A03, 0x84},
          {MULTI_PCM, 0x88},
          {UPD7759, 0x8c},
          {OKIM6258, 0x90},
          {OKIM6295, 0x98},
          {K051649, 0x9c},
          {K054539, 0xa0},
          {HUC6280, 0xa4},
          {C140, 0xa8},
          {K053260, 0xac},
          {POKEY, 0xb0},
          {SCSP, 0xb8},
          {WONDER_SWAN, 0xc0},
          {VSU, 0xc4},
          {SAA1099, 0xc8},
          {ES5503, 0xcc},
          {ES5506, 0xd0},
          {X1_010, 0xd8},
          {C352, 0xdc},
          {GA20, 0xe0}
        };
      // clang-format on

      for (const auto& dev : SIMPLE_DEVICES)
      {
        if (const auto data = ReadClock(dev.ClockOffset))
        {
          AddSimpleDevice(dev.Id, data);
        }
      }
      for (const auto& dev : DUAL_DEVICES)
      {
        if (const auto data = ReadClock(dev.ClockOffset))
        {
          AddDevice(dev.Id, data);
        }
      }
    }

    void AnalyzeCommands()
    {
      Input.Seek(DataOffset);
      try
      {
        for (;;)
        {
          const auto code = ReadByte();
          if (code == 0x66)
          {
            break;
          }
          Require(ParseFixedCommand(code) || ParseDataBlock(code) || ParseRamWrite(code) || ParseBuggyCommand(code));
        }
      }
      catch (const std::exception&)
      {}
    }

    uint32_t ReadClock(std::size_t offset)
    {
      if (offset + sizeof(uint32_t) <= DataOffset)
      {
        Input.Seek(offset);
        return ReadDword();
      }
      else
      {
        return 0;
      }
    }

    void AddDevice(DeviceType id, uint32_t data)
    {
      auto& trait = Traits[id];
      trait.Clock = data & 0x3fffffff;
      if (data & 0x40000000)
      {
        auto& dualTrait = Traits[id | DUAL];
        dualTrait.Clock = trait.Clock;
      }
    }

    void AddSimpleDevice(DeviceType id, uint32_t data)
    {
      auto& trait = Traits[id];
      trait.Clock = data;
    }

    struct FixedCmd
    {
      const uint8_t CodeMin;
      const uint8_t CodeMax;
      const std::size_t SequentSize;
      DeviceType Device;

      bool Match(uint8_t code) const
      {
        return code >= CodeMin && code <= CodeMax;
      }
    };

    bool ParseFixedCommand(uint8_t code)
    {
      if (code == 0x4f)
      {
        // assume that else Game Gear is backward compatible with Sega Master System
        const auto param = ReadByte();
        if (param != 0xff)
        {
          AddCmd(GAMEGEAR_STEREO);
        }
        return true;
      }

      // clang-format off
        static const FixedCmd SIMPLE_COMMANDS[] =
        {
          {0x50, 0x50, 1, SN76489},
          {0x61, 0x61, 2, UNUSED},
          {0x62, 0x63, 0, UNUSED},
          {0x64, 0x64, 3, UNUSED},
          {0x70, 0x7f, 0, UNUSED},
          {0xb0, 0xb0, 2, RF5C68},
          {0xb1, 0xb1, 2, RF5C164},
          {0xb2, 0xb2, 2, PWM},
          {0xc0, 0xc0, 3, SEGA_PCM},
          {0xc1, 0xc1, 3, RF5C68},
          {0xc2, 0xc2, 3, RF5C164},
          {0xc4, 0xc4, 3, QSOUND},
          {0xc9, 0xcf, 3, UNUSED},
          {0xd7, 0xdf, 3, UNUSED},
          {0xe0, 0xe0, 4, UNUSED},
          {0xe2, 0xff, 4, UNUSED},
          {0xd2, 0xd2, 3, UNUSED},//scc1?
          {0x80, 0x8f, 0, YM2612},
          //dac control
          {0x90, 0x92, 4, UNUSED},
          {0x93, 0x93, 10, UNUSED},
          {0x94, 0x94, 1, UNUSED},
          {0x95, 0x95, 4, UNUSED},
        };
      // clang-format on

      for (const auto& cmd : SIMPLE_COMMANDS)
      {
        if (cmd.Match(code))
        {
          AddCmd(cmd.Device);
          return Skip(cmd);
        }
      }

      static const FixedCmd DUAL_COMMANDS[] = {
          {0x30, 0x3f, 1, SN76489},
      };

      for (const auto& cmd : DUAL_COMMANDS)
      {
        if (cmd.Match(code))
        {
          AddDualCmd(cmd.Device);
          return Skip(cmd);
        }
      }

      // clang-format off
        static const FixedCmd DUAL_YM_COMMANDS[] =
        {
          {0x51, 0x51, 2, YM2413},
          {0x52, 0x53, 2, YM2612},
          {0x54, 0x54, 2, YM2151},
          {0x55, 0x55, 2, YM2203},
          {0x56, 0x57, 2, YM2608},
          {0x58, 0x59, 2, YM2610},
          {0x5a, 0x5a, 2, YM3812},
          {0x5b, 0x5b, 2, YM3526},
          {0x5c, 0x5c, 2, Y8950},
          {0x5d, 0x5d, 2, YMZ280B},
          {0x5e, 0x5f, 2, YMF262},
        };
      // clang-format on

      for (const auto& cmd : DUAL_YM_COMMANDS)
      {
        if (cmd.Match(code))
        {
          AddCmd(cmd.Device);
          return Skip(cmd);
        }
        else if (cmd.Match(code - 0x50))
        {
          AddDualCmd(cmd.Device);
          return Skip(cmd);
        }
      }

      // clang-format off
        static const FixedCmd DUAL_PARAMETER_COMMANDS[] =
        {
          {0xa0, 0xa0, 2, AY8910},
          {0xb3, 0xb3, 2, LR35902},
          {0xb4, 0xb4, 2, N2A03},
          {0xb5, 0xb5, 2, MULTI_PCM},
          {0xb6, 0xb6, 2, UPD7759},
          {0xb7, 0xb7, 2, OKIM6258},
          {0xb8, 0xb8, 2, OKIM6295},
          {0xb9, 0xb9, 2, HUC6280},
          {0xba, 0xba, 2, K053260},
          {0xbb, 0xbb, 2, POKEY},
          {0xbc, 0xbc, 2, WONDER_SWAN},
          {0xbd, 0xbd, 2, SAA1099},
          {0xbe, 0xbe, 2, ES5506},
          {0xbf, 0xbf, 2, GA20},
          {0xc3, 0xc3, 3, MULTI_PCM},
          {0xc5, 0xc5, 3, SCSP},
          {0xc6, 0xc6, 3, WONDER_SWAN},
          {0xc7, 0xc7, 3, VSU},
          {0xc8, 0xc8, 3, X1_010},
          {0xd0, 0xd0, 3, YMF278B},
          {0xd1, 0xd1, 3, YMF271},
          {0xd3, 0xd3, 3, K054539},
          {0xd4, 0xd4, 3, C140},
          {0xd5, 0xd5, 3, ES5503},
          {0xd6, 0xd6, 3, ES5506},
          {0xe1, 0xe1, 4, C352},
        };
      // clang-format on

      for (const auto& cmd : DUAL_PARAMETER_COMMANDS)
      {
        if (cmd.Match(code))
        {
          const auto param1 = ReadByte();
          Input.Skip(-1);
          if (param1 >= 0x80)
          {
            AddDualCmd(cmd.Device);
          }
          else
          {
            AddCmd(cmd.Device);
          }
          return Skip(cmd);
        }
      }
      return false;
    }

    void AddCmd(DeviceType id)
    {
      if (id != UNUSED)
      {
        Traits[id].HasCommands = true;
      }
    }

    void AddDualCmd(DeviceType id)
    {
      Traits[id | DUAL].HasCommands = true;
    }

    bool Skip(const FixedCmd& cmd)
    {
      Input.Skip(cmd.SequentSize);
      return true;
    }

    bool ParseDataBlock(uint8_t code)
    {
      if (code != 0x67)
      {
        return false;
      }
      Input.Skip(1);
      const auto type = ReadByte();
      const auto dev = GetDeviceTypeByBlockType(type);
      AddCmd(dev);
      const auto size = ReadDword() & 0x7fffffff;
      Input.Skip(size);
      return true;
    }

    bool ParseRamWrite(uint8_t code)
    {
      if (code != 0x68)
      {
        return false;
      }
      Input.Skip(1);
      const auto type = ReadByte();
      const auto dev = GetDeviceTypeByBlockType(type);
      AddCmd(dev);
      Input.Skip(6);
      const auto size = (ReadDword() - 1) & 0xffffff;
      Input.Skip(size);
      return true;
    }

    static DeviceType GetDeviceTypeByBlockType(uint8_t type)
    {
      static const DeviceType STREAMS[64] = {YM2612, RF5C68, RF5C164, PWM, OKIM6258, HUC6280, SCSP, N2A03};

      static const DeviceType DUMPS[64] = {SEGA_PCM, YM2608, YM2610,    YM2610,  YMF278B,  YMF271,  YMZ280B,
                                           YMF278B,  Y8950,  MULTI_PCM, UPD7759, OKIM6295, K054539, C140,
                                           K053260,  QSOUND, ES5506,    X1_010,  C352,     GA20};

      static const DeviceType WRITES[64] = {RF5C68, RF5C164, N2A03, SCSP, ES5503};

      if (type < 0x40)
      {
        return STREAMS[type];
      }
      else if (type < 0x7f)
      {
        return STREAMS[type - 0x40];
      }
      else if (type == 0x7f)
      {
        return UNUSED;
      }
      else if (type < 0xc0)
      {
        return DUMPS[type - 0x80];
      }
      else
      {
        return WRITES[type - 0xc0];
      }
    }

    bool ParseBuggyCommand(uint8_t code)
    {
      if (code >= 0x40 && code <= 0x4e)
      {
        Input.Skip(1 + (Version >= 160));
        return true;
      }
      return false;
    }

  private:
    Binary::DataInputStream Input;
    uint_t Version = 0;
    std::size_t DataOffset = 0;
    std::size_t TagsOffset;
    std::map<uint_t, DeviceTraits> Traits;
  };

  StringView DetectPlatform(Binary::View data)
  {
    PlatformDetector detector(data);
    return detector.GetResult();
  }
}  // namespace Module::VideoGameMusic
