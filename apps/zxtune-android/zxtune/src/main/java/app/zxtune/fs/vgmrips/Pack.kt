package app.zxtune.fs.vgmrips

// Backward links are not need
data class Pack(
    val id: String,
    val title: String,
    var songs: Int = 0,
    var score: Int = 0,
    var ratings: Int = 0,
    //unescaped path after /packs/images/large/ and before .png
    var imageLocation: String? = null,
)
