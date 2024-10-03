/**
 * @file
 * @brief Preferences activity
 * @author vitamin.caig@gmail.com
 */
package app.zxtune

import android.content.Context
import android.content.Intent
import android.os.Bundle
import androidx.annotation.XmlRes
import androidx.appcompat.app.AppCompatActivity
import androidx.preference.Preference
import androidx.preference.PreferenceFragmentCompat
import app.zxtune.analytics.Analytics
import app.zxtune.preferences.Preferences
import app.zxtune.ui.utils.FragmentIntProperty
import app.zxtune.ui.utils.ThemeUtils

class PreferencesActivity : AppCompatActivity(),
    PreferenceFragmentCompat.OnPreferenceStartFragmentCallback {

    companion object {
        @JvmStatic
        fun createIntent(ctx: Context) = Intent(ctx, PreferencesActivity::class.java)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        ThemeUtils.setupThemeChange(this, this)

        // Need to only create the first fragment
        supportFragmentManager.run {
            if (findFragmentById(android.R.id.content) == null) {
                beginTransaction()
                    .add(android.R.id.content, makeFragment(R.xml.preferences))
                    .disallowAddToBackStack()
                    .commit()
            }
        }

        Analytics.sendUiEvent(Analytics.UiAction.PREFERENCES)
    }

    override fun onPreferenceStartFragment(
        caller: PreferenceFragmentCompat,
        pref: Preference
    ) = if ("zxtune.sound.mixer.3" == pref.key) {
        supportFragmentManager
            .beginTransaction()
            .replace(android.R.id.content, makeFragment(R.xml.preferences_mixer3))
            .addToBackStack(null)
            .commit()
        true
    } else {
        false
    }

    private fun makeFragment(@XmlRes layout: Int) = PrefFragment().apply {
        this.layout = layout
    }

    class PrefFragment : PreferenceFragmentCompat() {
        var layout by FragmentIntProperty

        override fun onCreatePreferences(savedInstanceState: Bundle?, rootKey: String?) {
            preferenceManager.preferenceDataStore = Preferences.getDataStore(requireContext())
            setPreferencesFromResource(layout, rootKey)
        }
    }
}
