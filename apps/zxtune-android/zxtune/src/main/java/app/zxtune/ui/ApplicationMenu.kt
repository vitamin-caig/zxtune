package app.zxtune.ui

import android.content.ActivityNotFoundException
import android.content.Intent
import android.net.Uri
import android.view.Menu
import android.view.MenuInflater
import android.view.MenuItem
import androidx.appcompat.app.AlertDialog
import androidx.core.view.MenuProvider
import androidx.fragment.app.FragmentActivity
import app.zxtune.MainService
import app.zxtune.PreferencesActivity
import app.zxtune.R
import app.zxtune.ResultActivity
import app.zxtune.analytics.Analytics
import app.zxtune.device.PowerManagement
import app.zxtune.ui.utils.item
import kotlin.system.exitProcess

class ApplicationMenu(private val activity: FragmentActivity) : MenuProvider {

    override fun onCreateMenu(menu: Menu, menuInflater: MenuInflater) =
        menuInflater.inflate(R.menu.main, menu)

    override fun onPrepareMenu(menu: Menu) = menu.run {
        item(R.id.action_about).intent = AboutActivity.createIntent(activity)
        item(R.id.action_prefs).intent = PreferencesActivity.createIntent(activity)
        item(R.id.action_problems).let { item ->
            PowerManagement.create(activity).hasProblem.observe(activity) {
                item.isVisible = it
            }
        }
    }

    override fun onMenuItemSelected(menuItem: MenuItem) = when (menuItem.itemId) {
        R.id.action_problems -> {
            setupPowerManagement()
            true
        }

        R.id.action_rate -> {
            rate()
            true
        }

        R.id.action_quit -> {
            quit()
            true
        }

        else -> false
    }

    private fun setupPowerManagement() = AlertDialog.Builder(activity).apply {
        setTitle(R.string.problems)
        setMessage(R.string.problem_power_management)
        setPositiveButton(R.string.problem_power_management_fixit) { _, _ ->
            activity.startActivity(ResultActivity.createSetupPowerManagementIntent(activity))
        }
    }.show()

    // TODO: find proper intent or hide menuitem if no sutable
    private fun rate() {
        if (tryOpenIntent(makeGooglePlayIntent()) || tryOpenIntent(makeMarketIntent())) {
            Analytics.sendUiEvent(Analytics.UI_ACTION_RATE)
        }
    }

    private fun tryOpenIntent(intent: Intent) = try {
        activity.startActivity(intent)
        true
    } catch (e: ActivityNotFoundException) {
        false
    }

    private fun makeGooglePlayIntent() = Intent(Intent.ACTION_VIEW).apply {
        data = Uri.parse("https://play.google.com/store/apps/details?id=${activity.packageName}")
        `package` = "com.android.vending"
    }

    private fun makeMarketIntent() = Intent(Intent.ACTION_VIEW).apply {
        data = Uri.parse("market://details?id=${activity.packageName}")
    }

    private fun quit(): Unit = activity.run {
        Analytics.sendUiEvent(Analytics.UI_ACTION_QUIT)
        stopService(MainService.createIntent(this, null))
        finishAffinity()
        exitProcess(0)
    }
}
