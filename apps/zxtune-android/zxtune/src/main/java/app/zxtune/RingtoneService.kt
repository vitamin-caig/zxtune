/**
 *
 * @file
 *
 * @brief
 *
 * @author vitamin.caig@gmail.com
 */
package app.zxtune

import android.content.ContentUris
import android.content.ContentValues
import android.content.Context
import android.content.Intent
import android.media.RingtoneManager
import android.net.Uri
import android.os.Environment
import android.os.Handler
import android.os.Looper
import android.provider.MediaStore
import android.widget.Toast
import androidx.lifecycle.LifecycleService
import app.zxtune.analytics.Analytics
import app.zxtune.core.Module
import app.zxtune.core.Properties
import app.zxtune.device.ui.Notifications
import app.zxtune.playback.FileIterator
import app.zxtune.playback.PlayableItem
import app.zxtune.sound.SamplesSource
import app.zxtune.sound.WaveWriteSamplesTarget
import app.zxtune.utils.AsyncWorker
import java.io.File

class RingtoneService : LifecycleService() {

    private val worker by lazy {
        AsyncWorker(TAG)
    }

    @Deprecated("Deprecated in Java")
    override fun onStart(intent: Intent?, startId: Int) {
        super.onStart(intent, startId)
        worker.execute {
            onHandleIntent(intent)
            stopSelf(startId)
        }
    }

    private fun onHandleIntent(intent: Intent?) {
        if (ACTION_MAKERINGTONE == intent?.action) {
            val module = requireNotNull(intent.getParcelableExtra<Uri>(EXTRA_MODULE))
            val seconds = intent.getLongExtra(EXTRA_DURATION_SECONDS, DEFAULT_DURATION_SECONDS)
            createRingtone(module, seconds.toInt())
        }
    }

    private fun createRingtone(source: Uri, seconds: Int) = runCatching {
        val item = load(source)
        item.module.use {
            val target = getTargetLocation(getModuleId(item), seconds)
            convert(item.module, seconds, target)
            setAsRingtone(item, seconds, target)
        }
        Analytics.sendSocialEvent(source, "app.zxtune", Analytics.SocialAction.RINGTONE)
    }.onFailure {
        LOG.w(it) { "Failed to create ringtone" }
        makeToast(it)
    }

    private fun load(uri: Uri) = FileIterator.create(applicationContext, uri).item

    private fun getTargetLocation(moduleId: Long, seconds: Int): File {
        val dir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_RINGTONES)
        if (dir.mkdirs()) {
            LOG.d { "Created ringtones directory" }
        }
        val filename = "${moduleId}_${seconds}.wav"
        LOG.d { "Dir: $dir filename: $filename" }
        return File(dir, filename)
    }

    private fun makeToast(e: Throwable) {
        LOG.w(e) { "Failed to create ringtone" }
        val msg = e.cause?.message ?: e.message
        val txt = getString(R.string.ringtone_creating_failed, msg)
        makeToast(txt, Toast.LENGTH_LONG)
    }

    private fun makeToast(text: String, duration: Int) {
        Handler(Looper.getMainLooper()).post {
            Toast.makeText(applicationContext, text, duration).show()
        }
    }

    private fun convert(module: Module, limit: Int, location: File) {
        makeToast(getString(R.string.ringtone_create_started), Toast.LENGTH_SHORT)
        val target = WaveWriteSamplesTarget(location.absolutePath)

        target.use {
            val sampleRate = target.sampleRate
            val player = module.createPlayer(sampleRate).apply {
                position = TimeStamp.EMPTY
                setProperty(Properties.Sound.LOOPED, 1)
            }
            player.use {
                val buffer = ShortArray(sampleRate * SamplesSource.Channels.COUNT)
                target.start()
                repeat(limit) {
                    player.render(buffer)
                    target.writeSamples(buffer)
                }
                target.stop()
            }
        }
    }

    private fun setAsRingtone(item: PlayableItem, limit: Int, path: File) {
        val values = createRingtoneData(item, limit, path)
        val ringtoneUri = createOrUpdateRingtone(values)
        RingtoneManager.setActualDefaultRingtoneUri(
            this,
            RingtoneManager.TYPE_RINGTONE,
            ringtoneUri
        )
        val title = values.getAsString(MediaStore.MediaColumns.TITLE)
        Notifications.sendEvent(
            applicationContext,
            R.drawable.ic_stat_notify_ringtone,
            R.string.ringtone_changed,
            title
        )
    }

    private fun createOrUpdateRingtone(values: ContentValues): Uri {
        val path = values.getAsString(MediaStore.MediaColumns.DATA)
        val tableUri = checkNotNull(MediaStore.Audio.Media.getContentUriForPath(path))
        return contentResolver.query(
            tableUri, arrayOf(MediaStore.MediaColumns._ID),
            "${MediaStore.MediaColumns.DATA} = ?", arrayOf(path), null
        )?.use { query ->
            if (query.moveToFirst()) {
                val id = query.getLong(0)
                ContentUris.withAppendedId(tableUri, id).also {
                    LOG.d { "Use indexed ringtone $it" }
                }
            } else {
                null
            }
        } ?: checkNotNull(contentResolver.insert(tableUri, values).also {
            LOG.d { "Registered new ringtone at $it" }
        })
    }

    companion object {
        private val TAG = RingtoneService::class.java.name
        private val LOG = Logger(TAG)
        private val ACTION_MAKERINGTONE = "$TAG.makeringtone"
        private const val EXTRA_MODULE = "module"
        private const val EXTRA_DURATION_SECONDS = "duration"
        private const val DEFAULT_DURATION_SECONDS: Long = 30

        fun execute(context: Context, module: Uri, duration: TimeStamp) {
            val intent = Intent(context, RingtoneService::class.java).apply {
                action = ACTION_MAKERINGTONE
                putExtra(EXTRA_MODULE, module)
                putExtra(EXTRA_DURATION_SECONDS, duration.toSeconds())
            }
            context.startService(intent)
        }

        private fun getModuleId(item: PlayableItem) = item.module.getProperty(
            "CRC" /*ZXTune.Module.Attributes.CRC*/,
            item.dataId.hashCode().toLong()
        )

        private fun createRingtoneData(
            item: PlayableItem,
            seconds: Int,
            path: File
        ) = ContentValues().apply {
            put(MediaStore.MediaColumns.DATA, path.absolutePath)
            val filename = item.dataId.displayFilename
            put(MediaStore.MediaColumns.DISPLAY_NAME, filename)
            put(MediaStore.Audio.Media.DURATION, seconds * 1000)
            val title = Util.formatTrackTitle(item.author, item.title, filename)
            put(MediaStore.MediaColumns.TITLE, "$title ($seconds)")
            put(MediaStore.Audio.Media.IS_RINGTONE, true)
            put(MediaStore.Audio.Media.IS_NOTIFICATION, false)
            put(MediaStore.Audio.Media.IS_ALARM, false)
            put(MediaStore.Audio.Media.IS_MUSIC, false)
            put(MediaStore.Audio.Media.MIME_TYPE, "audio/x-wav")
        }
    }
}
