package app.zxtune.fs.provider

import app.zxtune.fs.Vfs
import app.zxtune.fs.VfsArchive
import app.zxtune.fs.VfsDir
import app.zxtune.fs.VfsFile
import app.zxtune.fs.VfsObject
import app.zxtune.fs.feed
import app.zxtune.fs.icon

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
            icon = it.icon,
            hasFeed = it.feed != null,
        )
    }

    override fun files(data: List<VfsFile>): List<Schema.Listing.File> {
        val uris = data.map { it.uri }
        val tracks = VfsArchive.getModulesCount(uris.toTypedArray())
        return data.mapIndexed { idx, file ->
            Schema.Listing.File(
                uri = uris[idx],
                name = file.name,
                description = file.description,
                details = file.size,
                type = getFileType(tracks[idx], file),
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
