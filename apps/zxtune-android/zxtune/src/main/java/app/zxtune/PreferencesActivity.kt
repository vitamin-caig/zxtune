/**
 * @file
 * @brief Preferences activity
 * @author vitamin.caig@gmail.com
 */
package app.zxtune

import android.app.Application
import android.content.Context
import android.content.Intent
import android.os.Bundle
import androidx.annotation.XmlRes
import androidx.appcompat.app.AppCompatActivity
import androidx.fragment.app.Fragment
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.preference.Preference
import androidx.preference.PreferenceDataStore
import androidx.preference.PreferenceFragmentCompat
import app.zxtune.analytics.Analytics
import app.zxtune.preferences.DataStore

class PreferencesActivity : AppCompatActivity(),
    PreferenceFragmentCompat.OnPreferenceStartFragmentCallback {

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
            preferenceManager.preferenceDataStore = Model.of(this).store
            setPreferencesFromResource(layout, rootKey)
        }
    }

    // public for provider
    class Model(app: Application) : AndroidViewModel(app) {
        val store: PreferenceDataStore = DataStore(app)

        companion object {
            fun of(owner: Fragment) = owner.requireActivity().let { activity ->
                ViewModelProvider(
                    activity,
                    ViewModelProvider.AndroidViewModelFactory.getInstance(activity.application)
                )[Model::class.java]
            }
        }
    }
}
