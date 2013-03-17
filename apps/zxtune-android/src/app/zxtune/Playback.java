/*
 * @file
 * @brief Playback control and callback interfaces
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune;

public final class Playback {

  public static interface Control {
    public void play();

    public void pause();

    public void stop();

    //after registration one of the first three methods will be called according to current state
    public void registerCallback(Callback cb);

    public void unregisterCallback(Callback cb);
  }

  public static interface Callback {
    public void started(String description, int duration);

    public void paused(String description);

    public void stopped();

    public void positionChanged(int curFrame, String curTime);
  }
}
