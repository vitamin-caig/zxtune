/**
*
* @file      data_streaming.h
* @brief     Defenition of data streaming abstractions
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef DATA_STREAMING_H_DEFINED
#define DATA_STREAMING_H_DEFINED

//common includes
#include <pointers.h>

//! @brief Template data consuming interface. Instantiated with working data class
template<class T>
class DataReceiver
{
public:
  typedef T InDataType;
  //! @brief Pointer type
  typedef typename boost::shared_ptr<DataReceiver<T> > Ptr;

  virtual ~DataReceiver() {}

  //! @brief Data consuming point
  virtual void ApplyData(const T& data) = 0;
  //! @brief Flushing all possible accumulated data
  virtual void Flush() = 0;

  static Ptr CreateStub();
};

// MSVC requires class not to be locally defined in method
template<class T>
class StubDataReceiver : public DataReceiver<T>
{
public:
  virtual void ApplyData(const T&) {}
  virtual void Flush() {}
};

template<class T>
inline typename DataReceiver<T>::Ptr DataReceiver<T>::CreateStub()
{
  static StubDataReceiver<T> instance;
  return MakeSingletonPointer(instance);
}

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
