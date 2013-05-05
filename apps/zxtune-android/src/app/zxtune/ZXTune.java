/*
 * @file
 * @brief Gate to native ZXTune library code
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune;

import java.lang.RuntimeException;

public final class ZXTune {

  /*
   * Public ZXTune interface
   */

  /**
   * Poperties 'namespace'
   */
  public static final class Properties {

    /**
     * Properties accessor interface
     */
    public interface Accessor {

      /**
       * Getting integer property
       * 
       * @param name Name of the property
       * @param defVal Default value
       * @return Property value or defVal if not found
       */
      long getProperty(String name, long defVal);

      /**
       * Getting string property
       * 
       * @param name Name of the property
       * @param defVal Default value
       * @return Property value or defVal if not found
       */
      String getProperty(String name, String defVal);
    }

    /**
     * Properties modifier interface
     */
    public static interface Modifier {

      /**
       * Setting integer property
       * 
       * @param name Name of the property
       * @param value Value of the property
       */
      void setProperty(String name, long value);

      /**
       * Setting string property
       * 
       * @param name Name of the property
       * @param value Value of the property
       */
      void setProperty(String name, String value);
    }

    /**
     * Sound properties 'namespace'
     */
    public static final class Sound {

      /**
       * Sound frequency in Hz
       */
      public final static String FREQUENCY = "zxtune.sound.frequency";

      /**
       * Frame duration in microseconds
       */
      public final static String FRAMEDURATION = "zxtune.sound.frameduration";
    }

    /**
     * Core properties 'namespace'
     */
    public static final class Core {

      /**
       * AY/YM properties 'namespace'
       */
      public static final class Aym {

        /**
         * Interpolation usage (1/0)
         */
        public final static String INTERPOLATION = "zxtune.core.aym.interpolation";
      }
    }
  }

  /**
   * Abstract data storage used to create module upon
   */
  public interface Data extends Releaseable {

    /**
     * Creates new module object
     * 
     * @return New module
     * @throws RuntimeException in case of error
     */
    public Module createModule();
  }

  /**
   * Module interface
   */
  public interface Module extends Releaseable, Properties.Accessor {

    /**
     * Attributes 'namespace'
     */
    public static final class Attributes {

      /**
       * Type. Several uppercased letters used to identify format
       */
      public final static String TYPE = "Type";

      /**
       * Title or name of the module
       */
      public final static String TITLE = "Title";

      /**
       * Author or creator of the module
       */
      public final static String AUTHOR = "Author";
    }

    /**
     * @return Module's duration in frames
     */
    int getDuration();

    /**
     * Creates new player object
     * 
     * @throws RuntimeException in case of error
     */
    Player createPlayer();
  }

  /**
   * Player interface
   */
  public interface Player extends Releaseable, Properties.Accessor, Properties.Modifier {
    
    /**
     * @return Index of next rendered frame
     */
    public int getPosition();

    /**
     * @param bands Array of bands to store
     * @param levels Array of levels to store
     * @return Count of actually stored entries
     */
    public int analyze(int bands[], int levels[]);
    
    /**
     * Render next result.length bytes of sound data
     * @param result Buffer to put data
     * @return Is there more data to render
     */
    public boolean render(short[] result);
    
    /**
     * @param pos Index of next rendered frame
     */
    public void setPosition(int pos);
  }

  /**
   * Simple data factory
   * @param Content raw content
   * @return New object
   */
  static Data createData(byte[] content) {
    return new NativeData(content);
  }

  /**
   * Base object of all the native implementations
   */
  private static class NativeObject implements Releaseable {

    protected int handle;

    protected NativeObject(int handle) {
      if (0 == handle) {
        throw new RuntimeException();
      }
      this.handle = handle;
    }

    @Override
    public void release() {
      Handle_Close(handle);
      handle = 0;
    }
  }

  private static final class NativeData extends NativeObject implements Data {

    NativeData(byte[] data) {
      super(Data_Create(data));
    }

    @Override
    public Module createModule() {
      return new NativeModule(Data_CreateModule(handle));
    }
  }

  private static final class NativeModule extends NativeObject implements Module {

    NativeModule(int handle) {
      super(handle);
    }

    @Override
    public int getDuration() {
      return Module_GetDuration(handle);
    }

    @Override
    public long getProperty(String name, long defVal) {
      return Module_GetProperty(handle, name, defVal);
    }

    @Override
    public String getProperty(String name, String defVal) {
      return Module_GetProperty(handle, name, defVal);
    }

    @Override
    public Player createPlayer() {
      return new NativePlayer(Module_CreatePlayer(handle));
    }
  }

  private static final class NativePlayer extends NativeObject implements Player {

    NativePlayer(int handle) {
      super(handle);
    }

    @Override
    public boolean render(short[] result) {
      return Player_Render(handle, result);
    }
    
    @Override
    public int analyze(int bands[], int levels[]) {
      return Player_Analyze(handle, bands, levels);
    }

    @Override
    public int getPosition() {
      return Player_GetPosition(handle);
    }

    @Override
    public void setPosition(int pos) {
      Player_SetPosition(handle, pos);
    }
    
    @Override
    public long getProperty(String name, long defVal) {
      return Player_GetProperty(handle, name, defVal);
    }

    @Override
    public String getProperty(String name, String defVal) {
      return Player_GetProperty(handle, name, defVal);
    }

    @Override
    public void setProperty(String name, long val) {
      Player_SetProperty(handle, name, val);
    }

    @Override
    public void setProperty(String name, String val) {
      Player_SetProperty(handle, name, val);
    }
  }

  static {
    System.loadLibrary("zxtune");
  }

  // working with handles
  private static native void Handle_Close(int handle);

  // working with data
  private static native int Data_Create(byte[] data);

  private static native int Data_CreateModule(int data);

  // working with module
  private static native int Module_GetDuration(int module);

  private static native long Module_GetProperty(int module, String name, long defVal);

  private static native String Module_GetProperty(int module, String name, String defVal);

  private static native int Module_CreatePlayer(int module);

  // working with player
  private static native boolean Player_Render(int player, short[] result);
  
  private static native int Player_Analyze(int player, int bands[], int levels[]);

  private static native int Player_GetPosition(int player);
  
  private static native void Player_SetPosition(int player, int pos);

  private static native long Player_GetProperty(int player, String name, long defVal);

  private static native String Player_GetProperty(int player, String name, String defVal);

  private static native void Player_SetProperty(int player, String name, long val);

  private static native void Player_SetProperty(int player, String name, String val);
}
