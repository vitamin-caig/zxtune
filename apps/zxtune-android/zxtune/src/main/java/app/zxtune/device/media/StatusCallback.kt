package app.zxtune.device.media

import android.content.Context
import android.net.Uri
import android.os.Build
import android.support.v4.media.MediaMetadataCompat
import android.support.v4.media.session.MediaSessionCompat
import android.support.v4.media.session.PlaybackStateCompat
import android.text.TextUtils
import android.widget.Toast
import app.zxtune.Logger
import app.zxtune.TimeStamp
import app.zxtune.Util
import app.zxtune.core.Identifier
import app.zxtune.core.ModuleAttributes
import app.zxtune.coverart.BitmapLoader
import app.zxtune.coverart.CoverartProviderClient
import app.zxtune.coverart.CoverartService
import app.zxtune.fs.Vfs
import app.zxtune.fs.VfsExtensions
import app.zxtune.fs.shareUrl
import app.zxtune.playback.Callback
import app.zxtune.playback.Item
import app.zxtune.playback.PlayableItem
import app.zxtune.playback.PlaybackControl
import app.zxtune.playback.PlaybackService
import kotlinx.coroutines.MainScope
import kotlinx.coroutines.launch
import java.util.concurrent.atomic.AtomicReference

//! Events gate from local service to mediasession
internal class StatusCallback private constructor(
    private val ctx: Context, private val session: MediaSessionCompat
) : Callback {
    private val builder = PlaybackStateCompat.Builder()
    private val scope = MainScope()
    private val lockscreenImageUrl = AtomicReference<Uri>()
    private val bitmapLoader by lazy {
        BitmapLoader("lockscreen", ctx, maxSize = 5, maxImageSize = 320)
    }

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
            if (item is PlayableItem) {
                putNonEmptyString(
                    ModuleAttributes.CHANNELS_NAMES,
                    item.module.getProperty(ModuleAttributes.CHANNELS_NAMES, "")
                )
            }
            putLong(MediaMetadataCompat.METADATA_KEY_DURATION, item.duration.toMilliseconds())
            putLong(ModuleAttributes.SIZE, item.size)
            putString(MediaMetadataCompat.METADATA_KEY_MEDIA_URI, dataId.toString())
            putString(MediaMetadataCompat.METADATA_KEY_MEDIA_ID, item.id.toString())
        }
        fillObjectUrls(dataId, builder).takeIf { Build.VERSION.SDK_INT <= 29 }?.let {
            // https://developer.android.com/media/legacy/mediasession#album_artwork
            fillAlbumArtwork(CoverartProviderClient.getUriFor(it), builder)
        }
        session.setMetadata(builder.build())
    } catch (e: Exception) {
        LOG.w(e) { "onItemChanged()" }
    }

    override fun onError(e: String) {
        scope.launch {
            Toast.makeText(ctx, e, Toast.LENGTH_SHORT).show()
        }
    }

    private fun fillAlbumArtwork(uri: Uri, builder: MediaMetadataCompat.Builder) {
        bitmapLoader.getCached(uri)?.bitmap?.let {
            LOG.d { "Cached album art" }
            builder.putBitmap(MediaMetadataCompat.METADATA_KEY_ART, it)
            return
        }
        lockscreenImageUrl.set(uri)
        scope.launch {
            bitmapLoader.load(uri).bitmap?.let { bitmap ->
                if (lockscreenImageUrl.compareAndSet(uri, null)) {
                    LOG.d { "Fetched album art" }
                    builder.putBitmap(MediaMetadataCompat.METADATA_KEY_ART, bitmap)
                    session.setMetadata(builder.build())
                } else {
                    LOG.d { "Drop outdated album art retrieval" }
                }
            }
        }
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
            dataId: Identifier, builder: MediaMetadataCompat.Builder
        ): Uri? = with(builder) {
            try {
                val location = dataId.dataLocation
                val obj = Vfs.resolve(location)
                putNonEmptyString(VfsExtensions.SHARE_URL, obj.shareUrl)
                with(CoverartService.get()) {
                    val coverArt = coverArtOf(dataId)
                    val albumArt = albumArtOf(dataId, obj)
                    val displayIcon = iconOf(dataId)
                    maybeAddImageUri(MediaMetadataCompat.METADATA_KEY_ART_URI, coverArt)
                    maybeAddImageUri(MediaMetadataCompat.METADATA_KEY_ALBUM_ART_URI, albumArt)
                    maybeAddImageUri(MediaMetadataCompat.METADATA_KEY_DISPLAY_ICON_URI, displayIcon)
                    return coverArt ?: albumArt ?: displayIcon
                }
            } catch (e: Exception) {
                LOG.w(e) { "Failed to get object urls" }
            }
            return null
        }
    }
}

private fun MediaMetadataCompat.Builder.putNonEmptyString(key: String, value: String?) = apply {
    if (!TextUtils.isEmpty(value)) {
        putString(key, value)
    }
}

private fun MediaMetadataCompat.Builder.maybeAddImageUri(key: String, value: Uri?) = apply {
    if (value != null) {
        putString(key, CoverartProviderClient.getUriFor(value).toString())
    }
}
