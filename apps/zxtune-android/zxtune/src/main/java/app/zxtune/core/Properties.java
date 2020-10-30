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

    static final String PREFIX = Properties.PREFIX + "sound.";

    /**
     * Loop mode
     */
    public static final String LOOPED = PREFIX + "looped";
  }

  /**
   * Core properties 'namespace'
   */
  static final class Core {

    static final String PREFIX = Properties.PREFIX + "core.";

    /**
     * AY/YM properties 'namespace'
     */
    static final class Aym {

      static final String PREFIX = Core.PREFIX + "aym.";

      public static final String INTERPOLATION = PREFIX + "interpolation";
    }
  }
}
