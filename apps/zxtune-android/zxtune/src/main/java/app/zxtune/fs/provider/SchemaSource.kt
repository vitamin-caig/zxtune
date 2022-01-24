package app.zxtune.fs.provider

import app.zxtune.fs.VfsDir
import app.zxtune.fs.VfsFile
import app.zxtune.fs.VfsObject

// Mapping between Vfs and Schema objects for different cases
internal interface SchemaSource {
    fun resolved(data: VfsObject): Schema.Object?
    fun parents(data: List<VfsObject>): List<Schema.Parents.Object>
    fun directories(data: List<VfsDir>): List<Schema.Listing.Dir>
    fun files(data: List<VfsFile>): List<Schema.Listing.File>
}
