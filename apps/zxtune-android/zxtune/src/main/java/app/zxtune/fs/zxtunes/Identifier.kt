/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs.zxtunes

import android.net.Uri
import androidx.core.text.isDigitsOnly

/**
 * Paths
 *
 *
 * 1) zxtune:/
 * 2) zxtune:/${authors_folder}
 * 3) zxtune:/${authors_folder}/${author_name}?author=${author_id}
 * 4) zxtune:/${authors_folder}/${author_name}/${date}?author=${author_id}
 * 5) zxtune:/${authors_folder}/${author_name}/${Filename}?author=${author_id}&track=${track_id}
 * 6) zxtune:/${authors_folder}/${author_name}/${date}/${Filename}?author=${author_id}&track=${track_id}
 *
 *
 * means
 *
 *
 * 1) root
 * 2) authors root:
 * 3) specific author's root, modules without date, dates folders:
 * 4) author's modules by date:
 * 5) author's module without date:
 * 6) author's module with date
 *
 *
 * resolving executed sequentally in despite of explicit parameters. E.g.
 * author=${author_id} parameter is not analyzed for cases 1 and 2,
 * track=${track_id} parameter is not analyzed for cases 1, 2, 3 and 4
 */
object Identifier {
    private const val SCHEME = "zxtunes"
    private const val POS_CATEGORY = 0
    private const val POS_AUTHOR_NICK = 1
    private const val POS_AUTHOR_DATE = 2
    private const val POS_AUTHOR_TRACK = 2
    private const val POS_AUTHOR_DATE_TRACK = 3
    private const val PARAM_AUTHOR_ID = "author"
    private const val PARAM_TRACK_ID = "track"
    const val CATEGORY_AUTHORS = "Authors"

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
    fun forAuthor(author: Author): Uri.Builder = forCategory(CATEGORY_AUTHORS)
        .appendPath(author.nickname)
        .appendQueryParameter(PARAM_AUTHOR_ID, author.id.toString())

    @JvmStatic
    fun forAuthor(author: Author, date: Int): Uri.Builder = forCategory(CATEGORY_AUTHORS)
        .appendPath(author.nickname)
        .appendPath(date.toString())
        .appendQueryParameter(PARAM_AUTHOR_ID, author.id.toString())

    @JvmStatic
    fun findAuthor(uri: Uri, path: List<String>) = path.getOrNull(POS_AUTHOR_NICK)?.let { nick ->
        uri.getQueryParameter(PARAM_AUTHOR_ID)?.toIntOrNull()?.let { id ->
            Author(id, nick)
        }
    }

    @JvmStatic
    fun findDate(uri: Uri, path: List<String>) =
        path.getOrNull(POS_AUTHOR_DATE)?.takeIf { isDate(it) }?.toInt()

    private fun isDate(str: String) = str.length == 4 && str.isDigitsOnly()

    // Tracks
    @JvmStatic
    fun forTrack(parent: Uri.Builder, track: Track): Uri.Builder {
        track.date?.let { parent.appendPath(track.date.toString()) }
        return parent.appendPath(track.filename)
            .appendQueryParameter(PARAM_TRACK_ID, track.id.toString())
    }

    @JvmStatic
    fun findTrack(uri: Uri, path: List<String>) =
        path.getOrNull(POS_AUTHOR_DATE_TRACK)?.let { filename ->
            uri.getQueryParameter(PARAM_TRACK_ID)?.toIntOrNull()?.let { id ->
                Track(id, filename, date = path[POS_AUTHOR_DATE].toIntOrNull())
            }
        } ?: path.getOrNull(POS_AUTHOR_TRACK)?.let { filename ->
            uri.getQueryParameter(PARAM_TRACK_ID)?.toIntOrNull()?.let { id ->
                Track(id, filename)
            }
        }
}
