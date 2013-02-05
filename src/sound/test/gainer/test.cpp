#include <tools.h>
#include <error_tools.h>
#include <math/numeric.h>
#include <src/sound/gainer.h>

#include <iostream>
#include <iomanip>

#define FILE_TAG B5BAF4C1

namespace
{
  using namespace ZXTune::Sound;

  BOOST_STATIC_ASSERT(SAMPLE_MIN == 0 && SAMPLE_MID == 32768 && SAMPLE_MAX == 65535);

  const int_t THRESHOLD = 5 * (SAMPLE_MAX - SAMPLE_MIN) / 1000;//0.5%
  
  const Gain GAINS[] = {
    0.0f,
    1.0f,
    0.5f,
    0.1f,
    0.9f
  };
  
  const String GAIN_NAMES[] = {
    "empty", "full", "middle", "-10dB", "-0.45dB"
  };
  
  const Gain INVALID_GAIN = 2.0f;

  const Sample INPUTS[] = {
    SAMPLE_MIN,
    SAMPLE_MID,
    SAMPLE_MAX
  };
  
  const String INPUT_NAMES[] = {
    "zero", "half", "maximum"
  };
  
  const Sample OUTS[] = {
  //zero matrix
     SAMPLE_MID,
     SAMPLE_MID,
     SAMPLE_MID,
  //full matrix
     SAMPLE_MIN,
     SAMPLE_MID,
     SAMPLE_MAX,
  //mid matrix
     SAMPLE_MID/2,
     SAMPLE_MID,
     (SAMPLE_MAX+SAMPLE_MID)/2,
  //-10dB
     Sample(SAMPLE_MID+0.1f*(SAMPLE_MIN-SAMPLE_MID)),
     SAMPLE_MID,
     Sample(SAMPLE_MID+0.1f*(SAMPLE_MAX-SAMPLE_MID)),
  //-0.45dB
     Sample(SAMPLE_MID+0.9f*(SAMPLE_MIN-SAMPLE_MID)),
     SAMPLE_MID,
     Sample(SAMPLE_MID+0.9f*(SAMPLE_MAX-SAMPLE_MID)),
  };

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
        if (Math::Absolute(int_t(data[chan]) - ToCompare) > THRESHOLD)
          throw MakeFormattedError(THIS_LINE, "Failed. Value=<%1%,%2%> while expected=<%3%,%3%>",
            data[0], data[1], ToCompare);
      }
      std::cout << "Passed";
    }
    
    virtual void Flush()
    {
    }
    
    void SetData(const Sample& tc)
    {
      ToCompare = tc;
    }
  private:
    Sample ToCompare;
  };
}

int main()
{
  using namespace ZXTune::Sound;
  
  try
  {
    Target* tgt = 0;
    Receiver::Ptr receiver(tgt = new Target);
    const FadeGainer::Ptr gainer = CreateFadeGainer();
    gainer->SetTarget(receiver);
    
    std::cout << "--- Test for invalid gain ---" << std::endl;
    try
    {
      gainer->SetGain(INVALID_GAIN);
      throw "Failed!";
    }
    catch (const Error& e)
    {
      std::cout << " Passed\n";
      std::cout << e.ToString();
    }
    catch (const std::string& str)
    {
      throw Error(THIS_LINE, str);
    }

    const Sample* result(OUTS);
    for (unsigned matrix = 0; matrix != ArraySize(GAINS); ++matrix)
    {
      std::cout << "--- Test for " << GAIN_NAMES[matrix] << " matrix ---\n";
      gainer->SetGain(GAINS[matrix]);
      for (unsigned input = 0; input != ArraySize(INPUTS); ++input, ++result)
      {
        tgt->SetData(*result);
        OutputSample in;
        in.fill(INPUTS[input]);
        gainer->ApplyData(in);
        std::cout << " checking for " << INPUT_NAMES[input] << " input\n";
      }
    }
    std::cout << " Succeed!" << std::endl;
  }
  catch (const Error& e)
  {
    std::cerr << e.ToString();
  }
}
