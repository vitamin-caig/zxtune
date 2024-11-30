package app.zxtune.core.jni

import androidx.annotation.Keep
import app.zxtune.TimeStamp
import app.zxtune.core.Player
import java.lang.annotation.Native

@Keep
internal class JniPlayer(handleVal: Int) : Player {
    @field:Native
    private val handle = handleVal
    private val handleRes = JniGC.register(this) {
        // to avoid capturing of 'this'
        close(handleVal)
    }

    override fun release() = handleRes.release()

    external override fun render(result: ShortArray): Boolean

    external override fun analyze(levels: ByteArray): Int

    override var position: TimeStamp
        get() = TimeStamp.fromMilliseconds(positionMs.toLong())
        set(pos) {
            positionMs = pos.toMilliseconds().toInt()
        }

    // Should be public!!! Or backing field is accessed instead of member
    // On the JVM: Access to private properties with default getters and setters is optimized to
    // avoid function call overhead.
    // https://kotlinlang.org/docs/properties.html#backing-properties
    var positionMs: Int
        external get
        external set

    override val performance: Int
        external get

    override val progress: Int
        external get

    external override fun getProperty(name: String, defVal: Long): Long

    external override fun getProperty(name: String, defVal: String): String

    override fun getProperty(name: String, defVal: ByteArray?) = defVal

    external override fun setProperty(name: String, value: Long)

    external override fun setProperty(name: String, value: String)

    companion object {
        @JvmStatic
        private external fun close(handle: Int)
    }
}
