/**
 *
 * @file
 *
 * @brief  SNDH dumper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/multitrack/sndh.h"
#include "formats/test/utils.h"

#include "strings/format.h"
#include "time/serialize.h"

namespace
{
  using namespace Formats::Multitrack;

  class SndhBuilder
    : public SNDH::Builder
    , public Formats::Chiptune::MetaBuilder
  {
  public:
    void SetProgram(StringView program) override
    {
      std::cout << "Program: " << program << std::endl;
    }

    void SetTitle(StringView title) override
    {
      std::cout << "Title: " << title << std::endl;
    }

    void SetAuthor(StringView author) override
    {
      std::cout << "Author: " << author << std::endl;
    }

    void SetStrings(const Strings::Array& strings) override
    {
      for (const auto& str : strings)
      {
        std::cout << "Strings: " << str << std::endl;
      }
    }

    void SetComment(StringView comment) override
    {
      std::cout << "Comment: " << comment << std::endl;
    }

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return *this;
    }

    void SetYear(StringView year) override
    {
      std::cout << "Year: " << year << std::endl;
    }

    void SetUnknownTag(StringView tag, StringView value) override
    {
      std::cout << tag << ": " << value << std::endl;
    }

    void SetTimer(char type, uint_t freq) override
    {
      std::cout << "Timer " << type << "=" << freq << std::endl;
    }

    void SetSubtunes(std::vector<StringView> names) override
    {
      std::cout << "Subtunes:\n";
      for (const auto& sub : names)
      {
        std::cout << "- '" << sub << "'\n";
      }
    }

    void SetDurations(std::vector<Time::Seconds> durations) override
    {
      std::cout << "Durations:\n";
      for (const auto& sub : durations)
      {
        std::cout << "- " << Time::ToString(sub) << "\n";
      }
    }
  };
}  // namespace

int main(int argc, char* argv[])
{
  try
  {
    if (argc < 2)
    {
      return 0;
    }
    const auto data = Test::OpenFile(argv[1]);
    SndhBuilder builder;
    if (const auto result = Formats::Multitrack::SNDH::Parse(*data, builder))
    {
      std::cout << "Tracks: " << result->TracksCount()
                << "\n"
                   "Start: "
                << result->StartTrackIndex()
                << "\n"
                   "Size: "
                << result->Size() << std::endl;
    }
    else
    {
      std::cout << "Failed to parse" << std::endl;
    }
    return 0;
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
    return 1;
  }
}
