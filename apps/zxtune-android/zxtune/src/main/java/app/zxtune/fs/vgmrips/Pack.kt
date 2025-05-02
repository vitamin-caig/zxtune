package app.zxtune.fs.vgmrips

// Backward links are not need
data class Pack(
    val id: Id,
    val title: String,
    // unescaped path after /files/
    val archive: FilePath,
    // unescaped path after /packs/images/large/
    val image: FilePath? = null,
    val songs: Int = 0,
    val size: String = "",
) {
    init {
        require(id.value.isNotEmpty())
        require(archive.value.isNotEmpty())
        image?.run {
            require(value.isNotEmpty())
        }
    }

    data class Id(val value: String)
}

data class FilePath(val value: String) {
    init {
        require(value.isNotEmpty() && '/' != value.first())
    }

    val displayName
        get() = value.substringAfterLast('/').substringBeforeLast('.')
}
