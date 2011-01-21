/**
*
* @file      data_streaming.h
* @brief     Defenition of data streaming abstractions
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __DATA_STREAMING_H_DEFINED__
#define __DATA_STREAMING_H_DEFINED__

//boost includes
#include <boost/shared_ptr.hpp>

//! @brief Template data consuming interface. Instantiated with working data class
template<class T>
class DataReceiver
{
  class StubImpl : public DataReceiver
  {
  public:
    virtual void ApplyData(const T&) {}
    virtual void Flush() {}
  };
public:
  typedef T InDataType;
  //! @brief Pointer type
  typedef typename boost::shared_ptr<DataReceiver<T> > Ptr;

  virtual ~DataReceiver() {}

  //! @brief Data consuming point
  virtual void ApplyData(const T& data) = 0;
  //! @brief Flushing all possible accumulated data
  virtual void Flush() = 0;

  static Ptr CreateStub()
  {
    return Ptr(new StubImpl);
  }
};

//! @brief Template data transmitter type
template<class T>
class DataTransmitter
{
public:
  typedef T OutDataType;
  //! @brief Pointer type
  typedef typename boost::shared_ptr<DataTransmitter> Ptr;

  virtual ~DataTransmitter() {}

  virtual void SetTarget(typename DataReceiver<T>::Ptr target) = 0;
};

//! @brief Template data chained with possible converting interface
template<class InType, class OutType = InType>
class DataTransceiver : public DataReceiver<InType>
                      , public DataTransmitter<OutType>
{
public:
  //! @brief Pointer type.
  typedef typename boost::shared_ptr<DataTransceiver<InType, OutType> > Ptr;
};

#endif //__DATA_STREAMING_H_DEFINED__
