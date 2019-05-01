package app.zxtune.core;

import android.support.annotation.NonNull;

/**
 * Properties modifier interface
 */
public interface PropertiesModifier {

  /**
   * Setting integer property
   *
   * @param name Name of the property
   * @param value Value of the property
   */
  void setProperty(@NonNull String name, long value);

  /**
   * Setting string property
   *
   * @param name Name of the property
   * @param value Value of the property
   */
  void setProperty(@NonNull String name, @NonNull String value);
}