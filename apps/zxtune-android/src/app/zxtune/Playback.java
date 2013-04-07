/*
 * @file
 * @brief Playback control and callback interfaces
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune;

import android.net.Uri;

public final class Playback {

  /**
   *  Interface for playback item abstraction
   */
  public interface Item {

    /**
     * @return Unique item identifier
     */
    public Uri getId();

    /**
     * @return Item's data identifier
     */
    public Uri getDataId();

    /**
     * @return Item's title
     * @note may be empty
     */
    public String getTitle();

    /**
     * @return Item's author
     * @note may be empty
     */
    public String getAuthor();

    /**
     * @return Item's duration
     */
    public TimeStamp getDuration();
  }
  
  public enum Status {
    STOPPED,
    PLAYING,
    PAUSED
  }
  
  /**
   *  Playback control interface
   */
  public interface Control {

    /*
     * @return Currently playing item or null if not selected
     */
    public Item getItem();
    
    /*
     * @return Currently playing item's position or null if stopped
     */
    public TimeStamp getPlaybackPosition();

    /*
     * @return Current playback status
     */
    public Status getStatus();
    
    /*
     * Start playback of specified item (data or playlist)
     */
    public void play(Uri item);

    /*
     * Continue previously played or do nothing if already played
     */
    public void play();

    /*
     * Pause playing or do nothing
     */
    public void pause();

    /*
     * Stop playing or do nothing
     */
    public void stop();
  }
  
  /**
   *  Playback callback interface
   */
  public interface Callback {
    
    /**
     * Called on new item playback started
     */
    public void itemChanged(Item item);
    
    /**
     * Called on every status changing operation
     */
    public void statusChanged(Status status);
  }
}
