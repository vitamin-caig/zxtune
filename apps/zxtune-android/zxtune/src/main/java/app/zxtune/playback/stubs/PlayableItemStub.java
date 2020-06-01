/**
 *
 * @file
 *
 * @brief Stub singleton implementation of PlayableItem
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.playback.stubs;

import app.zxtune.core.Module;
import app.zxtune.playback.PlayableItem;

public class PlayableItemStub extends ItemStub implements PlayableItem {
  
  private PlayableItemStub() {
  }
  
  @Override
  public Module getModule() {
    throw new IllegalStateException("Should not be called");
  }

  public static PlayableItem instance() {
    return Holder.INSTANCE;
  }

  //onDemand holder idiom
  private static class Holder {
    static final PlayableItem INSTANCE = new PlayableItemStub();
  }  
}
