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

  public static final class Properties {

    public static interface Accessor {

      long getProperty(String name, long defVal);

      String getProperty(String name, String defVal);
    }

    public static interface Modifier {

      void setProperty(String name, long value);

      void setProperty(String name, String value);
    }

    public final static String SOUND_FREQUENCY = "zxtune.sound.frequency";
    public final static String FRAME_DURATION = "zxtune.sound.frameduration";
    public final static String AYM_INTERPOLATION = "zxtune.core.aym.interpolation";
  }

  public static class NativeObject {

    protected int handle;

    protected NativeObject(int handle) {
      if (0 == handle) {
        throw new RuntimeException();
      }
      this.handle = handle;
    }

    @Override
    protected void finalize() {
      close();
    }

    public void close() {
      Handle_Close(handle);
    }
  }

  public static final class Data extends NativeObject {

    Data(byte[] data) {
      super(Data_Create(toByteBuffer(data)));
    }

    public Module createModule() {
      return new Module(Data_CreateModule(handle));
    }

    private static ByteBuffer toByteBuffer(byte[] data) {
      final ByteBuffer result = ByteBuffer.allocateDirect(data.length);
      result.put(data);
      return result;
    }
  }

  public static final class Module extends NativeObject implements Properties.Accessor {

    public static final class Attributes {
      public final static String TITLE = "Title";
      public final static String AUTHOR = "Author";
    }

    Module(int handle) {
      super(handle);
    }

    int getFramesCount() {
      return ModuleInfo_GetFramesCount(handle);
    }

    public long getProperty(String name, long defVal) {
      return Module_GetProperty(handle, name, defVal);
    }

    public String getProperty(String name, String defVal) {
      return Module_GetProperty(handle, name, defVal);
    }
    
    Player createPlayer() {
      return new Player(Module_CreatePlayer(handle));
    }
  }

  public static final class Player extends NativeObject
      implements
        Properties.Accessor,
        Properties.Modifier {

    private ByteBuffer renderBuffer;

    Player(int handle) {
      super(handle);
    }

    boolean render(byte[] result) {
      allocateBuffer(result.length);
      final boolean res = Player_Render(handle, renderBuffer);
      renderBuffer.get(result, 0, result.length);
      renderBuffer.rewind();
      return res;
    }

    int getPosition() {
      return Player_GetPosition(handle);
    }

    public long getProperty(String name, long defVal) {
      return Player_GetProperty(handle, name, defVal);
    }

    public String getProperty(String name, String defVal) {
      return Player_GetProperty(handle, name, defVal);
    }

    public void setProperty(String name, long val) {
      Player_SetProperty(handle, name, val);
    }

    public void setProperty(String name, String val) {
      Player_SetProperty(handle, name, val);
    }

    private void allocateBuffer(int size) {
      if (renderBuffer == null || renderBuffer.capacity() < size) {
        renderBuffer = ByteBuffer.allocateDirect(size);
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

  private static native int ModuleInfo_GetFramesCount(int module);

  private static native long Module_GetProperty(int module, String name, long defVal);

  private static native String Module_GetProperty(int module, String name, String defVal);
  
  private static native int Module_CreatePlayer(int module);

  // working with player
  private static native boolean Player_Render(int player, ByteBuffer result);

  private static native int Player_GetPosition(int player);

  private static native long Player_GetProperty(int player, String name, long defVal);

  private static native String Player_GetProperty(int player, String name, String defVal);

  private static native void Player_SetProperty(int player, String name, long val);

  private static native void Player_SetProperty(int player, String name, String val);
}
