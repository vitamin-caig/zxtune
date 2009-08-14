#ifndef __DETECTOR_H_DEFINED__
#define __DETECTOR_H_DEFINED__

#include <types.h>

namespace ZXTune
{
  namespace Module
  {
    //Pattern format
    //xx - match byte (hex)
    //? - any byte
    //+xx+ - skip xx bytes (dec)
    bool Detect(const uint8_t* data, std::size_t size, const std::string& pattern);

    struct DetectChain
    {
      const std::string PlayerFP;
      const std::size_t PlayerSize;
    };

    typedef bool (*CheckFunc)(const uint8_t*, std::size_t);

    class Detector : public std::unary_function<DetectChain, bool>
    {
    public:
      Detector(CheckFunc checker, const uint8_t* data, std::size_t limit)
        : Checker(checker), Data(data), Limit(limit)
      {
      }

      result_type operator()(const argument_type& arg) const
      {
        return Detect(Data, Limit, arg.PlayerFP) &&
          Checker(Data + arg.PlayerSize, Limit - arg.PlayerSize);
      }
    private:
      const CheckFunc Checker;
      const uint8_t* const Data;
      const std::size_t Limit;
    };
  }
}

#endif //__DETECTOR_H_DEFINED__
