/**
* 
* @file
*
* @brief  Module properties helper interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <formats/chiptune.h>
#include <parameters/modifier.h>
#include <strings/array.h>

namespace Module
{
  class PropertiesHelper
  {
  public:
    explicit PropertiesHelper(Parameters::Modifier& delegate)
      : Delegate(delegate)
    {
    }
    
    //Generic
    void SetNonEmptyProperty(const String& name, const String& value);
    
    //Common
    void SetType(const String& type);
    void SetContainer(const String& container);
    void SetSource(const Formats::Chiptune::Container& source);
    
    //Meta
    void SetAuthor(const String& author);
    void SetTitle(const String& title);
    void SetComment(const String& comment);
    void SetProgram(const String& program);
    void SetComputer(const String& computer);
    void SetStrings(const Strings::Array& strings);
    void SetVersion(uint_t major, uint_t minor);
    void SetVersion(const String& version);
    void SetDate(const String& date);
    void SetPlatform(const String& platform);
    
    //Sound
    void SetFramesFrequency(uint_t hz);
    void SetFramesParameters(uint_t samplesCount, uint_t sampleRate);
  protected:
    Parameters::Modifier& Delegate;
  };
}
