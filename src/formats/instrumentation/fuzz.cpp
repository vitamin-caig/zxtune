#include <formats/chiptune/decoders.h>
#include <formats/packed/decoders.h>

#include <binary/container_factories.h>

#include <stdexcept>

namespace
{
  class DecodersSet
  {
  public:
    DecodersSet()
    {
      FillPacked();
      FillChiptune();
    }

    void TestPacked(const Binary::Container& rawData) const
    {
      // Test(*PackedDecoders[5], rawData);
      // return;
      for (const auto& decoder : PackedDecoders)
      {
        Test(*decoder, rawData);
      }
    }

    void TestChiptune(const Binary::Container& rawData) const
    {
      Test(*ChiptuneDecoders[51], rawData);
      return;
      for (const auto& decoder : ChiptuneDecoders)
      {
        Test(*decoder, rawData);
      }
    }

    static const DecodersSet& Instance()
    {
      static const DecodersSet INSTANCE;
      return INSTANCE;
    }

  private:
    void FillPacked()
    {
      using namespace Formats::Packed;
      PackedDecoders.push_back(CreateHobetaDecoder());
      PackedDecoders.push_back(CreateHrust1Decoder());
      PackedDecoders.push_back(CreateHrust21Decoder());
      PackedDecoders.push_back(CreateHrust23Decoder());
      // PackedDecoders.push_back(CreateFullDiskImageDecoder());
      PackedDecoders.push_back(CreateDataSquieezerDecoder());
      PackedDecoders.push_back(CreateMSPackDecoder());
      PackedDecoders.push_back(CreateTRUSHDecoder());
      PackedDecoders.push_back(CreateLZSDecoder());
      PackedDecoders.push_back(CreatePowerfullCodeDecreaser61Decoder());
      PackedDecoders.push_back(CreatePowerfullCodeDecreaser61iDecoder());
      PackedDecoders.push_back(CreatePowerfullCodeDecreaser62Decoder());
      PackedDecoders.push_back(CreateHrumDecoder());
      PackedDecoders.push_back(CreateCodeCruncher3Decoder());
      PackedDecoders.push_back(CreateCompressorCode4Decoder());
      PackedDecoders.push_back(CreateCompressorCode4PlusDecoder());
      PackedDecoders.push_back(CreateESVCruncherDecoder());
      PackedDecoders.push_back(CreateGamePackerDecoder());
      PackedDecoders.push_back(CreateGamePackerPlusDecoder());
      PackedDecoders.push_back(CreateTurboLZDecoder());
      PackedDecoders.push_back(CreateTurboLZProtectedDecoder());
      PackedDecoders.push_back(CreateCharPresDecoder());
      PackedDecoders.push_back(CreatePack2Decoder());
      PackedDecoders.push_back(CreateLZH1Decoder());
      PackedDecoders.push_back(CreateLZH2Decoder());
      PackedDecoders.push_back(CreateSna128Decoder());
      // PackedDecoders.push_back(CreateTeleDiskImageDecoder());
      PackedDecoders.push_back(CreateZ80V145Decoder());
      PackedDecoders.push_back(CreateZ80V20Decoder());
      PackedDecoders.push_back(CreateZ80V30Decoder());
      PackedDecoders.push_back(CreateMegaLZDecoder());
      PackedDecoders.push_back(CreateDSKDecoder());
      PackedDecoders.push_back(CreateGzipDecoder());
      PackedDecoders.push_back(CreateTeleDiskImageDecoder());
    }

