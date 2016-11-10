/**
 * 
 * @file
 *
 * @brief
 *
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune.fs.archives;

import android.net.Uri;

import app.zxtune.TimeStamp;

public class Track {

  public final Uri path;
  public final String filename;
  public final String description;
  public final TimeStamp duration;
  
  Track(Uri path, String filename, String description, TimeStamp duration) {
    this.path = path;
    this.filename = filename;
    this.description = description;
    this.duration = duration;
  }
}
