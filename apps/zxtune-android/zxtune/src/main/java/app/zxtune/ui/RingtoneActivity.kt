package app.zxtune.ui

import android.content.Context
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.provider.Settings
import android.view.View
import android.view.ViewGroup
import androidx.activity.result.contract.ActivityResultContracts
import androidx.annotation.RequiresApi
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.children
import app.zxtune.R
import app.zxtune.RingtoneService
import app.zxtune.TimeStamp
import app.zxtune.device.Permission

class RingtoneActivity : AppCompatActivity(R.layout.ringtone) {
    companion object {
        fun createIntent(ctx: Context, uri: Uri) = Intent(ctx, RingtoneActivity::class.java).apply {
            data = uri
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        fillUi()
        if (!Permission.canWriteSystemSettings(this)) {
            requestPermission()
        }
    }

    private fun fillUi() {
        val uri = requireNotNull(intent?.data)
        val listener = View.OnClickListener { v: View ->
            val tagValue = (v.tag as String).toInt()
            RingtoneService.execute(this, uri, TimeStamp.fromSeconds(tagValue.toLong()))
            finish()
        }
        requireNotNull(findViewById<ViewGroup>(R.id.ringtone_durations)).children.forEach {
            it.setOnClickListener(listener)
        }
    }

    @RequiresApi(23)
    private fun requestPermission() =
        registerForActivityResult(ActivityResultContracts.StartActivityForResult()) {
            if (!Permission.canWriteSystemSettings(this)) {
                finish()
            }
        }.launch(
            Intent(Settings.ACTION_MANAGE_WRITE_SETTINGS, Uri.parse("package:$packageName"))
        )
}
