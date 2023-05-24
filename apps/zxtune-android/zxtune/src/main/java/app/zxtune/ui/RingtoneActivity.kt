package app.zxtune.ui

import android.content.Context
import android.content.Intent
import android.net.Uri
import android.view.View
import android.view.ViewGroup
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.children
import app.zxtune.R
import app.zxtune.RingtoneService
import app.zxtune.TimeStamp

class RingtoneActivity : AppCompatActivity(R.layout.ringtone) {
    companion object {
        fun createIntent(ctx: Context, uri: Uri) = Intent(ctx, RingtoneActivity::class.java).apply {
            data = uri
        }
    }

    override fun onStart() {
        super.onStart()
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
}
