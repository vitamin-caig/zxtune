/**
* 
* @file
*
* @brief UI widgets factory interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <parameters/container.h>
#include <strings/array.h>
//qt includes
#include <QtCore/QPointer>
#include <QtGui/QMainWindow>

class WidgetsFactory
{
public:
  virtual ~WidgetsFactory() {}
  
  //main window
  virtual QPointer<QMainWindow> CreateMainWindow(Parameters::Container::Ptr options, const Strings::Array& cmdline) const = 0;

  //singleton
  static WidgetsFactory& Instance();
};
