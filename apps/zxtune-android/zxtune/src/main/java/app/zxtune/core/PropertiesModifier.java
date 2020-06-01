package app.zxtune.core;

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
  void setProperty(String name, long value);

  /**
   * Setting string property
   *
   * @param name Name of the property
   * @param value Value of the property
   */
  void setProperty(String name, String value);
}