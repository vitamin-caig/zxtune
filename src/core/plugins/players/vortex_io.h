/*
Abstract:
  Vortex IO functions declaations

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef __CORE_PLUGINS_PLAYERS_VORTEX_IO_H_DEFINED__
#define __CORE_PLUGINS_PLAYERS_VORTEX_IO_H_DEFINED__

#include "vortex_base.h"

namespace ZXTune
{
  namespace Module
  {
    namespace Vortex
    {
      //Deserialization
      bool SampleHeaderFromString(const std::string& str, uint_t& idx);
      bool SampleLineFromString(const std::string& str, Sample::Line& line, bool& looped);

      template<class Iterator>
      Iterator SampleFromStrings(Iterator it, Iterator lim, Sample& sample)
      {
        Sample tmp;
        for (; it != lim; ++it)
        {
          if (it->empty())
          {
            continue;
          }
          tmp.Data.push_back(Sample::Line());
          bool loop(false);
          if (!SampleLineFromString(*it, tmp.Data.back(), loop))
          {
            return it;
          }
          if (loop)
          {
            tmp.Loop = tmp.Data.size() - 1;
          }
        }
        sample.Loop = tmp.Loop;
        sample.Data.swap(tmp.Data);
        return it;
      }

      bool OrnamentHeaderFromString(const std::string& str, uint_t& idx);
      bool OrnamentFromString(const std::string& str, Ornament& ornament);

      bool PatternHeaderFromString(const std::string& str, uint_t& idx);
      bool PatternLineFromString(const std::string& str, Track::Line& line);

      template<class Iterator>
      Iterator PatternFromStrings(Iterator it, Iterator lim, Track::Pattern& pat)
      {
        Track::Pattern tmp;
        for (; it != lim; ++it)
        {
          if (it->empty())
          {
            continue;
          }
          tmp.push_back(Track::Line());
          if (!PatternLineFromString(*it, tmp.back()))
          {
            return it;
          }
        }
        pat.swap(tmp);
        return it;
      }

      struct Description
      {
        Description() : Version(), Notetable(), Tempo(), Loop()
        {
        }
        String Title;
        String Author;
        uint_t Version;
        uint_t Notetable;
        uint_t Tempo;
        uint_t Loop;
        std::vector<uint_t> Order;

        bool PropertyFromString(const std::string& str);
        std::string PropertyToString(uint_t idx) const;
      };

      bool DescriptionHeaderFromString(const std::string& str);

      template<class Iterator>
      Iterator DescriptionFromStrings(Iterator it, Iterator lim, Description& descr)
      {
        Description tmp;
        for (; it != lim; ++it)
        {
          if (!tmp.PropertyFromString(*it))
          {
            return it;
          }
        }
        //TODO: perform checking
        descr.Title.swap(tmp.Title);
        descr.Author.swap(tmp.Author);
        descr.Version = tmp.Version;
        descr.Notetable = tmp.Notetable;
        descr.Tempo = tmp.Tempo;
        descr.Loop = tmp.Loop;
        descr.Order.swap(tmp.Order);
        return it;
      }

      //Serialization
      std::string SampleHeaderToString(uint_t idx);
      std::string SampleLineToString(const Sample::Line& line, bool looped);

      template<class Iterator>
      Iterator SampleToStrings(const Sample& sample, Iterator it)
      {
        uint_t lpos(sample.Loop);
        for (std::vector<Sample::Line>::const_iterator sit = sample.Data.begin(),
          slim = sample.Data.end(); sit != slim; ++sit, --lpos, ++it)
        {
          *it = SampleLineToString(*sit, !lpos);
        }
        return it;//empty line at end
      }

      std::string OrnamentHeaderToString(uint_t idx);
      std::string OrnamentToString(const Ornament& ornament);

      std::string PatternHeaderToString(uint_t idx);
      std::string PatternLineToString(const Track::Line& line);

      template<class Iterator>
      Iterator PatternToStrings(const Track::Pattern& pat, Iterator it)
      {
        return std::transform(pat.begin(), pat.end(), it, PatternLineToString);
      }

      std::string DescriptionHeaderToString();
      template<class Iterator>
      Iterator DescriptionToStrings(const Description& descr, Iterator it)
      {
        for (uint_t propIdx = 0; ; ++propIdx, ++it)
        {
          const std::string& propVal = descr.PropertyToString(propIdx);
          if (propVal.empty())
          {
            break;
          }
          *it = propVal;
        }
        return it;
      }
    }
  }
}

#endif //__CORE_PLUGINS_PLAYERS_VORTEX_IO_H_DEFINED__
