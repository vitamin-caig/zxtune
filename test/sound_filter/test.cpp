#include <tools.h>
#include <src/sound/filter.h>
#include <src/sound/error_codes.h>

#include <cmath>
#include <iostream>
#include <iomanip>

namespace
{
  using namespace ZXTune::Sound;

  void ErrOuter(unsigned /*level*/, Error::LocationRef loc, Error::CodeType code, const String& text)
  {
    const String txt = (Formatter("\t%1%\n\tCode: %2$#x\n\tAt: %3%\n\t--------\n") % text % code % Error::LocationToString(loc)).str();
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

  const unsigned FREQ = 48000;
  const unsigned ORDER = 50;
  
  const unsigned MSECS = 50;
  
  const unsigned FREQS[] = 
  {
    10, 20, 30, 40, 50, 60, 70, 80, 90,
    100, 200, 300, 400, 500, 600, 700, 800, 900,
    1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000,
    10000, 11000, 12000, 13000, 14000, 15000, 16000, 17000, 18000, 19000,
    20000, 21000, 22000, 23000
  };
  
  void Generate(unsigned freq, Receiver& rcv, Receiver& analyzer)
  {
    for (unsigned tick = 0; tick != FREQ * MSECS / 1000; ++tick)
    {
      const Sample smp = SAMPLE_MID + (SAMPLE_MID - 1) * sin(tick * freq * 2 * 3.14159265358 / FREQ);
      const MultiSample msmp = { {smp, smp} };
      analyzer.ApplySample(msmp);
      rcv.ApplySample(msmp);
    }
  }

  inline double AddRMS(Sample sm)
  {
    const double lvl((double(sm) - SAMPLE_MID) / SAMPLE_MID);
    return lvl * lvl;
  }

  class Target : public Receiver
  {
  public:
    Target()
    {
      Count = 0;
      RMSOrig = RMSNew = 0;
    }
    
    virtual void ApplySample(const MultiSample& data)
    {
      (Count & 1 ? RMSNew : RMSOrig) += AddRMS(data[0]);
      ++Count;
    }
    
    virtual void Flush()
    {
    }
    
    void Report(unsigned freq) const
    {
      const double newRMS = sqrt(RMSNew / (Count / 2));
      const double oldRMS = sqrt(RMSOrig / (Count / 2));
      std::cout << "Freq: " << std::setw(5) << freq << std::fixed << std::setprecision(5) <<
	": Gain:" << newRMS << '/' << oldRMS << '=' << newRMS / oldRMS << '=' << 10 * log10(newRMS / oldRMS) << "dB" << std::endl;
    }
  private:
    unsigned Count;
    double RMSOrig, RMSNew;
  };
  
  void TestInvalid(const char* text, unsigned order, unsigned lo, unsigned hi)
  {
    std::vector<signed> coeffs(order);
    try
    {
      std::cout << "--- Test for " << text << " ---\n";
      CalculateBandpassFilter(FREQ, lo, hi, coeffs);
      std::cout << " Failed\n";
    }
    catch (const Error& e)
    {
      std::cout << " Passed\n";
      e.WalkSuberrors(ErrOuter);
    }
  }
  
  void TestFilter(unsigned lo, unsigned hi)
  {
    std::cout << "--- Testing for bandpass " << lo << "..." << hi << " ---\n";
    std::vector<signed> coeffs(ORDER);
    CalculateBandpassFilter(FREQ, lo, hi, coeffs);
    
    for (const unsigned* fr = FREQS; fr != ArrayEnd(FREQS); ++fr)
    {
      Target* tgt = 0;
      Receiver::Ptr receiver(tgt = new Target);
      Converter::Ptr filter(CreateFIRFilter(coeffs));
      filter->SetEndpoint(receiver);
    
      Generate(*fr, *filter, *receiver);
      tgt->Report(*fr);
    }
  }
}

int main()
{
  try
  {
    TestInvalid("order too low", 0, 0, 0);
    TestInvalid("order too high", 1000000, 0, 0);
    TestInvalid("high cutoff too low", 20, 10, 0);
    TestInvalid("high cutoff too high", 20, 10, FREQ);
    TestInvalid("low cutoff too high", 20, FREQ, FREQ / 2);

    TestFilter(0, 10000);
    TestFilter(10000, 20000);
    TestFilter(5000, 12000);
  }
  catch (const Error& e)
  {
    std::cout << " Failed\n";
    e.WalkSuberrors(ErrOuter);
  }
}
