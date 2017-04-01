/**
 *
 * @file
 *
 * @brief Interface for playback item (metadata only)
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.playback;

import android.net.Uri;

import app.zxtune.Identifier;
import app.zxtune.TimeStamp;

public interface Item {

  /**
   * @return Unique item identifier - may be equal to dataId in case of plain file playback
   */
  public Uri getId();

  /**
   * @return Item's data identifier
   */
  public Identifier getDataId();

  /**
   * @return Item's title (possibly empty)
   */
  public String getTitle();

  /**
   * @return Item's author (possibly empty)
   */
  public String getAuthor();
  
  /**
   * @return Item's program information (possibly empty)
   */
  public String getProgram();
  
  /**
   * @return Item's comment (possibly empty)
   */
  public String getComment();
  
  /**
   * @return Item's internal strings (possibly empty)
   */
  public String getStrings();

  /**
   * @return Item's duration
   */
  public TimeStamp getDuration();
}
