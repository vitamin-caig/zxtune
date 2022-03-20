package app.zxtune.core

import java.nio.ByteBuffer

/**
 * Interface to additional files access
 */
interface AdditionalFiles {
    /**
     * @return null if no additional files required, empty array if all the files are resolved,
     * else list of required filenames relative to current one
     */
    val additionalFiles: Array<String>?

    /**
     * @param name additional file name as returned by additionalFiles
     * @param data additional file content
     */
    fun resolveAdditionalFile(name: String, data: ByteBuffer)
}
