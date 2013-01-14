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
  }

  private static class NativeObject {

    protected int handle;

    protected NativeObject(int handle) {
      if (0 == handle) {
        throw new RuntimeException();
      }
      this.handle = handle;
    }
  }

  public static final class Data extends NativeObject {

    Data(byte[] data) {
      super(Data_Create(toByteBuffer(data)));
    }

    @Override
    protected void finalize() {
      Data_Destroy(handle);
    }

    public Module createModule() {
      return new Module(Module_Create(handle));
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

    @Override
    protected void finalize() {
      Module_Destroy(handle);
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
      return new Player(Player_Create(handle));
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

    @Override
    protected void finalize() {
      Player_Destroy(handle);
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

  // working with data
  private static native int Data_Create(ByteBuffer data);

  private static native void Data_Destroy(int handle);

  // working with module
  private static native int Module_Create(int data);

  private static native void Module_Destroy(int module);

  private static native int ModuleInfo_GetFramesCount(int module);

  private static native long Module_GetProperty(int module, String name, long defVal);

  private static native String Module_GetProperty(int module, String name, String defVal);
  
  // working with player
  private static native int Player_Create(int module);

  private static native void Player_Destroy(int player);

  private static native boolean Player_Render(int player, ByteBuffer result);

  private static native int Player_GetPosition(int player);

  private static native long Player_GetProperty(int player, String name, long defVal);

  private static native String Player_GetProperty(int player, String name, String defVal);

  private static native void Player_SetProperty(int player, String name, long val);

  private static native void Player_SetProperty(int player, String name, String val);
}
