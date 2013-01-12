/*
* @file
* @brief Gate to native ZXTune library code
* @version $Id:$
* @author (C) Vitamin/CAIG
*/

package app.zxtune;

import java.nio.ByteBuffer;
import java.lang.RuntimeException;

public class ZXTune {

  public static class Data {

    private int Handle;

    Data(ByteBuffer data) {
      if (data == null) {
        throw new RuntimeException("Failed to create empty data");
      }
      Handle = Data_Create(data);
      if (Handle == 0) {
        throw new RuntimeException("Failed to create data");
      }
    }

    @Override
    protected void finalize() {
      Data_Destroy(Handle);
    }

    Module CreateModule() {
      return new Module(Module_Create(Handle));
    }
  }

  public static class Module {

    private int Handle;

    Module(int handle) {
      Handle = handle;
      if (Handle == 0) {
        throw new RuntimeException("Failed to create module");
      }
    }

    @Override
    protected void finalize() {
      Module_Destroy(Handle);
    }

    int GetFramesCount() {
      return ModuleInfo_GetFramesCount(Handle);
    }

    Player CreatePlayer() {
      return new Player(Player_Create(Handle));
    }
  }

  public static class Player {

    private int Handle;

    Player(int handle) {
      Handle = handle;
      if (Handle == 0) {
        throw new RuntimeException("Failed to create player");
      }
    }

    @Override
    protected void finalize() {
      Player_Destroy(Handle);
    }

    boolean Render(ByteBuffer result) {
      return Player_Render(Handle, result);
    }

    int GetPosition() {
      return Player_GetPosition(Handle);
    }

    long GetProperty(String name, long defVal) {
      return Player_GetProperty(Handle, name, defVal);
    }

    String GetProperty(String name, String defVal) {
      return Player_GetProperty(Handle, name, defVal);
    }

    void SetProperty(String name, long val) {
      Player_SetProperty(Handle, name, val);
    }

    void SetProperty(String name, String val) {
      Player_SetProperty(Handle, name, val);
    }
  }

  static {
    System.loadLibrary("zxtune");
  }

  //working with data
  private static native int Data_Create(ByteBuffer data);
  private static native void Data_Destroy(int handle);
  
  //working with module
  private static native int Module_Create(int data);
  private static native void Module_Destroy(int module);
  private static native int ModuleInfo_GetFramesCount(int module);
  
  //working with player
  private static native int Player_Create(int module);
  private static native void Player_Destroy(int player);
  private static native boolean Player_Render(int player, ByteBuffer result);
  private static native int Player_GetPosition(int player);
  private static native long Player_GetProperty(int player, String name, long defVal);
  private static native String Player_GetProperty(int player, String name, String defVal);
  private static native void Player_SetProperty(int player, String name, long val);
  private static native void Player_SetProperty(int player, String name, String val);
}
