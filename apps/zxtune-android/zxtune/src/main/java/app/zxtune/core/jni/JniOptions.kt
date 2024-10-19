package app.zxtune.core.jni

import app.zxtune.core.PropertiesContainer

internal object JniOptions : PropertiesContainer {
    external override fun setProperty(name: String, value: Long)

    external override fun setProperty(name: String, value: String)

    external override fun getProperty(name: String, defVal: Long): Long

    external override fun getProperty(name: String, defVal: String): String

    override fun getProperty(name: String, defVal: ByteArray?) = defVal
}
