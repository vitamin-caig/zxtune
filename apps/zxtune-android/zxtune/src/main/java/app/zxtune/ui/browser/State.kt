/**
 * @file
 * @brief File browser state helper
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.ui.browser

import android.content.Context
import android.net.Uri
import android.os.Bundle
import androidx.annotation.VisibleForTesting
import app.zxtune.preferences.DataStore
import app.zxtune.preferences.Preferences.getDataStore
import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

internal class State @VisibleForTesting constructor(
    ctx: Context, private val ioDispatcher: CoroutineDispatcher
) {
    private val prefs = getDataStore(ctx)
    private var _current: PathAndPosition? = null
    private var current
        get() = _current ?: PathAndPosition(prefs).also {
            _current = it
        }
        set(value) {
            _current = value
        }

    suspend fun getCurrentPath(): Uri = withContext(ioDispatcher) {
        current.path
    }

    suspend fun updateCurrentPath(uri: Uri) = withContext(ioDispatcher) {
        if (current.path != uri) {
            current = switchTo(uri).apply {
                save(this)
            }
        }
        current.viewPosition
    }

    private fun switchTo(uri: Uri) = when {
        isNested(current.path, uri) -> innerPath(uri)
        isNested(uri, current.path) -> outerPath(uri)
        else -> newPath(uri)
    }

    suspend fun updateCurrentPosition(pos: Int) = withContext(ioDispatcher) {
        current.run {
            viewPosition = pos
            save(this)
        }
    }

    private fun innerPath(newPath: Uri) = PathAndPosition(current.index + 1, newPath)

    private fun outerPath(newPath: Uri): PathAndPosition {
        for (idx in current.index - 1 downTo 0) {
            val pos = PathAndPosition(idx, prefs)
            if (newPath == pos.path) {
                return pos
            }
        }
        return PathAndPosition(0, newPath)
    }

    private fun newPath(newPath: Uri) = PathAndPosition(0, newPath)

    private fun save(data: PathAndPosition) {
        Bundle().apply {
            data.storeTo(this)
        }.also {
            prefs.putBatch(it)
        }
    }

    private class PathAndPosition {
        val index: Int
        val path: Uri
        var viewPosition: Int

        constructor(prefs: DataStore) : this(prefs.getInt(CURRENT_KEY, 0), prefs)

        constructor(idx: Int, prefs: DataStore) {
            index = idx
            path = Uri.parse(prefs.getString(pathKey, ""))
            viewPosition = prefs.getInt(posKey, 0)
        }

        constructor(idx: Int, path: Uri) {
            this.index = idx
            this.path = path
            this.viewPosition = 0
        }

        fun storeTo(bundle: Bundle) = with(bundle) {
            putString(pathKey, path.toString())
            putInt(posKey, viewPosition)
            putInt(CURRENT_KEY, index)
        }

        private val pathKey
            get() = "browser_${index}_path"

        private val posKey: String
            get() = "browser_${index}_viewpos"
    }

    companion object {
        private const val CURRENT_KEY = "browser_current"

        var create: (Context) -> State = { ctx -> State(ctx, Dispatchers.IO) }
            @VisibleForTesting set

        // checks if rh is nested path relative to lh
        private fun isNested(lh: Uri, rh: Uri) = lh.scheme == rh.scheme && rh.path?.let { rhPath ->
            when (val lhPath = lh.path) {
                null -> true
                rhPath -> isNestedSubpath(lh.fragment, rh.fragment)
                else -> rhPath.startsWith(lhPath)
            }
        } ?: false

        private fun isNestedSubpath(lh: String?, rh: String?) =
            rh != null && (lh == null || rh.startsWith(lh))
    }
}
