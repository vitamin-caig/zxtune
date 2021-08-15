/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs.zxart

import android.net.Uri

/**
 * Paths:
 *
 *
 * zxart:
 *
 *
 * zxart://Authors
 * zxart://Authors/${nick}?author=${author_id}
 * zxart://Authors/${nick}/${year}?author=${author_id}
 * zxart://Authors/${nick}/${year}/${filename}?author=${author_id}&track=${track_id}
 *
 *
 * zxart://Parties
 * zxart://Parties/${year}
 * zxart://Parties/${year}/${name}?party=${party_id}
 * zxart://Parties/${year}/${name}/${compo}?party=${party_id}
 * zxart://Parties/${year}/${name}/${compo}/${filename}?party=${party_id}&track=${track_id}
 *
 *
 * zxart://Top
 * zxart://Top/${filename}?track=${track_id}
 */
object Identifier {
    private const val SCHEME = "zxart"

    // indices of parts in path
    private const val POS_CATEGORY = 0
    private const val POS_AUTHOR_NICK = 1
    private const val POS_AUTHOR_YEAR = 2
    private const val POS_AUTHOR_TRACK = 3
    private const val POS_PARTY_YEAR = 1
    private const val POS_PARTY_NAME = 2
    private const val POS_PARTY_COMPO = 3
    private const val POS_PARTY_TRACK = 4
    private const val POS_TOP_TRACK = 1
    private const val PARAM_AUTHOR_ID = "author"
    private const val PARAM_PARTY_ID = "party"
    private const val PARAM_TRACK_ID = "track"
    const val CATEGORY_AUTHORS = "Authors"
    const val CATEGORY_PARTIES = "Parties"
    const val CATEGORY_TOP = "Top"
    private const val UNKNOWN_YEAR = "unknown"

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
    fun forAuthor(author: Author, year: Int): Uri.Builder = forCategory(CATEGORY_AUTHORS)
        .appendPath(author.nickname)
        .appendPath(if (year != 0) year.toString() else UNKNOWN_YEAR)
        .appendQueryParameter(PARAM_AUTHOR_ID, author.id.toString())

    @JvmStatic
    fun findAuthor(uri: Uri, path: List<String>) = path.getOrNull(POS_AUTHOR_NICK)?.let { nick ->
        uri.getQueryParameter(PARAM_AUTHOR_ID)?.toIntOrNull()?.let { id ->
            Author(id, nick, "" /*fake*/)
        }
    }

    @JvmStatic
    fun findAuthorYear(uri: Uri, path: List<String>) =
        path.getOrNull(POS_AUTHOR_YEAR)?.let { year ->
            if (UNKNOWN_YEAR == year) {
                0
            } else {
                year.toIntOrNull()
            }
        }

    // Parties
    @JvmStatic
    fun forPartiesYear(year: Int): Uri.Builder = forCategory(CATEGORY_PARTIES)
        .appendPath(year.toString())

    @JvmStatic
    fun findPartiesYear(uri: Uri, path: List<String>) =
        path.getOrNull(POS_PARTY_YEAR)?.toIntOrNull()

    @JvmStatic
    fun forParty(party: Party): Uri.Builder = forPartiesYear(party.year)
        .appendPath(party.name)
        .appendQueryParameter(PARAM_PARTY_ID, party.id.toString())

    @JvmStatic
    fun findParty(uri: Uri, path: List<String>) = path.getOrNull(POS_PARTY_NAME)?.let { name ->
        path.getOrNull(POS_PARTY_YEAR)?.toIntOrNull()?.let { year ->
            uri.getQueryParameter(PARAM_PARTY_ID)?.toIntOrNull()?.let { id ->
                Party(id, name, year)
            }
        }
    }

    @JvmStatic
    fun forPartyCompo(party: Party, compo: String): Uri.Builder = forParty(party)
        .appendPath(compo)

    @JvmStatic
    fun findPartyCompo(uri: Uri, path: List<String>) = path.getOrNull(POS_PARTY_COMPO)

    // Tracks
    @JvmStatic
    fun forTrack(parent: Uri.Builder, track: Track): Uri.Builder = parent
        .appendPath(track.filename)
        .appendQueryParameter(PARAM_TRACK_ID, track.id.toString())

    @JvmStatic
    fun findAuthorTrack(uri: Uri, path: List<String>) =
        path.getOrNull(POS_AUTHOR_TRACK)?.let { filename ->
            uri.getQueryParameter(PARAM_TRACK_ID)?.toIntOrNull()?.let { id ->
                Track(id, filename)
            }
        }

    @JvmStatic
    fun findPartyTrack(uri: Uri, path: List<String>) =
        path.getOrNull(POS_PARTY_TRACK)?.let { filename ->
            uri.getQueryParameter(PARAM_TRACK_ID)?.toIntOrNull()?.let { id ->
                Track(id, filename)
            }
        }

    @JvmStatic
    fun findTopTrack(uri: Uri, path: List<String>) =
        path.getOrNull(POS_TOP_TRACK)?.let { filename ->
            uri.getQueryParameter(PARAM_TRACK_ID)?.toIntOrNull()?.let { id ->
                Track(id, filename)
            }
        }
}
