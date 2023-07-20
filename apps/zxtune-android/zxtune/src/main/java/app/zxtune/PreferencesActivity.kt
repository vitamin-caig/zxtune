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

class PreferencesActivity : AppCompatActivity(),
    PreferenceFragmentCompat.OnPreferenceStartFragmentCallback {

    companion object {
        @JvmStatic
        fun createIntent(ctx: Context) = Intent(ctx, PreferencesActivity::class.java)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Need to only create the first fragment
        supportFragmentManager.run {
            if (findFragmentById(android.R.id.content) == null) {
                beginTransaction()
                    .add(android.R.id.content, makeFragment(R.xml.preferences))
                    .disallowAddToBackStack()
                    .commit()
            }
        }

        Analytics.sendUiEvent(Analytics.UI_ACTION_PREFERENCES)
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

        companion object {
            private const val LAYOUT_KEY = "layout"
        }

        var layout
            get() = requireArguments().getInt(LAYOUT_KEY)
            set(value) {
                arguments = Bundle().apply {
                    putInt(LAYOUT_KEY, value)
                }
            }

        override fun onCreatePreferences(savedInstanceState: Bundle?, rootKey: String?) {
            preferenceManager.preferenceDataStore = Preferences.getDataStore(requireContext())
            setPreferencesFromResource(layout, rootKey)
        }
    }
}
