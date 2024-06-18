/**
 *
 * @file
 *
 * @brief  Gainer test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// library includes
#include <math/numeric.h>
#include <sound/gainer.h>
// common includes
#include <contract.h>
#include <error_tools.h>
// std includes
#include <algorithm>
#include <iostream>

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
      Sample::MIN / 2,
      Sample::MID,
      Sample::MAX / 2,
      Sample::MAX,
  };

  bool Check(Sample::Type data, Sample::Type ref)
  {
    return Math::Absolute(int_t(data) - ref) <= THRESHOLD;
  }

  void Test(const Chunk& data, Sample::Type ref)
  {
    Require(data.size() == 1);
    if (Check(data.front().Left(), ref) && Check(data.front().Right(), ref))
    {
      std::cout << "passed\n";
    }
    else
    {
      std::cout << "failed\n";
      throw MakeFormattedError(THIS_LINE, "Failed. Value=<{},{}> while expected=<{},{}>", data.front().Left(),
                               data.front().Right(), ref, ref);
    }
  }
}  // namespace Sound

int main()
{
  using namespace Sound;

  try
  {
    const auto gainer = CreateGainer();
    const Sample::Type* result(OUTS);
    for (unsigned matrix = 0; matrix != std::size(GAINS); ++matrix)
    {
      std::cout << "--- Test for " << GAIN_NAMES[matrix] << " gain ---\n";
      gainer->SetGain(GAINS[matrix]);
      for (unsigned input = 0; input != std::size(INPUTS); ++input, ++result)
      {
        std::cout << "Checking for " << INPUT_NAMES[input] << " input: ";
        Sound::Chunk chunk;
        chunk.emplace_back(INPUTS[input], INPUTS[input]);
        const auto output = gainer->Apply(std::move(chunk));
        Test(output, *result);
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
