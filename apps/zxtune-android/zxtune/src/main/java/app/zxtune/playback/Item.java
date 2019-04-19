/**
 * @file
 * @brief Interface for playback item (metadata only)
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.playback;

import android.net.Uri;

import app.zxtune.core.Identifier;
import app.zxtune.TimeStamp;

public interface Item {

  /**
   * @return Unique item identifier - may be equal to dataId in case of plain file playback
   */
  Uri getId();

  /**
   * @return Item's data identifier
   */
  Identifier getDataId();

  /**
   * @return Item's title (possibly empty)
   */
  String getTitle();

  /**
   * @return Item's author (possibly empty)
   */
  String getAuthor();

  /**
   * @return Item's program information (possibly empty)
   */
  String getProgram();

  /**
   * @return Item's comment (possibly empty)
   */
  String getComment();

  /**
   * @return Item's internal strings (possibly empty)
   */
  String getStrings();

  /**
   * @return Item's duration
   */
  TimeStamp getDuration() throws Exception;
}
