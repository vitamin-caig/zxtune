/**
* 
* @file
*
* @brief  Container for music files with multiple undivideable tracks inside (e.g. SID)
*
* @author vitamin.caig@gmail.com
*
**/

//library includes
#include <formats/chiptune.h>

namespace Formats
{
  namespace Chiptune
  {
    class PolyTrackContainer : public Container
    {
    public:
      typedef boost::shared_ptr<const PolyTrackContainer> Ptr;
      
      //! @return total tracks count
      virtual uint_t TracksCount() const = 0;

      //! @return 0-based index of first track
      virtual uint_t StartTrackIndex() const = 0;
      
      //! @brief Create copy of self with modified start track
      virtual Binary::Container::Ptr WithStartTrackIndex(uint_t idx) const = 0;
    };
  }
}
