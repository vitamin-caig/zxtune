package app.zxtune.fs.provider

import app.zxtune.fs.*

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
            tracks = null,
            isCached = null,
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
                isCached = Vfs.getCache(file)?.isFile,
                tracks = tracks[idx],
            )
        }
    }
}
