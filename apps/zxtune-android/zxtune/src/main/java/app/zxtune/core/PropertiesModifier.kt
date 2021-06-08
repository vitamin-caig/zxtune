package app.zxtune.core

/**
 * Properties modifier interface
 */
interface PropertiesModifier {
    /**
     * Setting integer property
     *
     * @param name Name of the property
     * @param value Value of the property
     */
    fun setProperty(name: String, value: Long)

    /**
     * Setting string property
     *
     * @param name Name of the property
     * @param value Value of the property
     */
    fun setProperty(name: String, value: String)
}
