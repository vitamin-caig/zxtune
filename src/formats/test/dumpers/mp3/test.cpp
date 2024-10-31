/**
 *
 * @file
 *
 * @brief  MP3 dumper
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/builder_meta.h"
#include "formats/chiptune/music/mp3.h"
#include "formats/test/utils.h"

#include "binary/view.h"
#include "formats/chiptune.h"
#include "strings/array.h"
#include "strings/format.h"
#include "time/duration.h"
#include "time/instant.h"

#include "string_view.h"

#include <exception>
#include <iostream>
#include <memory>

namespace
{
  using namespace Formats::Chiptune;

  class Mp3Builder
    : public Mp3::Builder
    , public MetaBuilder
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

    void SetPicture(Binary::View content) override
    {
      std::cout << "Picture: " << content.Size() << " bytes" << std::endl;
    }

    MetaBuilder& GetMetaBuilder() override
    {
      return *this;
    }

    void AddFrame(const Mp3::Frame& frame) override
    {
      std::cout << Strings::Format("Frame: @{0}(0x{0:08x})/{1} bytes {2}hz {3} samples (at {4} uS)\n",
                                   frame.Location.Offset, frame.Location.Size, frame.Properties.Samplerate,
                                   frame.Properties.SamplesCount, Start.Get());
      Start += Time::Microseconds::FromRatio(frame.Properties.SamplesCount, frame.Properties.Samplerate);
    }

  private:
    Time::AtMicrosecond Start;
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
    Mp3Builder builder;
    if (const auto result = Formats::Chiptune::Mp3::Parse(*data, builder))
    {
      std::cout << result->Size() << " bytes total" << std::endl;
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
