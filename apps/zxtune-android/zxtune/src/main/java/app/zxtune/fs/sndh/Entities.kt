package app.zxtune.fs.sndh

data class Author(
    val name: String,
    val files: Int = 0,
)

data class Track(
    val id: Int,
    val title: String,
    val sizeText: String,
)
