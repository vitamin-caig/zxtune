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
      long getProperty(String name, long defVal) throws Exception;

      /**
       * Getting string property
       * 
       * @param name Name of the property
       * @param defVal Default value
       * @return Property value or defVal if not found
       */
      String getProperty(String name, String defVal) throws Exception;
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
      void setProperty(String name, long value) throws Exception;

      /**
       * Setting string property
       * 
       * @param name Name of the property
       * @param value Value of the property
       */
      void setProperty(String name, String value) throws Exception;
    }
    
    /**
     * Prefix for all properties
     */
    public static final String PREFIX = "zxtune.";

    /**
     * Sound properties 'namespace'
     */
    public static final class Sound {
      
      public static final String PREFIX = Properties.PREFIX + "sound.";

      /**
       * Sound frequency in Hz
       */
      public static final String FREQUENCY = PREFIX + "frequency";

      /**
       * Frame duration in microseconds
       */
      public static final String FRAMEDURATION = PREFIX + "frameduration";
      public static final long FRAMEDURATION_DEFAULT = 20000;
      
      /**
       * Loop mode
       */
      public static final String LOOPED = PREFIX + "looped";
    }

    /**
     * Core properties 'namespace'
     */
    public static final class Core {
      
      public static final String PREFIX = Properties.PREFIX + "core.";

      /**
       * AY/YM properties 'namespace'
       */
      public static final class Aym {
        
        public static final String PREFIX = Core.PREFIX + "aym.";

        public static final String INTERPOLATION = PREFIX + "interpolation";
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
      public static final String TYPE = "Type";

      /**
       * Title or name of the module
       */
      public static final String TITLE = "Title";

      /**
       * Author or creator of the module
       */
      public static final String AUTHOR = "Author";
      
      /**
       * Program module created in compatible with
       */
      public static final String PROGRAM = "Program";
      
      /**
       * Comment for module
       */
      public static final String COMMENT = "Comment";

      /**
       * Module strings
       */
      public static final String STRINGS = "Strings";

      /**
       * Module container
       */
      public static final String CONTAINER = "Container";
    }

    /**
     * @return Module's duration in frames
     */
    int getDuration() throws Exception;

    /**
     * Creates new player object
     * 
     * @throws Exception in case of error
     */
    Player createPlayer() throws Exception;
  }

  /**
   * Player interface
   */
  public interface Player extends Releaseable, Properties.Accessor, Properties.Modifier {
    
    /**
     * @return Index of next rendered frame
     */
    int getPosition() throws Exception;

    /**
     * @param bands Array of bands to store
     * @param levels Array of levels to store
     * @return Count of actually stored entries
     */
    int analyze(int bands[], int levels[]) throws Exception;
    
    /**
     * Render next result.length bytes of sound data
     * @param result Buffer to put data
     * @return Is there more data to render
     */
    boolean render(short[] result) throws Exception;
    
    /**
     * @param pos Index of next rendered frame
     */
    void setPosition(int pos) throws Exception;
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
      public static final GlobalOptions INSTANCE = new GlobalOptions();
    }
  }

  public static final class Plugins {
    
    public static final class DeviceType {
      //ZXTune::Capabilities::Module::Device::Type
      public static final int AY38910 = 1;
      public static final int TURBOSOUND = 2;
      public static final int BEEPER = 4;
      public static final int YM2203 = 8;
      public static final int TURBOFM = 16;
      public static final int DAC = 32;
      public static final int SAA1099 = 64;
      public static final int MOS6581 = 128;
      public static final int SPC700 = 256;
      public static final int MULTIDEVICE = 512;
      public static final int RP2A0X = 1024;
      public static final int LR35902 = 2048;
      public static final int CO12294 = 4096;
      public static final int HUC6270 = 8192;
    }
    
    public static final class ContainerType {
      //ZXTune::Capabilities::Container::Type
      public static final int ARCHIVE = 0;
      public static final int COMPRESSOR = 1;
      public static final int SNAPSHOT = 2;
      public static final int DISKIMAGE = 3;
      public static final int DECOMPILER = 4;
      public static final int MULTITRACK = 5;
      public static final int SCANER = 6;
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
   * @param content raw content
   * @param subpath module subpath in content
   * @return New object
   */
  public static Module loadModule(ByteBuffer content, String subpath) throws Exception {
    return new NativeModule(Module_Create(makeDirectBuffer(content), subpath));
  }
  
  public interface ModuleDetectCallback {
    void onModule(String subpath, Module obj) throws Exception;
  }
  
  static class ModuleDetectCallbackNativeAdapter {

    private final ModuleDetectCallback delegate;
    
    public ModuleDetectCallbackNativeAdapter(ModuleDetectCallback delegate) {
      this.delegate = delegate;
    }

    final void onModule(String subpath, int handle) throws Exception {
      delegate.onModule(subpath, new NativeModule(handle));
    }
  }
  
  public static void detectModules(ByteBuffer content, ModuleDetectCallback cb) throws Exception {
    Module_Detect(makeDirectBuffer(content), new ModuleDetectCallbackNativeAdapter(cb));
  }
  
  private static ByteBuffer makeDirectBuffer(ByteBuffer content) throws Exception {
    if (content.position() != 0) {
      throw new Exception("Input data should have zero position");
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

    protected NativeObject(int handle) {
      this.handle = handle;
    }

    @Override
    public void release() {
      Handle_Close(handle);
      handle = 0;
    }
  }

  private static final class NativeModule extends NativeObject implements Module {

    NativeModule(int handle) {
      super(handle);
    }

    @Override
    public int getDuration() throws Exception {
      return Module_GetDuration(handle);
    }

    @Override
    public long getProperty(String name, long defVal) throws Exception {
      return Module_GetProperty(handle, name, defVal);
    }

    @Override
    public String getProperty(String name, String defVal) throws Exception {
      return Module_GetProperty(handle, name, defVal);
    }

    @Override
    public Player createPlayer() throws Exception {
      return new NativePlayer(Module_CreatePlayer(handle));
    }
  }

  private static final class NativePlayer extends NativeObject implements Player {

    NativePlayer(int handle) {
      super(handle);
    }

    @Override
    public boolean render(short[] result) throws Exception {
      return Player_Render(handle, result);
    }
    
    @Override
    public int analyze(int bands[], int levels[]) throws Exception {
      return Player_Analyze(handle, bands, levels);
    }

    @Override
    public int getPosition() throws Exception {
      return Player_GetPosition(handle);
    }

    @Override
    public void setPosition(int pos) throws Exception {
      Player_SetPosition(handle, pos);
    }
    
    @Override
    public long getProperty(String name, long defVal) throws Exception {
      return Player_GetProperty(handle, name, defVal);
    }

    @Override
    public String getProperty(String name, String defVal) throws Exception {
      return Player_GetProperty(handle, name, defVal);
    }

    @Override
    public void setProperty(String name, long val) throws Exception {
      Player_SetProperty(handle, name, val);
    }

    @Override
    public void setProperty(String name, String val) throws Exception {
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
  private static native int Module_Create(ByteBuffer data, String subpath) throws Exception;
  private static native void Module_Detect(ByteBuffer data, ModuleDetectCallbackNativeAdapter cb) throws Exception;
  private static native int Module_GetDuration(int module) throws Exception;
  private static native long Module_GetProperty(int module, String name, long defVal) throws Exception;
  private static native String Module_GetProperty(int module, String name, String defVal) throws Exception;
  private static native int Module_CreatePlayer(int module) throws Exception;

  // working with player
  private static native boolean Player_Render(int player, short[] result) throws Exception;
  private static native int Player_Analyze(int player, int bands[], int levels[]) throws Exception;
  private static native int Player_GetPosition(int player) throws Exception;
  private static native void Player_SetPosition(int player, int pos) throws Exception;
  private static native long Player_GetProperty(int player, String name, long defVal) throws Exception;
  private static native String Player_GetProperty(int player, String name, String defVal) throws Exception;
  private static native void Player_SetProperty(int player, String name, long val) throws Exception;
  private static native void Player_SetProperty(int player, String name, String val) throws Exception;
  
  // working with plugins
  private static native void Plugins_Enumerate(Plugins.Visitor visitor);
}
