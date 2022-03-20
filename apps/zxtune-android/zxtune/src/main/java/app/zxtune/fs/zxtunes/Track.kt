/**
 * @file
 * @brief Track description POJO
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs.zxtunes

data class Track(
    val id: Int,
    val filename: String,
    val title: String = "",
    val duration: Int? = null,
    val date: Int? = null
)
