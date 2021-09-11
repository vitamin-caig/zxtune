/**
 * @file
 * @brief Track description POJO
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs.modland

import android.net.Uri
import java.util.zip.CRC32

data class Track constructor(
    // unique id (hash of decoded path)
    val id: Long,
    // encoded path starting at /pub/modules/...
    // encoding is not fully compatible with Uri.encode, so keep it as is
    val path: String,
    // in bytes
    val size: Int,
    // decoded last segment of id
    val filename: String,
) {
    // TODO: use default arguments
    constructor(id: Long, path: String, size: Int) : this(
        id,
        path,
        size,
        Uri.decode(path.substringAfterLast('/'))
    )

    constructor(path: String, size: Int) : this(crc32(Uri.decode(path)), path, size) {}
}

private fun crc32(str: String) = with(CRC32()) {
    update(str.toByteArray())
    value
}
