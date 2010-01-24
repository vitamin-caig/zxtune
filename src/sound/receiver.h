/**
*
* @file      sound/receiver.h
* @brief     Defenition of sound receiver interface
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#ifndef __SOUND_RECEIVER_H_DEFINED__
#define __SOUND_RECEIVER_H_DEFINED__

#include <sound/sound_types.h>

#include <boost/shared_ptr.hpp>

#include <vector>

namespace ZXTune
{
  namespace Sound
  {
    //! @brief Template sound consuming interface
    template<class T>
    class BasicReceiver
    {
    public:
      typedef T DataType;
      //! @brief Pointer type
      typedef typename boost::shared_ptr<BasicReceiver<T> > Ptr;
      
      virtual ~BasicReceiver() {}
      
      //! @brief Data consuming point
      virtual void ApplySample(const T& data) = 0;
      //! @brief Flushing all possible accumulated data
      virtual void Flush() = 0;
    };
    
    //! @brief Template sound converting interface
    template<class S, class T = S>
    class BasicConverter : public BasicReceiver<S>
    {
    public:
      //! @brief Pointer type. Downcastable to BasicReceiver<S>::Ptr
      typedef typename boost::shared_ptr<BasicConverter<S, T> > Ptr;
      
      //! @brief Attaching next point in chain
      virtual void SetEndpoint(typename BasicReceiver<T>::Ptr endpoint) = 0;
    };
    
    //! @brief Simple sound stream endpoint receiver
    typedef BasicReceiver<MultiSample> Receiver;
    //! @brief Simle sound stream converter
    typedef BasicConverter<MultiSample> Converter;
    //! @brief Multichannel stream receiver
    typedef BasicReceiver<std::vector<Sample> > MultichannelReceiver;
  }
}

#endif //__SOUND_RECEIVER_H_DEFINED__
