/**
*
* @file     iterator.h
* @brief    Iterator interfaces
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#ifndef __ITERATOR_H_DEFINED__
#define __ITERATOR_H_DEFINED__

#include <memory>

//! @brief Iterator pattern implementation
template<class T>
class ObjectIterator
{
public:
  typedef typename std::auto_ptr<ObjectIterator<T> > Ptr;

  //! Virtual destructor
  virtual ~ObjectIterator() {}

  //! Check if accessor is valid
  virtual bool IsValid() const = 0;
  //! Getting stored value
  //! @invariant Should be called only on valid accessors)
  virtual T Get() const = 0;
  //! Moving to next stored value
  virtual void Next() = 0;
};

#endif //__ITERATOR_H_DEFINED__
