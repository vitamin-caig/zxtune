package app.zxtune.preferences

import androidx.core.text.isDigitsOnly
import app.zxtune.Logger
import app.zxtune.core.Properties
import app.zxtune.core.PropertiesModifier

/**
 * Adapts generic properties transport to PropertiesModifier
 */
private val LOG = Logger(RawPropertiesAdapter::class.java.name)

class RawPropertiesAdapter(val target: PropertiesModifier) {

    fun setProperty(name: String, value: Any) {
        if (!name.startsWith(Properties.PREFIX)) {
            return
        }
        try {
            when (value) {
                is String -> setProperty(name, value)
                is Long -> target.setProperty(name, value)
                is Int -> target.setProperty(name, value.toLong())
                is Boolean -> target.setProperty(name, if (value) 1 else 0)
                else -> throw Exception("Unknown type of property: ${value.javaClass.name}")
            }
        } catch (e: java.lang.Exception) {
            LOG.w(e) { "setProperty" }
        }
    }

    private fun setProperty(name: String, value: String) =
        if (isNumber(value)) {
            target.setProperty(name, value.toLong())
        } else {
            target.setProperty(name, value)
        }
}

private fun isNumber(seq: CharSequence) = when (seq.firstOrNull()) {
    null -> false
    '-', '+' -> seq.drop(1).isDigitsOnly()
    else -> seq.isDigitsOnly()
}
