/**
 * @file
 * @brief Vfs file object interface
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs

interface VfsFile : VfsObject {
    /**
     * @return File size in implementation-specific units
     */
    val size: String
}
