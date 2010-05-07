/**
*
* @file     signals_collector.h
* @brief    Interface for working with asynchronous signals
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#ifndef __SIGNALS_COLLECTOR_H_DEFINED__
#define __SIGNALS_COLLECTOR_H_DEFINED__

//common includes
#include <types.h>
//std includes
#include <memory>
//boost includes
#include <boost/shared_ptr.hpp>

//! @brief Signals collector interface
class SignalsCollector
{
public:
  //! @brief Pointer type
  typedef boost::shared_ptr<SignalsCollector> Ptr;

  virtual ~SignalsCollector() {}

  //! @brief Waiting for signals or gathering already collected between calls
  //! @param sigmask Reference to result
  //! @param timeoutMs Timeout in specified milliseconds
  //! @return false if timeout reached before any signal collected, true otherwise
  //! @invariant If return true, signals is not empty
  virtual bool WaitForSignals(uint_t& sigmask, uint_t timeoutMs) = 0;
};

//! @brief Signals managing interface
class SignalsDispatcher
{
public:
  //! @brief Pointer type
  typedef std::auto_ptr<SignalsDispatcher> Ptr;
  virtual ~SignalsDispatcher() {}

  //! @brief Creating new signals collector
  //! @param signalsMask Required signals mask
  //! @return Pointer to new collector registered in backend. Automatically unregistered when released
  virtual SignalsCollector::Ptr CreateCollector(uint_t signalsMask) const = 0;

  //! @brief Notifies all the collectors about new signal
  //! @param sigmask New signals set
  virtual void Notify(uint_t sigmask) = 0;

  //! @brief Creator
  static Ptr Create();
};

#endif //__SIGNALS_COLLECTOR_H_DEFINED__
