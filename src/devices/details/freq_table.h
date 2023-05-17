/**
 *
 * @file
 *
 * @brief  Frequency table helper class
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// library includes
#include <math/fixedpoint.h>

namespace Devices::Details
{
  // in centiHz
  using Frequency = Math::FixedPoint<int_t, 100>;

  class FreqTable
  {
  public:
    static const uint_t SIZE = 96;

    static Frequency GetHalftoneFrequency(uint_t halftones)
    {
      // http://www.phy.mtu.edu/~suits/notefreqs.html
      // clang-format off
        static const Frequency NOTES[SIZE] =
        {
        //C           C#/Db       D           D#/Eb       E           F           F#/Gb       G           G#/Ab       A           A#/Bb       B
          //octave1
            CHz(3270),  CHz(3465),  CHz(3671),  CHz(3889),  CHz(4120),  CHz(4365),  CHz(4625),  CHz(4900),  CHz(5191),  CHz(5500),  CHz(5827),  CHz(6174),
          //octave2
            CHz(6541),  CHz(6930),  CHz(7342),  CHz(7778),  CHz(8241),  CHz(8731),  CHz(9250),  CHz(9800), CHz(10383), CHz(11000), CHz(11654), CHz(12347),
          //octave3
           CHz(13081), CHz(13859), CHz(14683), CHz(15556), CHz(16481), CHz(17461), CHz(18500), CHz(19600), CHz(20765), CHz(22000), CHz(23308), CHz(24694),
          //octave4
           CHz(26163), CHz(27718), CHz(29366), CHz(31113), CHz(32963), CHz(34923), CHz(36999), CHz(39200), CHz(41530), CHz(44000), CHz(46616), CHz(49388),
          //octave5
           CHz(52325), CHz(55437), CHz(58733), CHz(62225), CHz(65926), CHz(69846), CHz(73999), CHz(78399), CHz(83061), CHz(88000), CHz(93233), CHz(98777),
          //octave6
          CHz(104650),CHz(110873),CHz(117466),CHz(124451),CHz(131851),CHz(139691),CHz(147998),CHz(156798),CHz(166122),CHz(176000),CHz(186466),CHz(197553),
          //octave7
          CHz(209300),CHz(221746),CHz(234932),CHz(248902),CHz(263702),CHz(279383),CHz(295996),CHz(313596),CHz(332244),CHz(352000),CHz(372931),CHz(395107),
          //octave8
          CHz(418601),CHz(443492),CHz(469864),CHz(497803),CHz(527405),CHz(558766),CHz(591991),CHz(627192),CHz(664488),CHz(704000),CHz(745862),CHz(790214)
        };
      // clang-format on
      return NOTES[halftones];
    }

  private:
    static inline Frequency CHz(uint_t frq)
    {
      return Frequency(frq, Frequency::PRECISION);
    }
  };
}  // namespace Devices::Details
