package app.zxtune.coverart

import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.net.Uri
import androidx.annotation.VisibleForTesting
import app.zxtune.Logger
import app.zxtune.MainApplication
import app.zxtune.core.Identifier
import app.zxtune.core.Module
import app.zxtune.core.ModuleAttributes
import app.zxtune.fs.VfsObject
import app.zxtune.fs.coverArtUri
import java.io.InputStream

open class CoverartService @VisibleForTesting constructor(private val db: Database) {

    private object Self : CoverartService(Database(MainApplication.getGlobalContext()))

    companion object {
        private const val MAX_PICTURE_SIZE = 800
        private val LOG = Logger(CoverartService::class.java.name)

        @JvmStatic
        fun get(): CoverartService = Self
    }

    fun cleanupFor(uri: Uri) = db.remove(uri)

    fun addEmbedded(id: Identifier, module: Module) {
        if (null == db.findEmbedded(id)) {
            module.picture?.let {
                addPicture(id, it)
            }
        }
    }

    fun addPicture(id: Identifier, data: ByteArray) =
        addBitmap(id, BitmapFactory.decodeByteArray(data, 0, data.size))

    fun addPicture(id: Identifier, data: InputStream) =
        addBitmap(id, BitmapFactory.decodeStream(data))

    private fun addBitmap(id: Identifier, bitmap: Bitmap?) = if (bitmap != null) {
        LOG.d { "Found ${bitmap.width}x${bitmap.height} raw image" }
        bitmap.use {
            bitmap.publish(MAX_PICTURE_SIZE)
        }.let { pic ->
            LOG.d { "Added ${pic.size} bytes image for $id" }
            db.addImage(id, pic)
        }
    } else {
        LOG.d { "Failed to parse raw image at $id" }
        null
    }

    private fun addReference(id: Identifier, uri: Uri) = db.addInferred(id, uri).also {
        LOG.d { "Added external picture $uri for $id" }
    }

    private fun bind(id: Identifier, uri: Uri) = db.addBoundImage(id, uri).also {
        LOG.d { "Bind external picture $uri for $id" }
    }

    fun getBlob(id: Long) = db.queryBlob(id)
    fun imageFor(uri: Uri) = db.queryImage(Identifier(uri))

    // Returns:
    // - raw blob url if exists
    // - null elsewhere
    fun coverArtUriOf(id: Identifier) = coverArtOf(id)?.toPictureUrl()

    @VisibleForTesting
    fun coverArtOf(id: Identifier) = db.findEmbedded(id)

    // Returns:
    // - null if id is known as having no albumart
    // - archive's bound image or nearest in archive if id is for archived
    // - bound image of id or any parent
    fun albumArtUriOf(id: Identifier, dataObject: VfsObject) =
        albumArtOf(id, dataObject)?.toPictureUrl()

    @VisibleForTesting
    fun albumArtOf(id: Identifier, dataObject: VfsObject): Reference.Target? {
        // Do not mix archive and storage worlds
        if (id.archiveEntryName != null) {
            return archiveArtOf(id, dataObject)
        }
        var ref = dataObject
        for (step in 0..Int.MAX_VALUE) {
            val refId = Identifier(ref.uri)
            // Just to keep invariant for tests, VfsObject.parent returns this
            assert(step > 0 || id.dataLocation == refId.fullLocation) {
                "$id (${id.dataLocation} + ${id.subPath}) and $refId"
            }
            db.queryImageReferences(refId)?.let {
                // cached or explicit none
                return it
            }
            ref.coverArtUri?.let {
                coverArtOf(Identifier(it))?.let {
                    return it
                }
                // cache coverart if it's not random
                if (!it.isRandomized()) {
                    if (id != refId) {
                        addReference(id, it)
                    }
                    bind(refId, it)
                }
                return Reference.Target(it)
            }
            ref = ref.parent ?: break
        }
        return null
    }

