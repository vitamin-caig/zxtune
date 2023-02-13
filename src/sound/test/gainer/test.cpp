/**
 *
 * @file
 *
 * @brief  Gainer test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include <boost/range/size.hpp>
#include <error_tools.h>
#include <iomanip>
#include <iostream>
#include <math/numeric.h>
#include <sound/gainer.h>

namespace Sound
{
  const int_t THRESHOLD = 5 * (Sample::MAX - Sample::MIN) / 1000;  // 0.5%

  const Gain::Type GAINS[] = {
      Gain::Type(0),     Gain::Type(1),     Gain::Type(1, 2),    Gain::Type(1, 10),
      Gain::Type(9, 10), Gain::Type(10, 1), Gain::Type(1000, 1),
  };

  const String GAIN_NAMES[] = {"empty", "full", "middle", "-10dB", "-0.45dB", "10dB", "Overload"};

  inline Sample::Type ScaledMin(int denom, int divisor)
  {
    return Sample::MID + denom * (int_t(Sample::MIN) - Sample::MID) / divisor;
  }

  inline Sample::Type ScaledMax(int denom, int divisor)
  {
    return Sample::MID + denom * (int_t(Sample::MAX) - Sample::MID) / divisor;
  }

  const Sample::Type INPUTS[] = {Sample::MIN, ScaledMin(1, 256), Sample::MID, ScaledMax(1, 256), Sample::MAX};

  const String INPUT_NAMES[] = {"min", "min/256", "mid", "max/256", "max"};

  const Sample::Type OUTS[] = {
      // zero matrix
      Sample::MID,
      Sample::MID,
      Sample::MID,
      Sample::MID,
      Sample::MID,
      // full matrix
      Sample::MIN,
      ScaledMin(1, 256),
      Sample::MID,
      ScaledMax(1, 256),
      Sample::MAX,
      // mid matrix
      ScaledMin(1, 2),
      ScaledMin(1, 512),
      Sample::MID,
      ScaledMax(1, 512),
      ScaledMax(1, 2),
      //-10dB
      ScaledMin(1, 10),
      ScaledMin(1, 2560),
      Sample::MID,
      ScaledMax(1, 2560),
      ScaledMax(1, 10),
      //-0.45dB
      ScaledMin(9, 10),
      ScaledMin(9, 2560),
      Sample::MID,
      ScaledMax(9, 2560),
      ScaledMax(9, 10),
      // 10dB
      Sample::MIN,
      ScaledMin(10, 256),
      Sample::MID,
      ScaledMax(10, 256),
      Sample::MAX,
      // Overload
      Sample::MIN,
      Sample::MIN,
      Sample::MID,
      Sample::MAX,
      Sample::MAX,
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
    Target() {}

    void ApplyData(Chunk data) override
    {
      if (Check(data.front().Left(), ToCompare.Left()) && Check(data.front().Right(), ToCompare.Right()))
      {
        std::cout << "passed\n";
      }
      else
      {
        std::cout << "failed\n";
        throw MakeFormattedError(THIS_LINE, "Failed. Value=<{},{}> while expected=<{},{}>", data.front().Left(),
                                 data.front().Right(), ToCompare.Left(), ToCompare.Right());
      }
    }

    void Flush() override {}

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

  class Source : public GainSource
  {
  public:
    void SetGain(Gain::Type gain)
    {
      Value = gain;
    }

    Gain::Type Get() const override
    {
      return Value;
    }

  private:
    Gain::Type Value = Gain::Type();
  };
}  // namespace Sound

int main()
{
  using namespace Sound;

  try
  {
    Source* src = nullptr;
    Target* tgt = nullptr;
    const auto gainer = CreateGainer(GainSource::Ptr(src = new Source), Receiver::Ptr(tgt = new Target));

    const Sample::Type* result(OUTS);
    for (unsigned matrix = 0; matrix != boost::size(GAINS); ++matrix)
    {
      std::cout << "--- Test for " << GAIN_NAMES[matrix] << " gain ---\n";
      src->SetGain(GAINS[matrix]);
      for (unsigned input = 0; input != boost::size(INPUTS); ++input, ++result)
      {
        std::cout << "Checking for " << INPUT_NAMES[input] << " input: ";
        tgt->SetData(*result);
        Chunk chunk;
        chunk.reserve(1);
        chunk.push_back(Sample(INPUTS[input], INPUTS[input]));
        gainer->ApplyData(std::move(chunk));
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
