/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.playback.stubs;

import app.zxtune.TimeStamp;
import app.zxtune.playback.Callback;
import app.zxtune.playback.Item;
import app.zxtune.playback.PlaybackControl;

public abstract class CallbackStub implements Callback {

  @Override
  public void onInitialState(PlaybackControl.State state) {}

  @Override
  public void onStateChanged(PlaybackControl.State state, TimeStamp pos) {}

  @Override
  public void onItemChanged(Item item) {}

  @Override
  public void onError(String e) {}
}
