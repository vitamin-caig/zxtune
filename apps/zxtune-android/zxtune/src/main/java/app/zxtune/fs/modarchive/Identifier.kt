/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs.modarchive

import android.net.Uri

/**
 * Paths:
 *
 *
 * 1) modarchive:/
 * 2) modarchive:/Author
 * 3) modarchive:/Author/${author_name}?author=${author_id}
 * 4) modarchive:/Genre
 * 5) modarchive:/Genre/${genre_name}?genre=${genre_id}
 * 6) modarchive:/Random
 */
object Identifier {
    private const val SCHEME = "modarchive"
    private const val POS_CATEGORY = 0
    private const val POS_AUTHOR_NAME = 1
    private const val POS_GENRE_NAME = 1

    //private const val POS_TRACK_FILENAME = 2
    private const val PARAM_AUTHOR = "author"
    private const val PARAM_GENRE = "genre"
    private const val PARAM_TRACK = "track"
    const val CATEGORY_AUTHOR = "Author"
    const val CATEGORY_GENRE = "Genre"
    const val CATEGORY_RANDOM = "Random"

    // Root
    @JvmStatic
    fun forRoot(): Uri.Builder = Uri.Builder().scheme(SCHEME)

    @JvmStatic
    fun isFromRoot(uri: Uri) = SCHEME == uri.scheme

    // Categories
    @JvmStatic
    fun forCategory(category: String): Uri.Builder = forRoot().appendPath(category)

    @JvmStatic
    fun findCategory(path: List<String>) = path.getOrNull(POS_CATEGORY)

    // Authors
    @JvmStatic
    fun forAuthor(author: Author): Uri.Builder = forCategory(CATEGORY_AUTHOR)
        .appendPath(author.alias)
        .appendQueryParameter(PARAM_AUTHOR, author.id.toString())

    @JvmStatic
    fun findAuthor(uri: Uri, path: List<String>) = path.getOrNull(POS_AUTHOR_NAME)?.let { name ->
        uri.getQueryParameter(PARAM_AUTHOR)?.toIntOrNull()?.let { id ->
            Author(id, name)
        }
    }

    // Genres
    @JvmStatic
    fun forGenre(genre: Genre): Uri.Builder = forCategory(CATEGORY_GENRE)
        .appendPath(genre.name)
        .appendQueryParameter(PARAM_GENRE, genre.id.toString())

    @JvmStatic
    fun findGenre(uri: Uri, path: List<String>) = path.getOrNull(POS_GENRE_NAME)?.let { name ->
        uri.getQueryParameter(PARAM_GENRE)?.toIntOrNull()?.let { id ->
            Genre(id, name, 0 /*fake*/)
        }
    }

    // Tracks
    @JvmStatic
    fun forTrack(parent: Uri.Builder, track: Track): Uri.Builder = parent.appendPath(track.filename)
        .appendQueryParameter(PARAM_TRACK, track.id.toString())

    // do not allow files on root
    @JvmStatic
    fun findTrack(uri: Uri, path: List<String>) =
        path.takeIf { it.size >= 2 }?.last()?.let { name ->
            uri.getQueryParameter(PARAM_TRACK)?.toIntOrNull()?.let { id ->
                Track(id, name, "" /*fake*/, 0 /*fake*/)
            }
        }
}
