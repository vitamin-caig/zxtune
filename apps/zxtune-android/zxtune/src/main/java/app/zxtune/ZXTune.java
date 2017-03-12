/**
 *
 * @file
 *
 * @brief Gate to native zxtune code and related types
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune;

import java.io.InvalidObjectException;
import java.nio.ByteBuffer;

public final class ZXTune {

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
    public interface Modifier {

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
     * Prefix for all properties
     */
    public final static String PREFIX = "zxtune.";

    /**
     * Sound properties 'namespace'
     */
    public static final class Sound {
      
      public final static String PREFIX = Properties.PREFIX + "sound.";

      /**
       * Sound frequency in Hz
       */
      public final static String FREQUENCY = PREFIX + "frequency";

      /**
       * Frame duration in microseconds
       */
      public final static String FRAMEDURATION = PREFIX + "frameduration";
      public final static long FRAMEDURATION_DEFAULT = 20000;
      
      /**
       * Loop mode
       */
      public final static String LOOPED = PREFIX + "looped";
    }

    /**
     * Core properties 'namespace'
     */
    public static final class Core {
      
      public final static String PREFIX = Properties.PREFIX + "core.";

      /**
       * AY/YM properties 'namespace'
       */
      public static final class Aym {
        
        public final static String PREFIX = Core.PREFIX + "aym.";

        public final static String INTERPOLATION = PREFIX + "interpolation";
      }
    }
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
      
      /**
       * Program module created in compatible with
       */
      public final static String PROGRAM = "Program";
      
      /**
       * Comment for module
       */
      public final static String COMMENT = "Comment";

      /**
       * Module strings
       */
      public final static String STRINGS = "Strings";

      /**
       * Module container
       */
      public final static String CONTAINER = "Container";
    }

    /**
     * @return Module's duration in frames
     */
    int getDuration();

    /**
     * Creates new player object
     * 
     * @throws InvalidObjectException in case of error
     */
    Player createPlayer() throws InvalidObjectException;
  }

  /**
   * Player interface
   */
  public interface Player extends Releaseable, Properties.Accessor, Properties.Modifier {
    
    /**
     * @return Index of next rendered frame
     */
    int getPosition();

    /**
     * @param bands Array of bands to store
     * @param levels Array of levels to store
     * @return Count of actually stored entries
     */
    int analyze(int bands[], int levels[]);
    
    /**
     * Render next result.length bytes of sound data
     * @param result Buffer to put data
     * @return Is there more data to render
     */
    boolean render(short[] result);
    
    /**
     * @param pos Index of next rendered frame
     */
    void setPosition(int pos);
  }
  
  public static class GlobalOptions implements Properties.Accessor, Properties.Modifier {
    
    private GlobalOptions() {
    }

    @Override
    public void setProperty(String name, long value) {
      GlobalOptions_SetProperty(name, value);
    }

    @Override
    public void setProperty(String name, String value) {
      GlobalOptions_SetProperty(name, value);
    }

    @Override
    public long getProperty(String name, long defVal) {
      return GlobalOptions_GetProperty(name, defVal);
    }

    @Override
    public String getProperty(String name, String defVal) {
      return GlobalOptions_GetProperty(name, defVal);
    }
    
    public static GlobalOptions instance() {
      return Holder.INSTANCE;
    }
    
    private static class Holder {
      public final static GlobalOptions INSTANCE = new GlobalOptions(); 
    }
  }

  public final static class Plugins {
    
    public final static class DeviceType {
      //ZXTune::Capabilities::Module::Device::Type
      public final static int AY38910 = 1;
      public final static int TURBOSOUND = 2;
      public final static int BEEPER = 4;
      public final static int YM2203 = 8;
      public final static int TURBOFM = 16;
      public final static int DAC = 32;
      public final static int SAA1099 = 64;
      public final static int MOS6581 = 128;
      public final static int SPC700 = 256;
      public final static int MULTIDEVICE = 512;
      public final static int RP2A0X = 1024;
      public final static int LR35902 = 2048;
      public final static int CO12294 = 4096;
      public final static int HUC6270 = 8192;
    }
    
    public final static class ContainerType {
      //ZXTune::Capabilities::Container::Type
      public final static int ARCHIVE = 0;
      public final static int COMPRESSOR = 1;
      public final static int SNAPSHOT = 2;
      public final static int DISKIMAGE = 3;
      public final static int DECOMPILER = 4;
      public final static int MULTITRACK = 5;
      public final static int SCANER = 6;
    }
    
    public interface Visitor {
      void onPlayerPlugin(int devices, String id, String description);
      void onContainerPlugin(int type, String id, String description);
    }
    
    public static void enumerate(Visitor visitor) {
      Plugins_Enumerate(visitor);
    }
    
    private static native void init();
    
    static {
      init();
    }
  }
  
  /**
   * Simple data factory
   * @param Content raw content
   * @return New object
   */
  public static Module loadModule(ByteBuffer content, String subpath) throws InvalidObjectException {
    return new NativeModule(Module_Create(makeDirectBuffer(content), subpath));
  }
  
  public interface ModuleDetectCallback {
    void onModule(String subpath, Module obj);
  }
  
  static class ModuleDetectCallbackNativeAdapter {

    private final ModuleDetectCallback delegate;
    
    public ModuleDetectCallbackNativeAdapter(ModuleDetectCallback delegate) {
      this.delegate = delegate;
    }

    final void onModule(String subpath, int handle) {
      try {
        delegate.onModule(subpath, new NativeModule(handle));
      } catch (InvalidObjectException e) {
        //TODO
      }
    }
  }
  
  public static void detectModules(ByteBuffer content, ModuleDetectCallback cb) {
    Module_Detect(makeDirectBuffer(content), new ModuleDetectCallbackNativeAdapter(cb));
  }
  
  private static ByteBuffer makeDirectBuffer(ByteBuffer content) {
    if (content.position() != 0) {
      throw new Error("Input data should have zero position");
    }
    if (content.isDirect()) {
      return content;
    } else {
      final ByteBuffer direct = ByteBuffer.allocateDirect(content.limit());
      direct.put(content);
      direct.position(0);
      content.position(0);
      return direct;
    }
  }

  /**
   * Base object of all the native implementations
   */
  private static class NativeObject implements Releaseable {

    protected int handle;

    protected NativeObject(int handle) throws InvalidObjectException {
      if (0 == handle) {
        throw new InvalidObjectException(getClass().getName());
      }
      this.handle = handle;
    }

    @Override
    public void release() {
      Handle_Close(handle);
      handle = 0;
    }
  }

  private static final class NativeModule extends NativeObject implements Module {

    NativeModule(int handle) throws InvalidObjectException {
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
    public Player createPlayer() throws InvalidObjectException {
      return new NativePlayer(Module_CreatePlayer(handle));
    }
  }

  private static final class NativePlayer extends NativeObject implements Player {

    NativePlayer(int handle) throws InvalidObjectException {
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
  
  // working with global options
  private static native long GlobalOptions_GetProperty(String name, long defVal);
  private static native String GlobalOptions_GetProperty(String name, String defVal);
  private static native void GlobalOptions_SetProperty(String name, long value);
  private static native void GlobalOptions_SetProperty(String name, String value);

  // working with handles
  private static native void Handle_Close(int handle);

  // working with module
  private static native int Module_Create(ByteBuffer data, String subpath);
  private static native void Module_Detect(ByteBuffer data, ModuleDetectCallbackNativeAdapter cb);
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
  
  // working with plugins
  private static native void Plugins_Enumerate(Plugins.Visitor visitor);
}
