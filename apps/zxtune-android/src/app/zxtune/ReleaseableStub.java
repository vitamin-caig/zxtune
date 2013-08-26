/**
 * @file
 * @brief
 * @version $Id:$
 * @author
 */
package app.zxtune;

import app.zxtune.playback.Item;
import app.zxtune.playback.ItemStub;

/**
 * 
 */
public final class ReleaseableStub implements Releaseable {

  private ReleaseableStub() {
    // TODO Auto-generated constructor stub
  }

  @Override
  public void release() {
  }

  public static Releaseable instance() {
    return Holder.INSTANCE;
  }

  //onDemand holder idiom
  private static class Holder {
    public static final Releaseable INSTANCE = new ReleaseableStub();
  }  
}
