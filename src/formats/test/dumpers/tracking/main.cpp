/**
 *
 * @file
 *
 * @brief  Tracking formats dumper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "dumpers.h"

#include "formats/test/utils.h"

#include "string_view.h"

#include <exception>
#include <iostream>
#include <utility>

std::ostream& StdOut = std::cout;

bool DumpDigitalStudio(const Binary::Container&);
bool DumpSoundTracker1(const Binary::Container&);
bool DumpSoundTrackerCompiled(const Binary::Container&);
bool DumpSoundTracker3(const Binary::Container&);
bool DumpSoundTrackerPro(const Binary::Container&);
bool DumpASCSoundMaster0x(const Binary::Container&);
bool DumpASCSoundMaster1x(const Binary::Container&);
bool DumpProTracker2(const Binary::Container&);
bool DumpProTracker3(const Binary::Container&);
bool DumpVortexTracker2(const Binary::Container&);
bool DumpProSoundMaker(const Binary::Container&);
bool DumpGlobalTracker(const Binary::Container&);
bool DumpProTracker1(const Binary::Container&);
bool DumpChipTracker(const Binary::Container&);
bool DumpSampleTracker(const Binary::Container&);
bool DumpProDigiTracker(const Binary::Container&);
bool DumpSQTracker(const Binary::Container&);
bool DumpProSoundCreator(const Binary::Container&);
bool DumpFastTracker(const Binary::Container&);
bool DumpETracker(const Binary::Container&);
bool DumpSQDigitalTracker(const Binary::Container&);
bool DumpTFMMusicMaker05(const Binary::Container&);
bool DumpTFMMusicMaker13(const Binary::Container&);
bool DumpDigitalMusicMaker(const Binary::Container&);
bool DumpExtremeTracker1(const Binary::Container&);

namespace
{
  constexpr std::pair<StringView, Dump::Dumper> ALL_FORMATS[] = {
      {"dst"sv, &DumpDigitalStudio},    {"st1"sv, &DumpSoundTracker1},   {"stc"sv, &DumpSoundTrackerCompiled},
      {"st3"sv, &DumpSoundTracker3},    {"stp"sv, &DumpSoundTrackerPro}, {"as0"sv, &DumpASCSoundMaster0x},
      {"asc"sv, &DumpASCSoundMaster1x}, {"pt2"sv, &DumpProTracker2},     {"pt3"sv, &DumpProTracker3},
      {"txt"sv, &DumpVortexTracker2},   {"psm"sv, &DumpProSoundMaker},   {"gtr"sv, &DumpGlobalTracker},
      {"pt1"sv, &DumpProTracker1},      {"chi"sv, &DumpChipTracker},     {"str"sv, &DumpSampleTracker},
      {"pdt"sv, &DumpProDigiTracker},   {"sqt"sv, &DumpSQTracker},       {"psc"sv, &DumpProSoundCreator},
      {"ftc"sv, &DumpFastTracker},      {"cop"sv, &DumpETracker},        {"sqd"sv, &DumpSQDigitalTracker},
      {"tf0"sv, &DumpTFMMusicMaker05},  {"tfe"sv, &DumpTFMMusicMaker13}, {"dmm"sv, &DumpDigitalMusicMaker},
      {"et1"sv, &DumpExtremeTracker1},
  };

  const Dump::Dumper* FindDumper(StringView type)
  {
    for (const auto& entry : ALL_FORMATS)
    {
      if (entry.first == type)
      {
        return &entry.second;
      }
    }
    return nullptr;
  }

  void PrintUsage()
  {
    std::cerr << "Usage: formats_test_tracking [--type <id> | <id>] <file[:offset[:size]]>\n"
                 "Available types:";
    for (const auto& entry : ALL_FORMATS)
    {
      std::cerr << ' ' << entry.first;
    }
    std::cerr << '\n';
  }
}  // namespace

int main(int argc, char* argv[])
{
  try
  {
    if (argc < 2)
    {
      PrintUsage();
      return 1;
    }

    StringView type;
    int fileArg = 1;
    if (StringView(argv[1]) == "--type")
    {
      if (argc < 4)
      {
        PrintUsage();
        return 1;
      }
      type = argv[2];
      fileArg = 3;
    }
    else if (const auto arg = StringView(argv[1]); arg.starts_with("--type="))
    {
      type = arg.substr(7);
      fileArg = 2;
    }
    else if (argc >= 3 && FindDumper(argv[1]))
    {
      type = argv[1];
      fileArg = 2;
    }

    const auto data = Test::OpenFile(argv[fileArg]);
    if (!type.empty())
    {
      const auto dumper = FindDumper(type);
      if (!dumper)
      {
        std::cerr << "Unknown type " << type << '\n';
        PrintUsage();
        return 1;
      }
      return (*dumper)(*data) ? 0 : 1;
    }

    for (const auto& entry : ALL_FORMATS)
    {
      if ((*entry.second)(*data))
      {
        return 0;
      }
    }
    std::cerr << "Unsupported tracking format\n";
    return 1;
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
