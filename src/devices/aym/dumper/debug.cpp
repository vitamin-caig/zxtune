/**
 *
 * @file
 *
 * @brief  Debug stream dumper implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "devices/aym/dumper/dump_builder.h"

#include "binary/data_builder.h"

#include "make_ptr.h"
#include "string_view.h"

namespace
{
  inline char HexSymbol(uint8_t sym)
  {
    return sym >= 10 ? 'A' + sym - 10 : '0' + sym;
  }
}  // namespace

namespace Devices::AYM
{
  class DebugDumpBuilder : public FramedDumpBuilder
  {
  public:
    void Initialize() override
    {
      static const char HEADER[] = "000102030405060708090a0b0c0d";
      AddLine(HEADER);
      FrameNumber = 0;
    }

    Binary::Data::Ptr GetResult() override
    {
      return Data.CaptureResult();
    }

    void WriteFrame(uint_t framesPassed, const Registers& /*state*/, const Registers& update) override
    {
      assert(framesPassed);
      for (uint_t skips = 0; skips < framesPassed - 1; ++skips)
      {
        AddNochangesMessage();
      }
      String str(Registers::TOTAL * 2, ' ');
      for (Registers::IndicesIterator it(update); it; ++it)
      {
        const uint8_t val = update[*it];
        str[*it * 2 + 0] = HexSymbol(val >> 4);
        str[*it * 2 + 1] = HexSymbol(val & 15);
      }
      AddLine(str);
    }

  private:
    void AddNochangesMessage()
    {
      AddLine("=");
    }

    void AddLine(StringView str)
    {
      Data.AddCString(str);
      Data.Get<char>(Data.Size() - 1) = '\n';
      ++FrameNumber;
    }

  private:
    Binary::DataBuilder Data;
    uint_t FrameNumber = 0;
  };

  Dumper::Ptr CreateDebugDumper(const DumperParameters& params)
  {
    auto builder = MakePtr<DebugDumpBuilder>();
    return CreateDumper(params, std::move(builder));
  }
}  // namespace Devices::AYM
