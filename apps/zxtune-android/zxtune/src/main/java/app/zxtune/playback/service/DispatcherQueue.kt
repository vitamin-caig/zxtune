package app.zxtune.playback.service

import android.content.Context
import android.net.Uri
import app.zxtune.Logger
import app.zxtune.fs.provider.Schema
import app.zxtune.fs.provider.VfsProviderClient
import app.zxtune.playlist.ProviderClient
import app.zxtune.utils.ifNotNulls
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.flatMapLatest

class DispatcherQueue(private val ctx: Context) : Queue {
    private val loader = Loader()
    private val vfs by lazy {
        VfsProviderClient(ctx)
    }
    private var shuffledCache = false
    private val _currentQueue = MutableStateFlow<Queue>(EmptyQueue)
    private var currentQueue
        get() = _currentQueue.value
        set(value) {
            if (_currentQueue.value !== value) {
                _currentQueue.value.release()
                value.shuffled = shuffledCache
                _currentQueue.value = value
            }
        }

    override suspend fun activate(uri: Uri) = findQueue(uri)?.let {
        LOG.d { "Activate queue $it for $uri" }
        currentQueue = it
        it.activate(uri)
    } ?: Unit

    override suspend fun next() = currentQueue.next()

    override suspend fun prev() = currentQueue.prev()

    @OptIn(ExperimentalCoroutinesApi::class)
    override val items
        get() = _currentQueue.flatMapLatest { it.items }

    override var shuffled
        get() = currentQueue.shuffled
        set(value) {
            shuffledCache = value
            currentQueue.shuffled = value
        }

    override fun release() = currentQueue.release()

    private suspend fun findQueue(uri: Uri): Queue? = ProviderClient.findId(uri)?.let { id ->
        if (id.playlist == (currentQueue as? PlaylistQueue)?.playlist) {
            LOG.d { "Reuse playlist queue" }
            currentQueue
        } else {
            PlaylistQueue(ctx, loader, id.playlist)
        }
    } ?: run {
        var played: Uri? = null
        var parent: Uri? = null
        var dirWithFeed: Uri? = null
        vfs.resolve(uri, object : VfsProviderClient.ListingCallback {
            override fun onProgress(status: Schema.Status.Progress) = Unit
            override fun onDir(dir: Schema.Content.Dir) {
                parent = parent ?: dir.uri
                dirWithFeed = dirWithFeed ?: if (dir.hasFeed) dir.uri else null
            }

            override fun onFile(file: Schema.Content.File) {
                if (parent == null && dirWithFeed == null) {
                    played = played ?: file.uri
                }
            }
        })
        dirWithFeed?.let { feedUri ->
            if (feedUri == (currentQueue as? FeedQueue)?.context) {
                LOG.d { "Reuse feed queue for $parent" }
                currentQueue
            } else {
                FeedQueue(vfs, feedUri, loader)
            }
        } ?: ifNotNulls(played, parent) { _, parent ->
            if (parent == (currentQueue as? FolderQueue)?.context) {
                LOG.d { "Reuse folder queue for $parent" }
                currentQueue
            } else {
                FolderQueue(vfs, parent, loader)
            }
        }
    }

    private companion object {
        private val LOG = Logger(DispatcherQueue::class.java.name)
    }
}