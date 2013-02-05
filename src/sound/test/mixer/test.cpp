#include <tools.h>
#include <error_tools.h>
#include <math/numeric.h>
#include <sound/matrix_mixer.h>
#include <sound/mixer_parameters.h>

#include <iostream>
#include <iomanip>

#define FILE_TAG 25E829A2

namespace
{
  using namespace ZXTune::Sound;

  BOOST_STATIC_ASSERT(SAMPLE_MIN == 0 && SAMPLE_MID == 32768 && SAMPLE_MAX == 65535);

  const int_t THRESHOLD = 5 * (SAMPLE_MAX - SAMPLE_MIN) / 1000;//0.5%
  
  const MultiGain GAINS[] = {
    { {0.0f, 0.0f} },
    { {1.0f, 1.0f} },
    { {1.0f, 0.0f} },
    { {0.0f, 1.0f} },
    { {0.5f, 0.5f} },
    { {0.1f, 0.9f} }
  };
  
  const String GAIN_NAMES[] = {
    "empty", "full", "left", "right", "middle", "-10dB,-0.45dB"
  };
  
  const MultiGain INVALID_GAIN = { {2.0f, 3.0f} };

  const Sample INPUTS[] = {
    SAMPLE_MIN,
    SAMPLE_MID,
    SAMPLE_MAX
  };
  
  const String INPUT_NAMES[] = {
    "zero", "half", "maximum"
  };
  
  const OutputSample OUTS[] = {
  //zero matrix
     { {SAMPLE_MID,SAMPLE_MID} },
     { {SAMPLE_MID,SAMPLE_MID} },
     { {SAMPLE_MID,SAMPLE_MID} },
  //full matrix
     { {SAMPLE_MIN,SAMPLE_MIN} },
     { {SAMPLE_MID,SAMPLE_MID} },
     { {SAMPLE_MAX,SAMPLE_MAX} },
  //left matrix
     { {SAMPLE_MIN,SAMPLE_MID} },
     { {SAMPLE_MID,SAMPLE_MID} },
     { {SAMPLE_MAX,SAMPLE_MID} },
  //right matrix
     { {SAMPLE_MID,SAMPLE_MIN} },
     { {SAMPLE_MID,SAMPLE_MID} },
     { {SAMPLE_MID, SAMPLE_MAX} },
  //mid matrix
     { {SAMPLE_MID/2,SAMPLE_MID/2} },
     { {SAMPLE_MID,SAMPLE_MID} },
     { {(SAMPLE_MAX+SAMPLE_MID)/2, (SAMPLE_MAX+SAMPLE_MID)/2} },
  //balanced
     //left=25 right=230
     //(25*32768)/256, (230*32768)/256
     //(25*65535)/256, (230*65535)/256
     { {Sample(SAMPLE_MID+0.1f*(SAMPLE_MIN-SAMPLE_MID)),Sample(SAMPLE_MID+0.9f*(SAMPLE_MIN-SAMPLE_MID))} },
     { {SAMPLE_MID,SAMPLE_MID} },
     { {Sample(SAMPLE_MID+0.1f*(SAMPLE_MAX-SAMPLE_MID)),Sample(SAMPLE_MID+0.9f*(SAMPLE_MAX-SAMPLE_MID))} }
  };

  template<class T>
  std::vector<T> MakeMatrix(unsigned chans, const T& mg)
  {
    return std::vector<T>(chans, mg);
  }

  bool ShowIfError(const Error& e)
  {
    if (e)
    {
      std::cerr << e.ToString();
    }
    return e;
  }

  class Target : public Receiver
  {
  public:
    Target()
    {
    }
    
    virtual void ApplyData(const OutputSample& data)
    {
      for (uint_t chan = 0; chan != data.size(); ++chan)
      {
        if (Math::Absolute(int_t(data[chan]) - ToCompare[chan]) > THRESHOLD)
          throw MakeFormattedError(THIS_LINE, "Failed. Value=<%1%,%2%> while expected=<%3%,%4%>",
            data[0], data[1], ToCompare[0], ToCompare[1]);
      }
      std::cout << "Passed";
    }
    
    virtual void Flush()
    {
    }
    
    void SetData(const OutputSample& tc)
    {
      ToCompare = tc;
    }
  private:
    OutputSample ToCompare;
  };
}

int main()
{
  using namespace ZXTune::Sound;
  
  try
  {
    for (unsigned chans = 1; chans <= 4; ++chans)
    {
      std::cout << "**** Testing for " << chans << " channels ****\n";
      Target* tgt = 0;
      Receiver::Ptr receiver(tgt = new Target);
    
      const MatrixMixer::Ptr mixer = CreateMatrixMixer(chans);
      
      mixer->SetTarget(receiver);
    
      std::cout << "--- Test for invalid matrix---\n";
      try
      {
        mixer->SetMatrix(MakeMatrix(chans, INVALID_GAIN));
        throw "Failed";
      }
      catch (const Error& e)
      {
        std::cout << " Passed\n";
        std::cerr << e.ToString();
      }
      catch (const std::string& str)
      {
        throw Error(THIS_LINE, str);
      }
      
      assert(ArraySize(OUTS) == ArraySize(GAINS) * ArraySize(INPUTS));
      assert(ArraySize(GAINS) == ArraySize(GAIN_NAMES));
      assert(ArraySize(INPUTS) == ArraySize(INPUT_NAMES));
      
      const OutputSample* result(OUTS);
      for (unsigned matrix = 0; matrix != ArraySize(GAINS); ++matrix)
      {
        std::cout << "--- Test for " << GAIN_NAMES[matrix] << " matrix ---\n";
        mixer->SetMatrix(MakeMatrix(chans, GAINS[matrix]));
        for (unsigned input = 0; input != ArraySize(INPUTS); ++input, ++result)
        {
          tgt->SetData(*result);
          mixer->ApplyData(MakeMatrix(chans, INPUTS[input]));
          std::cout << " checking for " << INPUT_NAMES[input] << " input\n";
        }
      }
      std::cout << "Parameters:" << std::endl;
      for (uint_t inChan = 0; inChan != chans; ++inChan)
      {
        for (uint_t outChan = 0; outChan != OUTPUT_CHANNELS; ++outChan)
        {
          const Parameters::NameType name = Parameters::ZXTune::Sound::Mixer::LEVEL(chans, inChan, outChan);
          const Parameters::IntType val = Parameters::ZXTune::Sound::Mixer::LEVEL_DEFAULT(chans, inChan, outChan);
          std::cout << name.FullPath() << ": " << val << std::endl;
        }
      }
    }
    std::cout << " Succeed!" << std::endl;
  }
  catch (const Error& e)
  {
    std::cerr << e.ToString();
  }
}
