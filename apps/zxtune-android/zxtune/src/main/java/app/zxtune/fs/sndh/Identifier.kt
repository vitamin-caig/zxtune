package app.zxtune.fs.sndh

import android.net.Uri

/**
 * Paths:
 *
 * sndh:/
 * sndh:/Authors
 * sndh:/Authors/{author_name}
 * sndh:/Authors/{author_name}/{track_title}?track={track_id}
 * sndh:/Random
 * sndh:/Random/{track_title}?track={track_id}
 */
object Identifier {
    private const val SCHEME = "sndh"
    private const val PARAM_TRACK = "track"
    const val CATEGORY_AUTHORS = "Authors"
    const val CATEGORY_RANDOM = "Random"

    fun forRoot(): Uri.Builder = Uri.Builder().scheme(SCHEME)

    fun isFromRoot(uri: Uri) = SCHEME == uri.scheme

    fun forCategory(category: String): Uri.Builder = forRoot().appendPath(category)

    fun forAuthor(author: Author): Uri.Builder = forCategory(CATEGORY_AUTHORS).appendPath(author.name)

    fun forTrack(parent: Uri.Builder, track: Track): Uri.Builder = parent.appendPath(track.title)
        .appendQueryParameter(PARAM_TRACK, track.id.toString())

    fun findCategory(path: List<String>) = path.firstOrNull()

    fun findAuthor(uri: Uri, path: List<String>) = path.run {
        val isAuthorPath = size == 2 || size >= 3 && uri.getQueryParameter(PARAM_TRACK) != null
        if (findCategory(this) == CATEGORY_AUTHORS && isAuthorPath) {
            Author(get(1))
        } else {
            null
        }
    }

    fun findTrack(uri: Uri, path: List<String>) = path.run {
        when {
            findCategory(this) == CATEGORY_AUTHORS && size >= 3 -> findTrackId(uri, last())
            findCategory(this) == CATEGORY_RANDOM && size >= 2 -> findTrackId(uri, last())
            else -> null
        }
    }

    private fun findTrackId(uri: Uri, title: String) = uri.getQueryParameter(PARAM_TRACK)
        ?.toIntOrNull()?.let { Track(it, title, "") }
}
