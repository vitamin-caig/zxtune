package app.zxtune.preferences

import android.content.SharedPreferences
import android.content.SharedPreferences.OnSharedPreferenceChangeListener
import app.zxtune.Releaseable
import app.zxtune.core.PropertiesModifier

/**
 * Gate between SharedPreferences and PropertiesModifier
 */
object SharedPreferencesBridge {
    @JvmStatic
    fun subscribe(preferences: SharedPreferences, target: PropertiesModifier): Releaseable {
        val adapter = RawPropertiesAdapter(target)
        dumpExistingProperties(preferences.all, adapter)
        val listener =
            OnSharedPreferenceChangeListener { updated: SharedPreferences, key: String? ->
                updated.all[key]?.let { value ->
                    adapter.setProperty(key!!, value)
                }
            }
        preferences.registerOnSharedPreferenceChangeListener(listener)
        return Releaseable { preferences.unregisterOnSharedPreferenceChangeListener(listener) }
    }

    private fun dumpExistingProperties(prefs: Map<String, *>, target: RawPropertiesAdapter) {
        for ((key, value) in prefs) {
            target.setProperty(key, value!!)
        }
    }
}
