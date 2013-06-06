/**
 * @file
 * @brief
 * @version $Id:$
 * @author
 */
package app.zxtune.playback;

import android.net.Uri;
import app.zxtune.TimeStamp;
import app.zxtune.ZXTune;

public class StubPlayableItem implements PlayableItem {
  
  private static final String EMPTY_STRING = "";
  
  private StubPlayableItem() {
  }
  
  public Uri getId() {
    return Uri.EMPTY;
  }

  public Uri getDataId() {
    return Uri.EMPTY;
  }

  public String getTitle() {
    return EMPTY_STRING;
  }

  public String getAuthor() {
    return EMPTY_STRING;
  }

  public TimeStamp getDuration() {
    return TimeStamp.EMPTY;
  }

  public ZXTune.Player createPlayer() {
    throw new IllegalStateException("Should not be called");
  }

  @Override
  public void release() {
  }

  public static PlayableItem instance() {
    return Holder.INSTANCE;
  }

  //onDemand holder idiom
  private static class Holder {
    public static final PlayableItem INSTANCE = new StubPlayableItem();
  }  
}
