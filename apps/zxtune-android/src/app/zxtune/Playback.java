/*
 * @file
 * 
 * @brief Playback control and callback interfaces
 * 
 * @version $Id:$
 * 
 * @author (C) Vitamin/CAIG
 */

package app.zxtune;

public final class Playback {
  
  static interface Control {
    public void open(String moduleId);

    public void play();

    public void pause();

    public void stop();
  }
}