    @VisibleForTesting
    fun archiveArtOf(id: Identifier, dataObject: VfsObject): Reference.Target? {
        require(id.subPath.isNotEmpty())
        db.queryImageReferences(id)?.let {
            // cached or explicit none
            return it
        }
        val dataLocation = id.dataLocation
        val dataId = Identifier(dataLocation)
        // bound to archive - higher priority
        db.queryImageReferences(dataId)?.let {
            return it
        }
        val images = ImagesSet(db.listArchiveImages(dataLocation))
        if (images.isEmpty()) {
            LOG.d { "Archive $dataLocation has no images" }
            return dataObject.coverArtUri.also { artUri ->
                artUri?.let {
                    if (!it.isRandomized()) {
                        bind(dataId, it)
                    }
                } ?: db.setNoImage(dataId)
            }?.let {
                Reference.Target(it)
            }
        }

        // TODO: try to set archive image by images.selectAlbumArt("at_root.ext")
        return images.selectAlbumArt(id.subPath)?.let { path ->
            Identifier(dataLocation, path).fullLocation.also {
                LOG.d { "Select internal picture $it for archive $dataLocation" }
                addReference(id, it)
            }.let {
                Reference.Target(it)
            }
        }
    }
}

@VisibleForTesting
class Location(val path: String) {
    private val lastDelimiter = path.lastIndexOf('/')
    val dir
        get() = if (lastDelimiter != -1) path.subSequence(0, lastDelimiter) else ""
    val name
        get() = if (lastDelimiter != -1) path.subSequence(lastDelimiter + 1, path.length) else path
    var priority = -1
        private set
        get() {
            if (field == -1) {
                field = AlbumArt.pictureFilePriority(name.toString()) ?: 0
            }
            return field
        }
    val take
        get() = if (priority > 0) this else null

    // =0 if loc and this are at the same dir
    // >0 if loc is in sub directory
    // <0 if loc in in sup directory
    // null if loc and this are unrelated
    fun directDistanceTo(loc: Location) = treeDistance(dir, loc.dir)

    companion object {
        private fun treeDistance(from: CharSequence, to: CharSequence): Int? = when {
            from == to -> 0
            to.length > from.length -> directDistance(from, to)
            from.length > to.length -> directDistance(to, from)?.let { -it }
            else -> null
        }

        private fun directDistance(from: CharSequence, to: CharSequence) = when {
            from.isEmpty() -> 1 + to.count { it == '/' }
            to.startsWith(from) && to[from.length] == '/' -> to.drop(from.length)
                .count { it == '/' }

            else -> null
        }
    }
}

@VisibleForTesting
class ImagesSet(paths: List<String>) {
    private val locations = ArrayList<Location>(paths.size).apply {
        paths.takeUnless { it.isEmpty() }?.sorted()?.iterator()?.let { iterator ->
            var cursor = Location(iterator.next())
            while (iterator.hasNext()) {
                val next = Location(iterator.next())
                if (cursor.dir != next.dir) {
                    cursor.take?.let {
                        add(it)
                    }
                    cursor = next
                } else if (cursor.priority < next.priority) {
                    cursor = next
                }
            }
            cursor.take?.let {
                add(it)
            }
        }
    }

    fun isEmpty() = locations.isEmpty()

    fun selectAlbumArt(subPath: String): String? {
        val loc = Location(subPath)
        var nestedCandidate: Pair<String, Int>? = null
        var parentCandidate: Pair<String, Int>? = null
        for (candidate in locations) {
            val distance = loc.directDistanceTo(candidate)
            when {
                distance == null -> continue
                distance == 0 -> return candidate.path // this dir
                distance > 0 && (nestedCandidate == null || nestedCandidate.second > distance) -> {
                    nestedCandidate = candidate.path to distance
                }

                parentCandidate == null || parentCandidate.second < distance -> {
                    parentCandidate = candidate.path to distance
                }
            }
        }
        return nestedCandidate?.first ?: parentCandidate?.first
    }
}

private fun Uri.isRandomized() = getQueryParameter("seed") != null

private val Module.picture
    get() = runCatching {
        getProperty(ModuleAttributes.PICTURE, null)
    }.getOrNull()

private fun Bitmap.publish(maxSize: Int) = if (maxOf(width, height) > maxSize) {
    fitScaledTo(maxSize, maxSize).use { scaled ->
        scaled.compress()
    }
} else {
    compress()
}

private fun Bitmap.compress() = if (hasAlpha()) toPng() else toJpeg()

@VisibleForTesting
fun Reference.Target.toPictureUrl() = when {
    pic != null -> Query.uriFor(Query.Case.RAW, pic.toString())
    url != null -> Query.uriFor(Query.Case.IMAGE, url.toString())
    else -> null
}