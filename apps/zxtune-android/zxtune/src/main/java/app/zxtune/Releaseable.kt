/**
 *
 * @file
 *
 * @brief Releaseable interface declaration
 *
 * @author vitamin.caig@gmail.com
 */
package app.zxtune

/**
 * Base interface for managed resources supporting release call
 * Analogue of Closeable but do not throw any specific exceptions
 */
fun interface Releaseable {
    fun release()
}

object ReleaseableStub : Releaseable {
    override fun release() {}
}

inline fun <R> Releaseable.use(block: () -> R): R =
    try {
        block()
    } finally {
        release()
    }
