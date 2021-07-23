/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs.dbhelpers

import java.io.IOException

interface QueryCommand {
    val isCacheExpired: Boolean

    @Throws(IOException::class)
    fun updateCache()

    fun queryFromCache(): Boolean
}
