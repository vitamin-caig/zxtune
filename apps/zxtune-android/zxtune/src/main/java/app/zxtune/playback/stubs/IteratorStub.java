/**
 *
 * @file
 *
 * @brief Stub singleton implementation of Iterator
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.playback.stubs;

import app.zxtune.playback.Iterator;
import app.zxtune.playback.PlayableItem;

public final class IteratorStub implements Iterator {

  private IteratorStub() {
  }

  @Override
  public PlayableItem getItem() {
    throw new IllegalStateException("Should not be called");
  }

  @Override
  public boolean next() {
    return false;
  }

  @Override
  public boolean prev() {
    return false;
  }
  
  public static Iterator instance() {
    return Holder.INSTANCE;
  }

  //onDemand holder idiom
  private static class Holder {
    public static final Iterator INSTANCE = new IteratorStub();
  }  
}
