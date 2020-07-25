package app.zxtune.core;

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
  long getProperty(String name, long defVal);

  /**
   * Getting string property
   *
   * @param name Name of the property
   * @param defVal Default value
   * @return Property value or defVal if not found
   */
  String getProperty(String name, String defVal);
}