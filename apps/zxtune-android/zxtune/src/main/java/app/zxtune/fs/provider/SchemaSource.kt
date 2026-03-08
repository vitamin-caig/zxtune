package app.zxtune.fs.provider

import android.net.Uri
import app.zxtune.fs.Vfs
import app.zxtune.fs.VfsArchive
import app.zxtune.fs.VfsDir
import app.zxtune.fs.VfsFile
import app.zxtune.fs.VfsObject
import app.zxtune.fs.feed
import app.zxtune.fs.iconUri

internal class SchemaSource {
    fun resolved(obj: VfsObject) = when (obj) {
        // check dir first
        is VfsDir -> obj.toSchema()
        is VfsFile -> obj.toSchema(
            type = getFileType(
                VfsArchive.getModulesCount(arrayOf(obj.uri)).first(), obj
            )
        )

        else -> throw IllegalStateException("Invalid type of object")
    }

    fun directories(data: List<VfsDir>) = data.map { it.toSchema() }

    fun files(data: List<VfsFile>): List<Schema.Content.File> {/*
         TODO:
         - faster getModulesCount (everything is track if archive listed)
         - batched query of coverart uris
         - parallel querying
         */
        val uris = Array(data.size) {
            data[it].uri
        }
        val tracks = VfsArchive.getModulesCount(uris)
        return data.mapIndexed { idx, file ->
            val uri = uris[idx]
            val type = getFileType(tracks[idx], file)
            val icon = file.iconUri ?: if (type == Schema.Content.File.Type.TRACK) {
                uri // fallback to coverart
            } else null
            file.toSchema(icon, type)
        }
    }

    companion object {
        private fun getFileType(tracksCount: Int?, file: VfsFile) = when {
            tracksCount == null -> if (false == Vfs.getCache(file)?.isFile) Schema.Content.File.Type.REMOTE else Schema.Content.File.Type.UNKNOWN
            tracksCount > 1 -> Schema.Content.File.Type.ARCHIVE
            tracksCount == 1 -> Schema.Content.File.Type.TRACK
            else -> Schema.Content.File.Type.UNSUPPORTED
        }

        private fun VfsFile.toSchema(
            icon: Uri? = null, type: Schema.Content.File.Type = Schema.Content.File.Type.UNKNOWN
        ) = Schema.Content.File(
            uri = uri,
            name = name,
            description = description,
            icon = icon,
            details = size,
            type = type,
        )

        private fun VfsDir.toSchema() = Schema.Content.Dir(
            uri = uri,
            name = name,
            description = description,
            icon = iconUri,
            hasFeed = feed != null,
        )
    }
}
