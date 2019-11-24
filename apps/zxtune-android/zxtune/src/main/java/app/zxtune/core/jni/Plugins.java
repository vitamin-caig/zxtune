package app.zxtune.core.jni;

import androidx.annotation.NonNull;

public final class Plugins {

  static {
    JniLibrary.load();
  }

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

  @SuppressWarnings({"unused"})
  public interface Visitor {
    void onPlayerPlugin(int devices, @NonNull String id, @NonNull String description);

    void onContainerPlugin(int type, @NonNull String id, @NonNull String description);
  }

  public static native void enumerate(@NonNull Visitor visitor);
}
