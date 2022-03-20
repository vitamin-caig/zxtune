/**
 * @file
 * @brief Track description POJO
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs.amp

data class Track(
    // unique id
    val id: Int,
    // display name
    val filename: String,
    // in kb
    val size: Int
)
