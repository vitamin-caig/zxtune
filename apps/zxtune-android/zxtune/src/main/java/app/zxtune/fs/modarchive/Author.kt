/**
 * @file
 * @brief Author description POJO
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs.modarchive

//TODO: pictures
data class Author(val id: Int, val alias: String) {
    companion object {
        //Currently API does not provide information about track without author info.
        //The only way to get them - perform search
        @JvmField
        val UNKNOWN = Author(0, "!Unknown")
    }
}
