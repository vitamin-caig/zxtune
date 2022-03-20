/**
 * @file
 * @brief Stub object base for safe inheritance
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs

// TODO: remove
abstract class StubObject : VfsObject {
    override val description
        get() = ""

    override fun getExtension(id: String): Any? = null
}
