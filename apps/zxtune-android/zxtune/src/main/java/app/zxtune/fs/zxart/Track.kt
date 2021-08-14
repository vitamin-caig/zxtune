/**
 * @file
 * @brief Track description POJO
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs.zxart

data class Track(
    val id: Int,
    val filename: String,
    val title: String,
    val votes: String,
    val duration: String,
    val year: Int,
    val compo: String?,
    val partyplace: Int
) {
    // TODO: use default parameters
    constructor(id: Int, filename: String) : this(id, filename, "", "", "", 0, "", 0)
}
