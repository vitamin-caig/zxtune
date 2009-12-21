#include <tools.h>
#include <src/sound/mixer.h>
#include <src/sound/error_codes.h>

#include <iostream>
#include <iomanip>

namespace
{
  using namespace ZXTune::Sound;

  BOOST_STATIC_ASSERT(SAMPLE_MIN == 0 && SAMPLE_MID == 32768 && SAMPLE_MAX == 65535);
  
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
    0,
    32768,
    65535
  };
  
  const String INPUT_NAMES[] = {
    "zero", "half", "maximum"
  };
  
  const MultiSample OUTS[] = {
  //zero matrix
     { {0,0} }, { {0,0} }, { {0,0} },
  //full matrix
     { {0,0} }, { {32768,32768} }, { {65535,65535} },
  //left matrix
     { {0,0} }, { {32768,0} }, { {65535, 0} },
  //right matrix
     { {0,0} }, { {0,32768} }, { {0, 65535} },
  //mid matrix
     { {0,0} }, { {16384,16384} }, { {32767, 32767} },
  //balanced
     //left=25 right=230
     //(25*32768)/256, (230*32768)/256
     //(25*65535)/256, (230*65535)/256
     { {0,0} }, { {25*32768/256,230*32768/256} }, { {25*65535/256,230*65535/256} }
  };

  template<class T>
  std::vector<T> MakeMatrix(const T& mg)
  {
    return std::vector<T>(&mg, &mg + 1);
  }

  void ErrOuter(unsigned /*level*/, Error::LocationRef loc, Error::CodeType code, const String& text)
  {
    const String txt = (Formatter("\t%1%\n\tCode: %2%\n\tAt: %3%\n\t--------\n") % text % Error::CodeToString(code) % Error::LocationToString(loc)).str();
    std::cout << txt;
  }
  
  bool ShowIfError(const Error& e)
  {
    if (e)
    {
      e.WalkSuberrors(ErrOuter);
    }
    return e;
  }

  class Target : public Receiver
  {
  public:
    Target()
    {
    }
    
    virtual void ApplySample(const MultiSample& data)
    {
      if (const bool passed = data == ToCompare)
      {
        std::cout << "Passed";
      }
      else
      {
        std::cout << "Failed (in=<" << data[0] << ',' << data[1] << "> exp=<" << ToCompare[0] << ',' << ToCompare[1] << '>';
      }
    }
    
    virtual void Flush()
    {
    }
    
    void SetData(const MultiSample& tc)
    {
      ToCompare = tc;
    }
  private:
    MultiSample ToCompare;
  };
}

int main()
{
  using namespace ZXTune::Sound;
  
  try
  {
    Target* tgt = 0;
    Receiver::Ptr receiver(tgt = new Target);
  
    Mixer::Ptr mixer;
    ThrowIfError(CreateMixer(1, mixer));
    
    mixer->SetEndpoint(receiver);
  
    std::cout << "--- Test for invalid matrix---\n";
    if (const Error& e = mixer->SetMatrix(MakeMatrix(INVALID_GAIN)))
    {
      std::cout << " Passed\n";
      e.WalkSuberrors(ErrOuter);
    }
    else
    {
      std::cout << " Failed\n";
    }
    
    assert(ArraySize(OUTS) == ArraySize(GAINS) * ArraySize(INPUTS));
    assert(ArraySize(GAINS) == ArraySize(GAIN_NAMES));
    assert(ArraySize(INPUTS) == ArraySize(INPUT_NAMES));
    
    const MultiSample* result(OUTS);
    for (unsigned matrix = 0; matrix != ArraySize(GAINS); ++matrix)
    {
      std::cout << "--- Test for " << GAIN_NAMES[matrix] << " matrix ---\n";
      ThrowIfError(mixer->SetMatrix(MakeMatrix(GAINS[matrix])));
      for (unsigned input = 0; input != ArraySize(INPUTS); ++input, ++result)
      {
        tgt->SetData(*result);
	mixer->ApplySample(MakeMatrix(INPUTS[input]));
        std::cout << " checking for " << INPUT_NAMES[input] << " input\n";
      }
    }
  }
  catch (const Error& e)
  {
    std::cout << " Failed\n";
    e.WalkSuberrors(ErrOuter);
  }
}
