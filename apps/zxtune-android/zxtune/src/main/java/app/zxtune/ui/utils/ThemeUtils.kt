package app.zxtune.ui.utils

import android.content.Context
import androidx.appcompat.app.AppCompatDelegate
import androidx.lifecycle.LifecycleOwner
import androidx.startup.Initializer
import app.zxtune.R
import app.zxtune.preferences.Preferences
import app.zxtune.preferences.Preferences.getProviderClient

object ThemeUtils {
    private const val THEME_DARK = "dark"
    private const val THEME_LIGHT = "light"
    private const val THEME_SYSTEM = "system"

    private const val THEME_DEFAULT = THEME_DARK

    private const val PREF_KEY = "ui.theme"

    @JvmStatic
    fun setupTheme(ctx: Context) {
        if (needApplyTheme()) {
            require(PREF_KEY == ctx.getString(R.string.pref_ui_theme_key))
            require(THEME_DEFAULT == ctx.getString(R.string.pref_ui_theme_default))
            require(
                arrayOf(
                    THEME_DARK, THEME_LIGHT, THEME_SYSTEM
                ).contentEquals(ctx.resources.getStringArray(R.array.pref_ui_theme_values))
            )
            val value = Preferences.getDefaultSharedPreferences(ctx).getString(
                PREF_KEY, THEME_DEFAULT
            )
            applyTheme(value!!)
        }
    }

    fun setupThemeChange(ctx: Context, owner: LifecycleOwner) = owner.whenLifecycleStarted {
        getProviderClient(ctx).watchString(PREF_KEY).collect { applyTheme(it) }
    }

    private fun needApplyTheme() =
        AppCompatDelegate.getDefaultNightMode() == AppCompatDelegate.MODE_NIGHT_UNSPECIFIED

    private fun applyTheme(theme: String) {
        val mode = themeToMode(theme)
        AppCompatDelegate.setDefaultNightMode(mode)
    }

    private fun themeToMode(theme: String) = when (theme) {
        THEME_LIGHT -> AppCompatDelegate.MODE_NIGHT_NO
        THEME_SYSTEM -> AppCompatDelegate.MODE_NIGHT_FOLLOW_SYSTEM
        else -> AppCompatDelegate.MODE_NIGHT_YES
    }
}

class ThemeInitializer : Initializer<Unit> {
    override fun create(context: Context) = ThemeUtils.setupTheme(context)

    override fun dependencies(): List<Class<out Initializer<*>>> = emptyList()
}