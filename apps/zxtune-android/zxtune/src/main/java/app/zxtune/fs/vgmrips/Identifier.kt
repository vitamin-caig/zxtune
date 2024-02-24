/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs.vgmrips

import android.net.Uri
import app.zxtune.TimeStamp
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
 * 10) vgmrips://Random
 *
 *
 * X) ${url}&pack=${pack_id}
 *
 * vgmrips:/Images/${entity_name}?pack=${pack_id}&location=${location}
 */
object Identifier {
    private const val SCHEME = "vgmrips"
    private const val POS_CATEGORY = 0
    private const val POS_GROUP_NAME = 1
    private const val POS_PACK_NAME = 2
    private const val POS_RANDOM_PACK_NAME = 1
    private const val POS_IMAGE_ENTITY_NAME = 1
    private const val PARAM_GROUP = "group"
    private const val PARAM_PACK = "pack"
    private const val PARAM_TRACK = "track"
    private const val PARAM_LOCATION = "location"
    const val CATEGORY_COMPANY = "Company"
    const val CATEGORY_COMPOSER = "Composer"
    const val CATEGORY_CHIP = "Chip"
    const val CATEGORY_SYSTEM = "System"
    const val CATEGORY_RANDOM = "Random"
    const val CATEGORY_IMAGES = "Images"

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

    @JvmStatic
    fun forGroup(parent: Uri.Builder, obj: Group): Uri.Builder =
        parent.appendPath(obj.title).appendQueryParameter(PARAM_GROUP, obj.id)

    @JvmStatic
    fun findGroup(uri: Uri, path: List<String>) = path.getOrNull(POS_GROUP_NAME)?.let { name ->
        uri.getQueryParameter(PARAM_GROUP)?.let { id ->
            Group(id, name, 0 /*fake*/)
        }
    }

    @JvmStatic
    fun forPack(parent: Uri.Builder, pack: Pack): Uri.Builder =
        parent.appendPath(pack.title).appendQueryParameter(PARAM_PACK, pack.id)

    @JvmStatic
    fun findPack(uri: Uri, path: List<String>) = path.getOrNull(POS_PACK_NAME)?.let { name ->
        uri.getQueryParameter(PARAM_PACK)?.let { id ->
            Pack(id, name)
        }
    }

    @JvmStatic
    fun findRandomPack(uri: Uri, path: List<String>) =
        path.getOrNull(POS_RANDOM_PACK_NAME)?.let { name ->
            uri.getQueryParameter(PARAM_PACK)?.let { id ->
                Pack(id, name)
            }
        }

    @JvmStatic
    fun forTrack(parent: Uri.Builder, track: Track): Uri.Builder =
        parent.appendPath(track.title).appendQueryParameter(PARAM_TRACK, track.location)

    @JvmStatic
    fun findTrack(uri: Uri, path: List<String>) =
        path.takeIf { it.size >= 2 }?.last()?.let { name ->
            uri.getQueryParameter(PARAM_TRACK)?.let { id ->
                Track(0 /*fake*/, name, TimeStamp.EMPTY /*fake*/, id)
            }
        }

    @JvmStatic
    fun forImageOf(pack: Pack): Uri = forCategory(CATEGORY_IMAGES).appendPath(pack.title)
        .appendQueryParameter(PARAM_PACK, pack.id)
        .also { uri ->
            pack.imageLocation.takeUnless { it.isNullOrEmpty() }?.let {
                uri.appendQueryParameter(PARAM_LOCATION, it)
            }
        }
        .build()

    @JvmStatic
    fun findPackForImage(uri: Uri, path: List<String>) = ifNotNulls(
        path.getOrNull(POS_IMAGE_ENTITY_NAME),
        uri.getQueryParameter(PARAM_PACK),
    ) { name, id ->
        Pack(id, name, imageLocation = uri.getQueryParameter(PARAM_LOCATION))
    }
}
