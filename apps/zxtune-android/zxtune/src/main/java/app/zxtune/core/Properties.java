package app.zxtune.core;

/**
 * Poperties 'namespace'
 */
public final class Properties {

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
