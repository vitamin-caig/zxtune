package app.zxtune.fs.vgmrips

// Backward links are not need
data class Pack(
    val id: String,
    val title: String,
    var songs: Int,
    var score: Int,
    var ratings: Int
) {
    // TODO: use only primary ctor with default values
    constructor(id: String, title: String) : this(id, title, 0, 0, 0)
}
