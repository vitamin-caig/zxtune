package app.zxtune.core;

import androidx.annotation.NonNull;

/**
 * Properties accessor interface
 */
public interface PropertiesAccessor {

  /**
   * Getting integer property
   *
   * @param name Name of the property
   * @param defVal Default value
   * @return Property value or defVal if not found
   */
  long getProperty(@NonNull String name, long defVal);

  /**
   * Getting string property
   *
   * @param name Name of the property
   * @param defVal Default value
   * @return Property value or defVal if not found
   */
  @NonNull
  String getProperty(@NonNull String name, @NonNull String defVal);
}