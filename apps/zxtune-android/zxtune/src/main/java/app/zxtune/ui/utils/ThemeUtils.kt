package app.zxtune.ui.utils

import android.content.Context
import androidx.appcompat.app.AppCompatDelegate
import androidx.lifecycle.Observer
import app.zxtune.R
import app.zxtune.Releaseable
import app.zxtune.preferences.Preferences.getProviderClient

object ThemeUtils {
    private const val THEME_DARK = "dark"
    private const val THEME_LIGHT = "light"
    private const val THEME_SYSTEM = "system"

    @JvmStatic
    fun setupThemeChange(ctx: Context): Releaseable {
        val obs = Observer<String> { theme ->
            val mode = themeToMode(theme)
            AppCompatDelegate.setDefaultNightMode(mode)
        }
        val key = ctx.getString(R.string.pref_ui_theme_key)
        val values = ctx.resources.getStringArray(R.array.pref_ui_theme_values).apply {
            require(contentEquals(arrayOf(THEME_DARK, THEME_LIGHT, THEME_SYSTEM)))
        }
        val data = getProviderClient(ctx).getLive(key, values[0]).apply {
            observeForever(obs)
        }
        return Releaseable { data.removeObserver(obs) }
    }

    private fun themeToMode(theme: String) = when (theme) {
        THEME_LIGHT -> AppCompatDelegate.MODE_NIGHT_NO
        THEME_SYSTEM -> AppCompatDelegate.MODE_NIGHT_FOLLOW_SYSTEM
        else -> AppCompatDelegate.MODE_NIGHT_YES
    }
}
