package app.zxtune.device.media

import android.content.Context
import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.drawable.BitmapDrawable
import android.net.Uri
import android.os.Handler
import android.os.Looper
import android.support.v4.media.MediaMetadataCompat
import android.support.v4.media.session.MediaSessionCompat
import android.support.v4.media.session.PlaybackStateCompat
import android.text.TextUtils
import android.widget.Toast
import androidx.core.content.res.ResourcesCompat
import app.zxtune.Logger
import app.zxtune.TimeStamp
import app.zxtune.Util
import app.zxtune.core.ModuleAttributes
import app.zxtune.fs.Vfs
import app.zxtune.fs.VfsExtensions
import app.zxtune.fs.VfsFile
import app.zxtune.fs.icon
import app.zxtune.fs.provider.VfsProviderClient.Companion.getFileUriFor
import app.zxtune.fs.shareUrl
import app.zxtune.playback.Callback
import app.zxtune.playback.Item
import app.zxtune.playback.PlaybackControl
import app.zxtune.playback.PlaybackService

//! Events gate from local service to mediasession
internal class StatusCallback private constructor(
    private val ctx: Context, private val session: MediaSessionCompat
) : Callback {
    private val builder = PlaybackStateCompat.Builder()
    private val handler = Handler(Looper.getMainLooper())

    init {
        builder.setActions(
            PlaybackStateCompat.ACTION_SKIP_TO_PREVIOUS or PlaybackStateCompat.ACTION_PLAY_PAUSE or PlaybackStateCompat.ACTION_PLAY or PlaybackStateCompat.ACTION_PAUSE or PlaybackStateCompat.ACTION_STOP or PlaybackStateCompat.ACTION_SKIP_TO_NEXT
        )
    }

    override fun onInitialState(state: PlaybackControl.State) =
        onStateChanged(state, TimeStamp.EMPTY)

    override fun onStateChanged(state: PlaybackControl.State, pos: TimeStamp) {
        builder.setState(state.toState(), pos.toMilliseconds(), 1f)
        session.run {
            setPlaybackState(builder.build())
            isActive = state !== PlaybackControl.State.STOPPED
        }
    }

    override fun onItemChanged(item: Item) = try {
        val dataId = item.dataId
        val builder = MediaMetadataCompat.Builder().apply {
            val title = Util.formatTrackTitle(item.title, dataId)
            putString(MediaMetadataCompat.METADATA_KEY_DISPLAY_TITLE, title)
            putString(MediaMetadataCompat.METADATA_KEY_TITLE, title)
            val author = item.author
            if (author.isNotEmpty()) {
                putString(MediaMetadataCompat.METADATA_KEY_DISPLAY_SUBTITLE, author)
                putString(MediaMetadataCompat.METADATA_KEY_ARTIST, author)
            } else {
                // Do not localize
                putString(MediaMetadataCompat.METADATA_KEY_ARTIST, "Unknown artist")
            }
            putNonEmptyString(ModuleAttributes.TITLE, item.title)
            putNonEmptyString(ModuleAttributes.AUTHOR, item.author)
            putNonEmptyString(ModuleAttributes.COMMENT, item.comment)
            putNonEmptyString(ModuleAttributes.PROGRAM, item.program)
            putNonEmptyString(ModuleAttributes.STRINGS, item.strings)
            putLong(MediaMetadataCompat.METADATA_KEY_DURATION, item.duration.toMilliseconds())
            putBitmap(
                MediaMetadataCompat.METADATA_KEY_DISPLAY_ICON, getLocationIcon(dataId.dataLocation)
            )
            putString(MediaMetadataCompat.METADATA_KEY_MEDIA_URI, dataId.toString())
            putString(MediaMetadataCompat.METADATA_KEY_MEDIA_ID, item.id.toString())
        }
        fillObjectUrls(dataId.dataLocation, dataId.subPath, builder)
        session.setMetadata(builder.build())
    } catch (e: Exception) {
        LOG.w(e) { "onItemChanged()" }
    }

    override fun onError(e: String) {
        handler.post { Toast.makeText(ctx, e, Toast.LENGTH_SHORT).show() }
    }

    private fun getLocationIcon(location: Uri) = try {
        val drawable = getLocationIconResource(location)?.let {
            ResourcesCompat.getDrawableForDensity(ctx.resources, it, 320 /*XHDPI*/, null)
        }
        when (drawable) {
            is BitmapDrawable -> drawable.bitmap
            null -> null
            else -> Bitmap.createBitmap(64, 64, Bitmap.Config.ARGB_8888).apply {
                val canvas = Canvas(this)
                drawable.setBounds(0, 0, canvas.width, canvas.height)
                drawable.draw(canvas)
            }
        }
    } catch (e: Exception) {
        LOG.w(e) { "Failed to get location icon" }
        null
    }

    companion object {
        private val LOG = Logger(StatusCallback::class.java.name)

        @JvmStatic
        fun subscribe(
            ctx: Context, svc: PlaybackService, session: MediaSessionCompat
        ) = svc.subscribe(StatusCallback(ctx, session).also {
            svc.playbackControl.run {
                session.setShuffleMode(sequenceMode.toShuffleMode())
                session.setRepeatMode(trackMode.toRepeatMode())
            }
        })

        private fun fillObjectUrls(
            location: Uri, subpath: String, builder: MediaMetadataCompat.Builder
        ) {
            try {
                val obj = Vfs.resolve(location)
                builder.putNonEmptyString(VfsExtensions.SHARE_URL, obj.shareUrl)
                if (subpath.isEmpty() && obj is VfsFile) {
                    if (null != Vfs.getCacheOrFile(obj)) {
                        builder.putNonEmptyString(
                            VfsExtensions.FILE, getFileUriFor(location).toString()
                        )
                    }
                }
            } catch (e: Exception) {
                LOG.w(e) { "Failed to get object urls" }
            }
        }

        private fun getLocationIconResource(location: Uri) =
            Vfs.resolve(Uri.Builder().scheme(location.scheme).build()).icon
    }
}

private fun MediaMetadataCompat.Builder.putNonEmptyString(key: String, value: String?) = apply {
    if (!TextUtils.isEmpty(value)) {
        putString(key, value)
    }
}
