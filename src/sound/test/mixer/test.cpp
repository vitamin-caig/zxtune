/**
 *
 * @file
 *
 * @brief  Mixer test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include <error_tools.h>
#include <math/numeric.h>
#include <sound/matrix_mixer.h>
#include <sound/mixer_parameters.h>

#include <iomanip>
#include <iostream>

#include <boost/range/size.hpp>

namespace Sound
{
  const int_t THRESHOLD = 5 * (Sample::MAX - Sample::MIN) / 1000;  // 0.5%

  Gain CreateGain(double l, double r)
  {
    return Gain(Gain::Type(l), Gain::Type(r));
  }

  const Gain GAINS[] = {CreateGain(0.0, 0.0), CreateGain(1.0, 1.0), CreateGain(1.0, 0.0),
                        CreateGain(0.0, 1.0), CreateGain(0.5, 0.5), CreateGain(0.1, 0.9)};

  const String GAIN_NAMES[] = {"empty", "full", "left", "right", "middle", "-10dB,-0.45dB"};

  const Gain INVALID_GAIN = CreateGain(2.0, 3.0);

  const Sample::Type INPUTS[] = {Sample::MIN, Sample::MID, Sample::MAX};

  const String INPUT_NAMES[] = {"min", "mid", "max"};

  const Sample OUTS[] = {
      // zero matrix
      Sample(Sample::MID, Sample::MID),
      Sample(Sample::MID, Sample::MID),
      Sample(Sample::MID, Sample::MID),
      // full matrix
      Sample(Sample::MIN, Sample::MIN),
      Sample(Sample::MID, Sample::MID),
      Sample(Sample::MAX, Sample::MAX),
      // left matrix
      Sample(Sample::MIN, Sample::MID),
      Sample(Sample::MID, Sample::MID),
      Sample(Sample::MAX, Sample::MID),
      // right matrix
      Sample(Sample::MID, Sample::MIN),
      Sample(Sample::MID, Sample::MID),
      Sample(Sample::MID, Sample::MAX),
      // mid matrix
      Sample((Sample::MID + Sample::MIN) / 2, (Sample::MID + Sample::MIN) / 2),
      Sample(Sample::MID, Sample::MID),
      Sample((Sample::MID + Sample::MAX) / 2, (Sample::MID + Sample::MAX) / 2),
      // balanced
      // left=25 right=230
      //(25*32768)/256, (230*32768)/256
      //(25*65535)/256, (230*65535)/256
      Sample(Sample::MID + (int_t(Sample::MIN) - Sample::MID) / 10,
             Sample::MID + 9 * (int_t(Sample::MIN) - Sample::MID) / 10),
      Sample(Sample::MID, Sample::MID),
      Sample(Sample::MID + (int_t(Sample::MAX) - Sample::MID) / 10,
             Sample::MID + 9 * (int_t(Sample::MAX) - Sample::MID) / 10),
  };

  template<class Res>
  typename Res::Type MakeSample(Sample::Type in)
  {
    typename Res::Type res;
    res.fill(in);
    return res;
  }

  template<unsigned Channels>
  typename FixedChannelsMatrixMixer<Channels>::Matrix MakeMatrix(const Gain& mg)
  {
    typename FixedChannelsMatrixMixer<Channels>::Matrix res;
    res.fill(mg);
    return res;
  }

  bool Check(Sample::Type data, Sample::Type ref)
  {
    return Math::Absolute(int_t(data) - ref) <= THRESHOLD;
  }

  void Check(const Sample& data, const Sample& ref)
  {
    if (Check(data.Left(), ref.Left()) && Check(data.Right(), ref.Right()))
    {
      std::cout << " passed\n";
    }
    else
    {
      std::cout << " failed\n";
      throw MakeFormattedError(THIS_LINE, "Value=<{},{}> while expected=<{},{}>", data.Left(), data.Right(), ref.Left(),
                               ref.Right());
    }
  }

  template<unsigned Channels>
  void TestMixer()
  {
    std::cout << "**** Testing for " << Channels << " channels ****\n";

    const typename FixedChannelsMatrixMixer<Channels>::Ptr mixer = FixedChannelsMatrixMixer<Channels>::Create();

    std::cout << "--- Test for invalid matrix---\n";
    try
    {
      mixer->SetMatrix(MakeMatrix<Channels>(INVALID_GAIN));
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

    assert(boost::size(OUTS) == boost::size(GAINS) * boost::size(INPUTS));
    assert(boost::size(GAINS) == boost::size(GAIN_NAMES));
    assert(boost::size(INPUTS) == boost::size(INPUT_NAMES));

    const Sample* result(OUTS);
    for (unsigned matrix = 0; matrix != boost::size(GAINS); ++matrix)
    {
      std::cout << "--- Test for " << GAIN_NAMES[matrix] << " matrix ---\n";
      mixer->SetMatrix(MakeMatrix<Channels>(GAINS[matrix]));
      for (unsigned input = 0; input != boost::size(INPUTS); ++input, ++result)
      {
        std::cout << "Checking for " << INPUT_NAMES[input] << " input: ";
        Check(mixer->ApplyData(MakeSample<MultichannelSample<Channels> >(INPUTS[input])), *result);
      }
    }
    std::cout << "Parameters:" << std::endl;
    for (uint_t inChan = 0; inChan != Channels; ++inChan)
    {
      for (uint_t outChan = 0; outChan != Sample::CHANNELS; ++outChan)
      {
        const auto name = Parameters::ZXTune::Sound::Mixer::LEVEL(Channels, inChan, outChan);
        const auto val = Parameters::ZXTune::Sound::Mixer::LEVEL_DEFAULT(Channels, inChan, outChan);
        std::cout << name.AsString() << ": " << val << std::endl;
      }
    }
  }
}  // namespace Sound

int main()
{
  using namespace Sound;
  try
  {
    TestMixer<1>();
    TestMixer<2>();
    TestMixer<3>();
    TestMixer<4>();
    std::cout << " Succeed!" << std::endl;
  }
  catch (const Error& e)
  {
    std::cerr << e.ToString();
    return 1;
  }
}
