package app.zxtune.fs.provider

import app.zxtune.fs.Vfs
import app.zxtune.fs.VfsArchive
import app.zxtune.fs.VfsDir
import app.zxtune.fs.VfsFile
import app.zxtune.fs.VfsObject
import app.zxtune.fs.feed
import app.zxtune.fs.icon
import app.zxtune.fs.iconUri

internal class SchemaSourceImplementation : SchemaSource {
    override fun resolved(data: VfsObject) = when (data) {
        is VfsDir -> Schema.Listing.Dir(
            uri = data.uri,
            name = data.name,
            description = data.description,
            icon = null,
            hasFeed = data.feed != null,
        )

        is VfsFile -> Schema.Listing.File(
            uri = data.uri,
            name = data.name,
            description = data.description,
            icon = null,
            details = data.size,
            type = Schema.Listing.File.Type.UNKNOWN,
        )

        else -> null
    }

    override fun parents(data: List<VfsObject>) = data.map {
        Schema.Parents.Object(
            uri = it.uri,
            name = it.name,
            icon = it.icon,
        )
    }

    override fun directories(data: List<VfsDir>) = data.map {
        Schema.Listing.Dir(
            uri = it.uri,
            name = it.name,
            description = it.description,
            icon = it.iconUri,
            hasFeed = it.feed != null,
        )
    }

    override fun files(data: List<VfsFile>): List<Schema.Listing.File> {
        /*
         TODO:
         - faster getModulesCount (everything is track if archive listed)
         - batched query of coverart uris
         - parallel querying
         */
        val uris = data.map { it.uri }
        val tracks = VfsArchive.getModulesCount(uris.toTypedArray())
        return data.mapIndexed { idx, file ->
            val uri = uris[idx]
            val type = getFileType(tracks[idx], file)
            Schema.Listing.File(
                uri = uri,
                name = file.name,
                description = file.description,
                icon = file.iconUri ?: if (type == Schema.Listing.File.Type.TRACK) {
                    uri // fallback to coverart
                } else null,
                details = file.size,
                type = type,
            )
        }
    }

    companion object {
        private fun getFileType(tracksCount: Int?, file: VfsFile) = when {
            tracksCount == null -> if (false == Vfs.getCache(file)?.isFile) Schema.Listing.File.Type.REMOTE else Schema.Listing.File.Type.UNKNOWN
            tracksCount > 1 -> Schema.Listing.File.Type.ARCHIVE
            tracksCount == 1 -> Schema.Listing.File.Type.TRACK
            else -> Schema.Listing.File.Type.UNSUPPORTED
        }
    }
}
