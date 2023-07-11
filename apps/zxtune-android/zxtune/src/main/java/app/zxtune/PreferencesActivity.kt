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
                    .add(android.R.id.content, makeMainFragment())
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
            .replace(android.R.id.content, makeMixer3Fragment())
            .addToBackStack(null)
            .commit()
        true
    } else {
        false
    }

    private fun makeMainFragment() = PrefFragment(R.xml.preferences)
    private fun makeMixer3Fragment() = PrefFragment(R.xml.preferences_mixer3)

    class PrefFragment(@XmlRes private val layout: Int) : PreferenceFragmentCompat() {
        override fun onCreatePreferences(savedInstanceState: Bundle?, rootKey: String?) {
            preferenceManager.preferenceDataStore = Preferences.getDataStore(requireContext())
            setPreferencesFromResource(layout, rootKey)
        }
    }
}
