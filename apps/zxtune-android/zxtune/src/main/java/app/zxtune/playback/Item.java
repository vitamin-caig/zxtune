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
  String getTitle() throws Exception;

  /**
   * @return Item's author (possibly empty)
   */
  String getAuthor() throws Exception;

  /**
   * @return Item's program information (possibly empty)
   */
  String getProgram() throws Exception;

  /**
   * @return Item's comment (possibly empty)
   */
  String getComment() throws Exception;

  /**
   * @return Item's internal strings (possibly empty)
   */
  String getStrings() throws Exception;

  /**
   * @return Item's duration
   */
  TimeStamp getDuration() throws Exception;
}
