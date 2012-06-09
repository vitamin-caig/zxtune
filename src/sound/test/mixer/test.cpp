#include <tools.h>
#include <format.h>
#include <src/sound/mixer.h>
#include <src/sound/error_codes.h>

#include <iostream>
#include <iomanip>

#define FILE_TAG 25E829A2

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
    SAMPLE_MIN,
    SAMPLE_MID,
    SAMPLE_MAX
  };
  
  const String INPUT_NAMES[] = {
    "zero", "half", "maximum"
  };
  
  const MultiSample OUTS[] = {
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
     { {SAMPLE_MID+25*(SAMPLE_MIN-SAMPLE_MID)/256,SAMPLE_MID+230*(SAMPLE_MIN-SAMPLE_MID)/256} },
     { {SAMPLE_MID,SAMPLE_MID} },
     { {SAMPLE_MID+25*(SAMPLE_MAX-SAMPLE_MID)/256,SAMPLE_MID+230*(SAMPLE_MAX-SAMPLE_MID)/256} }
  };

  template<class T>
  std::vector<T> MakeMatrix(unsigned chans, const T& mg)
  {
    return std::vector<T>(chans, mg);
  }

  void ErrOuter(unsigned /*level*/, Error::LocationRef loc, Error::CodeType code, const String& text)
  {
    std::cerr << Error::AttributesToString(loc, code, text) << std::endl;
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
    
    virtual void ApplyData(const MultiSample& data)
    {
      if (const bool passed = data == ToCompare)
      {
        std::cout << "Passed";
      }
      else
      {
        const String txt = Strings::Format("Failed. Value=<%1%,%2%> while expected=<%3%,%4%>",
          data[0], data[1], ToCompare[0], ToCompare[1]);
        throw Error(THIS_LINE, 1, txt);
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
        e.WalkSuberrors(ErrOuter);
      }
      catch (const std::string& str)
      {
        throw Error(THIS_LINE, 1, str);
      }
      
      assert(ArraySize(OUTS) == ArraySize(GAINS) * ArraySize(INPUTS));
      assert(ArraySize(GAINS) == ArraySize(GAIN_NAMES));
      assert(ArraySize(INPUTS) == ArraySize(INPUT_NAMES));
      
      const MultiSample* result(OUTS);
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
    }
    std::cout << " Succeed!" << std::endl;
  }
  catch (const Error& e)
  {
    e.WalkSuberrors(ErrOuter);
  }
}
