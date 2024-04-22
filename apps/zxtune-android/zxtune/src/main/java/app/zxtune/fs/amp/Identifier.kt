/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs.amp

import android.net.Uri

/**
 * Paths:
 *
 *
 * 1) amp:/
 * 2) amp:/Handle
 * 3) amp:/Handle/${author_letter}
 * 4) amp:/Handle/${author_letter}/${author_name}?author=${author_id}
 * 5) amp:/Country
 * 6) amp:/Country/${country_name}?country=${country_id}
 * 7) amp:/Country/${country_name}/${author_name}?author=${author_id}&country=${country_id}
 * 8) amp:/Group
 * 9) amp:/Group/${group_name}?group=${group_id}
 * 10) amp:/Group/${group_name}/${author_name}?author=${author_id}&group=${group_id}
 * 11) amp:/Image/Author/${author_name}?author=${author_id}
 */
object Identifier {
    private const val SCHEME = "amp"
    private const val POS_CATEGORY = 0
    private const val POS_HANDLE_LETTER = 1
    private const val POS_COUNTRY_NAME = 1
    private const val POS_GROUP_NAME = 1
    private const val POS_AUTHOR_NAME = 2
    private const val POS_TRACK_FILENAME = 3
    private const val PARAM_COUNTRY = "country"
    private const val PARAM_GROUP = "group"
    private const val PARAM_AUTHOR = "author"
    private const val PARAM_TRACK = "track"
    private const val PARAM_SEED = "seed"
    const val CATEGORY_HANDLE = "Handle"
    const val CATEGORY_COUNTRY = "Country"
    const val CATEGORY_GROUP = "Group"
    const val CATEGORY_IMAGE = "Image"

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

    // Handles
    @JvmStatic
    fun forHandleLetter(letter: String): Uri.Builder =
        forCategory(CATEGORY_HANDLE).appendPath(letter)

    @JvmStatic
    fun isHandleLetter(letter: String) =
        letter.length == 1 && Character.isLetter(letter[0]) && Character.isUpperCase(letter[0])

    @JvmStatic
    fun findHandleLetter(uri: Uri, path: List<String>) = path.getOrNull(POS_HANDLE_LETTER)
        ?.takeIf { Catalog.NON_LETTER_FILTER == it || isHandleLetter(it) }

    // Countries
    @JvmStatic
    fun forCountry(country: Country): Uri.Builder =
        forCategory(CATEGORY_COUNTRY).appendPath(country.name)
            .appendQueryParameter(PARAM_COUNTRY, country.id.toString())

    @JvmStatic
    fun findCountry(uri: Uri, path: List<String>) = path.getOrNull(POS_COUNTRY_NAME)?.let { name ->
        uri.getQueryParameter(PARAM_COUNTRY)?.toIntOrNull()?.let { id ->
            Country(id, name)
        }
    }

    // Groups
    @JvmStatic
    fun forGroup(group: Group): Uri.Builder = forCategory(CATEGORY_GROUP).appendPath(group.name)
        .appendQueryParameter(PARAM_GROUP, group.id.toString())

    @JvmStatic
    fun findGroup(uri: Uri, path: List<String>) = path.getOrNull(POS_GROUP_NAME)?.let { name ->
        uri.getQueryParameter(PARAM_GROUP)?.toIntOrNull()?.let { id ->
            Group(id, name)
        }
    }

    // Authors
    @JvmStatic
    fun forAuthor(parent: Uri.Builder, author: Author): Uri.Builder =
        parent.appendPath(author.handle).appendQueryParameter(PARAM_AUTHOR, author.id.toString())

    @JvmStatic
    fun findAuthor(uri: Uri, path: List<String>) = path.getOrNull(POS_AUTHOR_NAME)?.let { name ->
        uri.getQueryParameter(PARAM_AUTHOR)?.toIntOrNull()?.let { id ->
            Author(id, name, ""/*fake*/)
        }
    }

    // Tracks
    @JvmStatic
    fun forTrack(parent: Uri.Builder, track: Track): Uri.Builder =
        parent.appendPath(track.filename).appendQueryParameter(PARAM_TRACK, track.id.toString())

    @JvmStatic
    fun findTrack(uri: Uri, path: List<String>) =
        path.getOrNull(POS_TRACK_FILENAME)?.let { filename ->
            uri.getQueryParameter(PARAM_TRACK)?.toIntOrNull()?.let { id ->
                Track(id, filename, 0/*fake*/)
            }
        }

    @JvmStatic
    fun forPictureOf(author: Author, seed: Int): Uri =
        forAuthor(forCategory(CATEGORY_IMAGE).appendPath("Author"), author).appendQueryParameter(
            PARAM_SEED, seed.toString()
        ).build()

    @JvmStatic
    fun findSeed(uri: Uri, path: List<String>) =
        uri.takeIf { it.isHierarchical }?.getQueryParameter(PARAM_SEED)?.toIntOrNull()
}
