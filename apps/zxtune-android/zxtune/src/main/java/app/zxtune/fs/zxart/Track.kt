/**
 * @file
 * @brief Track description POJO
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs.zxart

data class Track(
    val id: Int,
    val filename: String,
    val title: String = "",
    val votes: String = "",
    val duration: String = "",
    val year: Int = 0,
    val compo: String = "",
    val partyplace: Int = 0
)
