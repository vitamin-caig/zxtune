package app.zxtune.core

/**
 * Properties accessor interface
 */
interface PropertiesAccessor {
    /**
     * Getting integer property
     *
     * @param name Name of the property
     * @param defVal Default value
     * @return Property value or defVal if not found
     */
    fun getProperty(name: String, defVal: Long): Long

    /**
     * Getting string property
     *
     * @param name Name of the property
     * @param defVal Default value
     * @return Property value or defVal if not found
     */
    fun getProperty(name: String, defVal: String): String
}
