/*
 * @file
 * 
 * @brief Gate to native ZXTune library code
 * 
 * @version $Id:$
 * 
 * @author (C) Vitamin/CAIG
 */

package app.zxtune;

import java.nio.ByteBuffer;
import java.lang.RuntimeException;

public final class ZXTune {

  /*
   * Public ZXTune interface
   */

  // ! @brief Properties namespace
  public static final class Properties {

    // ! @brief Properties accessor interface
    public static interface Accessor {
      // ! @brief Getting integer property
      // ! @return Property value if found, defVal elsewhere
      long getProperty(String name, long defVal);

      // ! @brief Getting string property
      // ! @return Property value if found, defVal elsewhere
      String getProperty(String name, String defVal);
    }

    // ! @brief Properties modifier interface
    public static interface Modifier {
      // ! @brief Modifying integer property
      void setProperty(String name, long value);

      // ! @brief Modifying string property
      void setProperty(String name, String value);
    }

    // ! @brief Sound properties namespace
    public static final class Sound {
      // ! @brief Sound frequency in Hz
      public final static String FREQUENCY = "zxtune.sound.frequency";
      // ! @brief Frame duration in uS
      public final static String FRAMEDURATION = "zxtune.sound.frameduration";
    }

    // ! @brief Core properties namespace
    public static final class Core {
      // ! @brief Aym properties namespace
      public static final class Aym {
        // ! @brief Use interpolation
        public final static String INTERPOLATION = "zxtune.core.aym.interpolation";
      }
    }
  }

  // ! @brief Abstract object interface used to manage lifetime of all the objects inside
  public static interface Object {
    // ! @brief Called when not need anymore
    public void release();
  }

  // ! @brief Interface for abstract data storage
  public static interface Data extends Object {
    // ! @brief Creates new module object
    // ! @throw RuntimeException in case of error
    public Module createModule();
  }

  // ! @brief Interface of module
  public static interface Module extends Object, Properties.Accessor {
    // ! @brief Module's attributes
    public static final class Attributes {
      // ! @brief Title or name
      public final static String TITLE = "Title";
      // ! @brief Author or creator
      public final static String AUTHOR = "Author";
    }

    // ! @return Module's duration in frames
    int getDuration();

    // ! @brief Creates new renderer object
    // ! @throw RuntimeException in case of error
    Player createPlayer();
  }

  public static interface Player extends Object, Properties.Accessor, Properties.Modifier {
    // ! @return Get position of further rendered sound in frames
    int getPosition();

    // ! @brief Render next result.length bytes of sound data
    // ! @param result Buffer place data to
    // ! @return true if it's more data to render
    boolean render(byte[] result);
  }

  // ! @brief Factory of data container
  static Data createData(byte[] content) {
    return new NativeData(content);
  }

  /*
   * Private interface
   */
  private static class NativeObject implements Object {

    protected int handle;

    protected NativeObject(int handle) {
      if (0 == handle) {
        throw new RuntimeException();
      }
      this.handle = handle;
    }

    @Override
    protected void finalize() {
      release();
    }

    @Override
    public void release() {
      Handle_Close(handle);
    }
  }

  private static final class NativeData extends NativeObject implements Data {

    NativeData(byte[] data) {
      super(Data_Create(toByteBuffer(data)));
    }

    @Override
    public Module createModule() {
      return new NativeModule(Data_CreateModule(handle));
    }

    private static ByteBuffer toByteBuffer(byte[] data) {
      final ByteBuffer result = ByteBuffer.allocateDirect(data.length);
      result.put(data);
      return result;
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

    private ByteBuffer buffer;

    NativePlayer(int handle) {
      super(handle);
    }

    @Override
    public boolean render(byte[] result) {
      allocateBuffer(result.length);
      final boolean res = Player_Render(handle, result.length, buffer);
      buffer.get(result, 0, result.length);
      buffer.rewind();
      return res;
    }

    @Override
    public int getPosition() {
      return Player_GetPosition(handle);
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

    private void allocateBuffer(int size) {
      if (buffer == null || buffer.capacity() < size) {
        buffer = ByteBuffer.allocateDirect(size);
      }
    }
  }

  static {
    System.loadLibrary("zxtune");
  }

  // working with handles
  private static native void Handle_Close(int handle);

  // working with data
  private static native int Data_Create(ByteBuffer data);

  private static native int Data_CreateModule(int data);

  // working with module
  private static native int Module_GetDuration(int module);

  private static native long Module_GetProperty(int module, String name, long defVal);

  private static native String Module_GetProperty(int module, String name, String defVal);

  private static native int Module_CreatePlayer(int module);

  // working with player
  private static native boolean Player_Render(int player, int bytes, ByteBuffer result);

  private static native int Player_GetPosition(int player);

  private static native long Player_GetProperty(int player, String name, long defVal);

  private static native String Player_GetProperty(int player, String name, String defVal);

  private static native void Player_SetProperty(int player, String name, long val);

  private static native void Player_SetProperty(int player, String name, String val);
}
