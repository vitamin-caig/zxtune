#include <tools.h>
#include <error_tools.h>
#include <math/numeric.h>
#include <src/sound/gainer.h>

#include <iostream>
#include <iomanip>

#define FILE_TAG B5BAF4C1

namespace Sound
{
  const int_t THRESHOLD = 5 * (Sample::MAX - Sample::MIN) / 1000;//0.5%
  
  const double GAINS[] = {
    0.0f,
    1.0f,
    0.5f,
    0.1f,
    0.9f
  };
  
  const String GAIN_NAMES[] = {
    "empty", "full", "middle", "-10dB", "-0.45dB"
  };
  
  const double INVALID_GAIN = 2.0f;

  const Sample::Type INPUTS[] = {
    Sample::MIN,
    Sample::MID,
    Sample::MAX
  };
  
  const String INPUT_NAMES[] = {
    "min", "mid", "max"
  };
  
  const Sample::Type OUTS[] = {
  //zero matrix
     Sample::MID,
     Sample::MID,
     Sample::MID,
  //full matrix
     Sample::MIN,
     Sample::MID,
     Sample::MAX,
  //mid matrix
     (Sample::MID+Sample::MIN)/2,
     Sample::MID,
     (Sample::MID+Sample::MAX)/2,
  //-10dB
     Sample::MID+(int_t(Sample::MIN)-Sample::MID)/10,
     Sample::MID,
     Sample::MID+(int_t(Sample::MAX)-Sample::MID)/10,
  //-0.45dB
     Sample::MID+9*(int_t(Sample::MIN)-Sample::MID)/10,
     Sample::MID,
     Sample::MID+9*(int_t(Sample::MAX)-Sample::MID)/10
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
    
    virtual void ApplyData(const Sample& data)
    {
      if (Check(data.Left(), ToCompare.Left()) && Check(data.Right(), ToCompare.Right()))
      {
        std::cout << "passed\n";
      }
      else
      {
        std::cout << "failed\n";
        throw MakeFormattedError(THIS_LINE, "Failed. Value=<%1%,%2%> while expected=<%3%,%4%>",
          data.Left(), data.Right(), ToCompare.Left(), ToCompare.Right());
      }
    }
    
    virtual void Flush()
    {
    }
    
    void SetData(const Sample::Type& tc)
    {
      ToCompare = Sample(tc, tc);
    }
  private:
    static bool Check(Sample::Type data, Sample::Type ref)
    {
      return Math::Absolute(int_t(data) - ref) <= THRESHOLD;
    }
  private:
    Sample ToCompare;
  };
}

int main()
{
  using namespace Sound;
  
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

    const Sample::Type* result(OUTS);
    for (unsigned matrix = 0; matrix != ArraySize(GAINS); ++matrix)
    {
      std::cout << "--- Test for " << GAIN_NAMES[matrix] << " gain ---\n";
      gainer->SetGain(GAINS[matrix]);
      for (unsigned input = 0; input != ArraySize(INPUTS); ++input, ++result)
      {
        std::cout << "Checking for " << INPUT_NAMES[input] << " input: ";
        tgt->SetData(*result);
        const Sample in(INPUTS[input], INPUTS[input]);
        gainer->ApplyData(in);
      }
    }
    std::cout << " Succeed!" << std::endl;
  }
  catch (const Error& e)
  {
    std::cerr << e.ToString();
    return 1;
  }
}
