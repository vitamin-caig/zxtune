/**
 *
 * @file
 *
 * @brief  PSG chiptune factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/aym/psg.h"

#include "module/players/aym/aym_base.h"
#include "module/players/aym/aym_base_stream.h"
#include <formats/chiptune/aym/psg.h>
#include <module/players/properties_helper.h>

#include <make_ptr.h>

namespace Module::PSG
{
  class DataBuilder : public Formats::Chiptune::PSG::Builder
  {
  public:
    DataBuilder()
      : Data(MakePtr<AYM::MutableStreamModel>())
    {}

    void AddChunks(std::size_t count) override
    {
      Data->Append(count);
    }

    void SetRegister(uint_t reg, uint_t val) override
    {
      if (reg < Devices::AYM::Registers::TOTAL)
      {
        if (auto* regs = Data->LastFrame())
        {
          (*regs)[static_cast<Devices::AYM::Registers::Index>(reg)] = val;
        }
      }
    }

    AYM::StreamModel::Ptr CaptureResult() const
    {
      return Data->IsEmpty() ? AYM::StreamModel::Ptr() : AYM::StreamModel::Ptr(Data);
    }

  private:
    AYM::MutableStreamModel::Ptr Data;
  };

  class Factory : public AYM::Factory
  {
  public:
    AYM::Chiptune::Ptr CreateChiptune(const Binary::Container& rawData,
                                      Parameters::Container::Ptr properties) const override
    {
      DataBuilder dataBuilder;
      if (const auto container = Formats::Chiptune::PSG::Parse(rawData, dataBuilder))
      {
        if (auto data = dataBuilder.CaptureResult())
        {
          PropertiesHelper props(*properties);
          props.SetSource(*container);
          return AYM::CreateStreamedChiptune(AYM::BASE_FRAME_DURATION, std::move(data), std::move(properties));
        }
      }
      return {};
    }
  };

  Factory::Ptr CreateFactory()
  {
    return MakePtr<Factory>();
  }
}  // namespace Module::PSG
