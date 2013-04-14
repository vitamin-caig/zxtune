/**
 * @file
 * @brief Callback subscription host interface
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */
package app.zxtune.playback;

import app.zxtune.Releaseable;

public interface CallbackSubscription {
  
  public Releaseable subscribe(Callback callback);
}
