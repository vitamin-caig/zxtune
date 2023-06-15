/**
 *
 * @file
 *
 * @brief  Defenition of data streaming abstractions
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <pointers.h>
// std includes
#include <memory>

//! @brief Template data consuming interface. Instantiated with working data class
template<class T>
class DataReceiver
{
public:
  using InDataType = T;
  //! @brief Pointer type
  using Ptr = typename std::shared_ptr<DataReceiver<T>>;

  virtual ~DataReceiver() = default;

  //! @brief Data consuming point
  virtual void ApplyData(T data) = 0;
  //! @brief Flushing all possible accumulated data
  virtual void Flush() = 0;

  static Ptr CreateStub();
};

// MSVC requires class not to be locally defined in method
template<class T>
class StubDataReceiver : public DataReceiver<T>
{
public:
  void ApplyData(T) override {}
  void Flush() override {}
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
  using OutDataType = T;
  //! @brief Pointer type
  using Ptr = typename std::shared_ptr<DataTransmitter>;

  virtual ~DataTransmitter() = default;

  virtual void SetTarget(typename DataReceiver<T>::Ptr target) = 0;
};

//! @brief Template data chained with possible converting interface
template<class InType, class OutType = InType>
class DataTransceiver
  : public DataReceiver<InType>
  , public DataTransmitter<OutType>
{
public:
  //! @brief Pointer type.
  using Ptr = typename std::shared_ptr<DataTransceiver<InType, OutType>>;
};

template<class InType, class OutType = InType>
class DataConverter
{
public:
  using Ptr = std::shared_ptr<DataConverter>;

  virtual ~DataConverter() = default;

  virtual OutType Apply(InType data) = 0;
};
