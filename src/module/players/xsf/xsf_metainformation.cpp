/**
 *
 * @file
 *
 * @brief  Xsf-based files structure support implementation. Metainformation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "module/players/xsf/xsf_metainformation.h"
// library includes
#include <module/players/properties_helper.h>
#include <parameters/modifier.h>
#include <sound/sound_parameters.h>

namespace Module::XSF
{
  template<class T>
  inline void MergeVal(T& dst, const T& src)
  {
    if (!(src == T()))
    {
      dst = src;
    }
  }

  void MetaInformation::Merge(const MetaInformation& rh)
  {
    MergeVal(Title, rh.Title);
    MergeVal(Artist, rh.Artist);
    MergeVal(Game, rh.Game);
    MergeVal(Year, rh.Year);
    MergeVal(Genre, rh.Genre);
    MergeVal(Comment, rh.Comment);
    MergeVal(Copyright, rh.Copyright);
    MergeVal(Dumper, rh.Dumper);

    MergeVal(RefreshRate, rh.RefreshRate);
    MergeVal(Duration, rh.Duration);
    MergeVal(Fadeout, rh.Fadeout);
    MergeVal(Volume, rh.Volume);
    // TODO: merge tags
  }

  void MetaInformation::Dump(Parameters::Modifier& out) const
  {
    PropertiesHelper props(out);
    auto game = Game.empty() ? nullptr : &Game;
    if (!Title.empty())
    {
      props.SetTitle(Title);
    }
    else if (game)
    {
      props.SetTitle(*game);
      game = nullptr;
    }

    auto copyright = Copyright.empty() ? nullptr : &Copyright;
    auto dumper = Dumper.empty() ? nullptr : &Dumper;
    if (!Artist.empty())
    {
      props.SetAuthor(Artist);
    }
    else if (copyright)
    {
      props.SetAuthor(*copyright);
      copyright = nullptr;
    }
    else if (dumper)
    {
      props.SetAuthor(*dumper);
      dumper = nullptr;
    }

    if (!Comment.empty())
    {
      props.SetComment(Comment);
    }
    else if (game)
    {
      props.SetComment(*game);
      game = nullptr;
    }
    else if (copyright)
    {
      props.SetComment(*copyright);
      copyright = nullptr;
    }
    else if (dumper)
    {
      props.SetComment(*dumper);
      dumper = nullptr;
    }

    if (game)
    {
      props.SetProgram(*game);
      game = nullptr;
    }

    props.SetDate(Year);

    if (Volume)
    {
      out.SetValue(Parameters::ZXTune::Sound::GAIN, Parameters::ZXTune::Sound::GAIN_PRECISION * Volume);
    }

    if (Fadeout)
    {
      props.SetFadeout(Fadeout);
    }
  }
}  // namespace Module::XSF
