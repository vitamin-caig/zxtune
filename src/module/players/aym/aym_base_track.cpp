/**
 *
 * @file
 *
 * @brief  AYM-based track chiptunes support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "module/players/aym/aym_base_track.h"
// common includes
#include <make_ptr.h>
// library includes
#include <math/numeric.h>
#include <parameters/tracking_helper.h>
// std includes
#include <utility>

namespace Module::AYM
{
  class TrackDataIterator : public DataIterator
  {
  public:
    TrackDataIterator(TrackParameters::Ptr trackParams, TrackStateIterator::Ptr delegate, DataRenderer::Ptr renderer)
      : Params(std::move(trackParams))
      , Delegate(std::move(delegate))
      , State(Delegate->GetStateObserver())
      , Render(std::move(renderer))
    {}

    void Reset() override
    {
      Params.Reset();
      Delegate->Reset();
      Render->Reset();
    }

    void NextFrame() override
    {
      Delegate->NextFrame();
    }

    Module::State::Ptr GetStateObserver() const override
    {
      return State;
    }

    Devices::AYM::Registers GetData() const override
    {
      SynchronizeParameters();
      TrackBuilder builder(Table);
      Render->SynthesizeData(*State, builder);
      return builder.GetResult();
    }

  private:
    void SynchronizeParameters() const
    {
      if (Params.IsChanged())
      {
        Params->FreqTable(Table);
      }
    }

  private:
    Parameters::TrackingHelper<AYM::TrackParameters> Params;
    const TrackStateIterator::Ptr Delegate;
    const TrackModelState::Ptr State;
    const AYM::DataRenderer::Ptr Render;
    mutable FrequencyTable Table;
  };

  void ChannelBuilder::SetTone(int_t halfTones, int_t offset)
  {
    const int_t halftone = Math::Clamp<int_t>(halfTones, 0, static_cast<int_t>(Table.size()) - 1);
    const uint_t tone = (Table[halftone] + offset) & 0xfff;
    SetTone(tone);
  }

  void ChannelBuilder::SetTone(uint_t tone)
  {
    using namespace Devices::AYM;
    const uint_t reg = Registers::TONEA_L + 2 * Channel;
    Data[static_cast<Registers::Index>(reg)] = static_cast<uint8_t>(tone & 0xff);
    Data[static_cast<Registers::Index>(reg + 1)] = static_cast<uint8_t>(tone >> 8);
  }

  void ChannelBuilder::SetLevel(int_t level)
  {
    using namespace Devices::AYM;
    const uint_t reg = Registers::VOLA + Channel;
    Data[static_cast<Registers::Index>(reg)] = static_cast<uint8_t>(Math::Clamp<int_t>(level, 0, 15));
  }

  void ChannelBuilder::DisableTone()
  {
    Data[Devices::AYM::Registers::MIXER] |= (Devices::AYM::Registers::MASK_TONEA << Channel);
  }

  void ChannelBuilder::EnableEnvelope()
  {
    using namespace Devices::AYM;
    const uint_t reg = Registers::VOLA + Channel;
    Data[static_cast<Registers::Index>(reg)] |= Registers::MASK_ENV;
  }

  void ChannelBuilder::DisableNoise()
  {
    Data[Devices::AYM::Registers::MIXER] |= (Devices::AYM::Registers::MASK_NOISEA << Channel);
  }

  void TrackBuilder::SetNoise(uint_t level)
  {
    Data[Devices::AYM::Registers::TONEN] = static_cast<uint8_t>(level & 31);
  }

  void TrackBuilder::SetEnvelopeType(uint_t type)
  {
    Data[Devices::AYM::Registers::ENV] = static_cast<uint8_t>(type);
  }

  void TrackBuilder::SetEnvelopeTone(uint_t tone)
  {
    Data[Devices::AYM::Registers::TONEE_L] = static_cast<uint8_t>(tone & 0xff);
    Data[Devices::AYM::Registers::TONEE_H] = static_cast<uint8_t>(tone >> 8);
  }

  uint_t TrackBuilder::GetFrequency(int_t halfTone) const
  {
    return Table[halfTone];
  }

  int_t TrackBuilder::GetSlidingDifference(int_t halfToneFrom, int_t halfToneTo) const
  {
    const int_t halfFrom = Math::Clamp<int_t>(halfToneFrom, 0, static_cast<int_t>(Table.size()) - 1);
    const int_t halfTo = Math::Clamp<int_t>(halfToneTo, 0, static_cast<int_t>(Table.size()) - 1);
    const int_t toneFrom = Table[halfFrom];
    const int_t toneTo = Table[halfTo];
    return toneTo - toneFrom;
  }

  DataIterator::Ptr CreateDataIterator(AYM::TrackParameters::Ptr trackParams, TrackStateIterator::Ptr iterator,
                                       DataRenderer::Ptr renderer)
  {
    return MakePtr<TrackDataIterator>(std::move(trackParams), std::move(iterator), std::move(renderer));
  }
}  // namespace Module::AYM
