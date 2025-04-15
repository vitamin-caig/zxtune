/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs.vgmrips

import android.net.Uri
import androidx.annotation.VisibleForTesting
import app.zxtune.utils.ifNotNulls

/**
 * Paths:
 *
 *
 * 1) vgmrips:/
 * 2) vgmrips:/Company
 * 3) vgmrips:/Company/${company_name}?group=${company_id}
 * 4) vgmrips:/Composer
 * 5) vgmrips:/Composer/${composer_name}?group=${composer_id}
 * 6) vgmrips:/Chip
 * 7) vgmrips:/Chip/${chip_name}?group=${chip_id}
 * 8) vgmrips:/System
 * 9) vgmrips:/System/${system_name}?group=${system_id}
 * 10) vgmrips:/Random
 *
 *
 * X) ${url}&pack=${pack.id}&location=${pack.archive}
 *
 * Track identifiers (for compatibility):
 * .../${pack.title}/${track.title}?pack=${pack.id}&track=${track.location}
 *
 * Images:
 * vgmrips:/Image?location=${path}
 *
 * Check query params at the end - UOE if uri is not hierarchical (root)
 */
object Identifier {
    private const val SCHEME = "vgmrips"
    private const val POS_CATEGORY = 0
    private const val POS_GROUP_NAME = 1
    private const val POS_PACK_NAME = 2
    private const val PARAM_GROUP = "group"
    private const val PARAM_PACK = "pack"
    private const val PARAM_TRACK = "track"
    private const val PARAM_LOCATION = "location"
    const val CATEGORY_COMPANY = "Company"
    const val CATEGORY_COMPOSER = "Composer"
    const val CATEGORY_CHIP = "Chip"
    const val CATEGORY_SYSTEM = "System"
    const val CATEGORY_RANDOM = "Random"
    const val CATEGORY_IMAGE = "Image"

    // Root
    fun forRoot(): Uri.Builder = Uri.Builder().scheme(SCHEME)

    fun isFromRoot(uri: Uri) = SCHEME == uri.scheme

    // Categories
    fun forCategory(category: String): Uri.Builder = forRoot().appendPath(category)

    fun findCategory(path: List<String>) = path.getOrNull(POS_CATEGORY)

    fun forGroup(parent: Uri.Builder, obj: Group) = parent.apply {
        appendPath(obj.title)
        appendQueryParameter(PARAM_GROUP, obj.id.value)
    }

    fun findGroup(uri: Uri, path: List<String>) = path.getOrNull(POS_GROUP_NAME)?.let { name ->
        uri.getQueryParameter(PARAM_GROUP)?.let { id ->
            Group(Group.Id(id), name, 0 /*fake*/)
        }
    }

    fun forPack(parent: Uri.Builder, pack: Pack) = parent.apply {
        appendPath(pack.title)
        appendQueryParameter(PARAM_PACK, pack.id.value)
        appendQueryParameter(PARAM_LOCATION, pack.archive.value)
    }

    @VisibleForTesting
    fun findPackId(uri: Uri) =
        uri.takeIf { it.isHierarchical }?.getQueryParameter(PARAM_PACK)?.let {
            Pack.Id(it)
        }

    fun findPack(uri: Uri, path: List<String>) = path.getOrNull(POS_PACK_NAME)?.let { name ->
        ifNotNulls(findPackId(uri), uri.getQueryParameter(PARAM_LOCATION)) { id, archive ->
            Pack(id, name, FilePath(archive))
        }
    }

    fun forRandomTrack(pack: Pack.Id, track: FilePath): Uri =
        forCategory(CATEGORY_RANDOM).appendQueryParameter(PARAM_PACK, pack.value)
            .appendQueryParameter(PARAM_TRACK, track.value).build()

    fun findTrack(uri: Uri) = findPackId(uri)?.let { id ->
        uri.getQueryParameter(PARAM_TRACK)?.let { track ->
            Pair(id, FilePath(track))
        }
    }

    fun forImage(path: FilePath): Uri =
        forCategory(CATEGORY_IMAGE).appendQueryParameter(PARAM_LOCATION, path.value).build()

    fun findImage(uri: Uri) = uri.getQueryParameter(PARAM_LOCATION)?.let {
        require(uri.lastPathSegment == CATEGORY_IMAGE)
        FilePath(it)
    }
}