    void FillChiptune()
    {
      using namespace Formats::Chiptune;
      //+0
      ChiptuneDecoders.push_back(CreatePSGDecoder());
      ChiptuneDecoders.push_back(CreateDigitalStudioDecoder());
      ChiptuneDecoders.push_back(CreateSoundTrackerDecoder());
      ChiptuneDecoders.push_back(CreateSoundTrackerCompiledDecoder());
      ChiptuneDecoders.push_back(CreateSoundTracker3Decoder());
      ChiptuneDecoders.push_back(CreateSoundTrackerProCompiledDecoder());
      ChiptuneDecoders.push_back(CreateASCSoundMaster0xDecoder());
      ChiptuneDecoders.push_back(CreateASCSoundMaster1xDecoder());
      ChiptuneDecoders.push_back(CreateProTracker2Decoder());
      ChiptuneDecoders.push_back(CreateProTracker3Decoder());
      //+10
      ChiptuneDecoders.push_back(CreateVortexTracker2Decoder());
      ChiptuneDecoders.push_back(CreateProSoundMakerCompiledDecoder());
      ChiptuneDecoders.push_back(CreateGlobalTrackerDecoder());
      ChiptuneDecoders.push_back(CreateProTracker1Decoder());
      ChiptuneDecoders.push_back(CreatePackedYMDecoder());
      ChiptuneDecoders.push_back(CreateYMDecoder());
      ChiptuneDecoders.push_back(CreateVTXDecoder());
      ChiptuneDecoders.push_back(CreateTFDDecoder());
      ChiptuneDecoders.push_back(CreateTFCDecoder());
      ChiptuneDecoders.push_back(CreateChipTrackerDecoder());
      //+20
      ChiptuneDecoders.push_back(CreateSampleTrackerDecoder());
      ChiptuneDecoders.push_back(CreateProDigiTrackerDecoder());
      ChiptuneDecoders.push_back(CreateSQTrackerDecoder());
      ChiptuneDecoders.push_back(CreateProSoundCreatorDecoder());
      ChiptuneDecoders.push_back(CreateFastTrackerDecoder());
      ChiptuneDecoders.push_back(CreateETrackerDecoder());
      ChiptuneDecoders.push_back(CreateSQDigitalTrackerDecoder());
      ChiptuneDecoders.push_back(CreateTFMMusicMaker05Decoder());
      ChiptuneDecoders.push_back(CreateTFMMusicMaker13Decoder());
      ChiptuneDecoders.push_back(CreateDigitalMusicMakerDecoder());
      //+30
      ChiptuneDecoders.push_back(CreateTurboSoundDecoder());
      ChiptuneDecoders.push_back(CreateExtremeTracker1Decoder());
      ChiptuneDecoders.push_back(CreateAYCDecoder());
      ChiptuneDecoders.push_back(CreateSPCDecoder());
      ChiptuneDecoders.push_back(CreateMultiTrackContainerDecoder());
      ChiptuneDecoders.push_back(CreateAYEMULDecoder());
      ChiptuneDecoders.push_back(CreateVideoGameMusicDecoder());
      ChiptuneDecoders.push_back(CreateGYMDecoder());
      ChiptuneDecoders.push_back(CreateAbyssHighestExperienceDecoder());
      ChiptuneDecoders.push_back(CreateKSSDecoder());
      //+40
      ChiptuneDecoders.push_back(CreateHivelyTrackerDecoder());
      ChiptuneDecoders.push_back(CreatePSFDecoder());
      ChiptuneDecoders.push_back(CreatePSF2Decoder());
      ChiptuneDecoders.push_back(CreateUSFDecoder());
      ChiptuneDecoders.push_back(CreateGSFDecoder());
      ChiptuneDecoders.push_back(Create2SFDecoder());
      ChiptuneDecoders.push_back(CreateNCSFDecoder());
      ChiptuneDecoders.push_back(CreateSSFDecoder());
      ChiptuneDecoders.push_back(CreateDSFDecoder());
      ChiptuneDecoders.push_back(CreateRasterMusicTrackerDecoder());
      ChiptuneDecoders.push_back(CreateMP3Decoder());
      //+50
      ChiptuneDecoders.push_back(CreateOGGDecoder());
      ChiptuneDecoders.push_back(CreateWAVDecoder());
      ChiptuneDecoders.push_back(CreateFLACDecoder());
      ChiptuneDecoders.push_back(CreateV2MDecoder());
    }

    static void Test(const Formats::Packed::Decoder& decoder, const Binary::Container& rawData)
    {
      const auto format = decoder.GetFormat();
      if (format->Match(rawData))
      {
        decoder.Decode(rawData);
      }
      else
      {
        const std::size_t offset = format->NextMatchOffset(rawData);
        if (offset != rawData.Size())
        {
          decoder.Decode(*rawData.GetSubcontainer(offset, rawData.Size() - offset));
        }
      }
    }

    static void Test(const Formats::Chiptune::Decoder& decoder, const Binary::Container& rawData)
    {
      decoder.Decode(rawData);
    }

  private:
    std::vector<Formats::Packed::Decoder::Ptr> PackedDecoders;
    std::vector<Formats::Chiptune::Decoder::Ptr> ChiptuneDecoders;
  };
}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
  if (size != 0)
  {
    const Binary::Container::Ptr container = Binary::CreateContainer(data, size);
    DecodersSet::Instance().TestChiptune(*container);
  }
  return 0;
}
