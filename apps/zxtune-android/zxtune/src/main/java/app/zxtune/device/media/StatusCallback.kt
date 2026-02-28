package app.zxtune.device.media

import android.content.Context
import android.net.Uri
import android.os.Build
import android.support.v4.media.MediaMetadataCompat
import android.support.v4.media.session.MediaSessionCompat
import android.support.v4.media.session.PlaybackStateCompat
import android.text.TextUtils
import app.zxtune.Logger
import app.zxtune.Releaseable
import app.zxtune.TimeStamp
import app.zxtune.Util
import app.zxtune.core.Identifier
import app.zxtune.core.ModuleAttributes
import app.zxtune.coverart.BitmapLoader
import app.zxtune.coverart.CoverartProviderClient
import app.zxtune.fs.Vfs
import app.zxtune.fs.VfsExtensions
import app.zxtune.fs.shareUrl
import app.zxtune.playback.Item
import app.zxtune.playback.PlayableItem
import app.zxtune.playback.PlaybackControl
import app.zxtune.playback.PlaybackService
import kotlinx.coroutines.CoroutineName
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.cancel
import kotlinx.coroutines.flow.filterNotNull
import kotlinx.coroutines.flow.launchIn
import kotlinx.coroutines.flow.onEach
import kotlinx.coroutines.launch
import java.util.concurrent.atomic.AtomicReference

//! Events gate from local service to mediasession
internal class StatusCallback private constructor(
    private val ctx: Context,
    svc: PlaybackService,
    private val session: MediaSessionCompat
) : Releaseable {
    private val builder = PlaybackStateCompat.Builder()
    private val scope = CoroutineScope(CoroutineName("SessionStatusCallback") + Dispatchers.IO)
    private val coverartClient = CoverartProviderClient(ctx)
    private val lockscreenImageUrl = AtomicReference<Uri>()
    private val bitmapLoader by lazy {
        BitmapLoader("lockscreen", ctx, maxSize = 5, maxImageSize = 320)
    }

    init {
        builder.setActions(
            PlaybackStateCompat.ACTION_SKIP_TO_PREVIOUS or PlaybackStateCompat.ACTION_PLAY_PAUSE or PlaybackStateCompat.ACTION_PLAY or PlaybackStateCompat.ACTION_PAUSE or PlaybackStateCompat.ACTION_STOP or PlaybackStateCompat.ACTION_SKIP_TO_NEXT
        )
        svc.playbackControl.run {
            session.setShuffleMode(sequenceMode.toShuffleMode())
            session.setRepeatMode(trackMode.toRepeatMode())
        }
        svc.nowPlaying.filterNotNull().onEach { onItemChanged(it) }.launchIn(scope)
        svc.state.onEach { onStateChanged(it.first, it.second) }.launchIn(scope)
    }

    override fun release() {
        scope.cancel()
    }

    private fun onStateChanged(state: PlaybackControl.State, pos: TimeStamp) {
        builder.setState(state.toState(), pos.toMilliseconds(), 1f)
        session.run {
            setPlaybackState(builder.build())
            isActive = state !== PlaybackControl.State.STOPPED
        }
    }

    private fun onItemChanged(item: Item) = try {
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
            fillAlbumArtwork(it, builder)
        }
        session.setMetadata(builder.build())
    } catch (e: Exception) {
        LOG.w(e) { "onItemChanged()" }
    }

    private fun fillObjectUrls(
        dataId: Identifier, builder: MediaMetadataCompat.Builder
    ): Uri? = with(builder) {
        try {
            val obj = Vfs.resolve(dataId.dataLocation)
            putNonEmptyString(VfsExtensions.SHARE_URL, obj.shareUrl)
            coverartClient.getMediaUris(dataId)?.let { uris ->
                var preferableResult: Uri? = null
                for (key in arrayOf(
                    MediaMetadataCompat.METADATA_KEY_DISPLAY_ICON_URI,
                    MediaMetadataCompat.METADATA_KEY_ALBUM_ART_URI,
                    MediaMetadataCompat.METADATA_KEY_ART_URI
                )) {
                    uris.getParcelable<Uri>(key)?.let {
                        preferableResult = it
                        putString(key, it.toString())
                    }
                }
                return preferableResult
            }
        } catch (e: Exception) {
            LOG.w(e) { "Failed to get object urls" }
        }
        return null
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
        fun subscribe(ctx: Context, svc: PlaybackService, session: MediaSessionCompat) =
            StatusCallback(ctx, svc, session)
    }
}

private fun MediaMetadataCompat.Builder.putNonEmptyString(key: String, value: String?) = apply {
    if (!TextUtils.isEmpty(value)) {
        putString(key, value)
    }
}
