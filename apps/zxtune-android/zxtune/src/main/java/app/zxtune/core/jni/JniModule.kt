package app.zxtune.core.jni

import androidx.annotation.Keep
import app.zxtune.TimeStamp
import app.zxtune.core.Module
import app.zxtune.core.Player
import java.lang.annotation.Native
import java.nio.ByteBuffer

@Keep
internal class JniModule(handleVal: Int) : Module {
    @field:Native
    private val handle = handleVal
    private val handleRes = JniGC.register(this) {
        // to avoid capturing of 'this'
        close(handleVal)
    }

    override fun release() = handleRes.release()

    override val duration
        get() = TimeStamp.fromMilliseconds(getDurationMs().toLong())

    private external fun getDurationMs(): Int

    external override fun createPlayer(samplerate: Int): Player

    external override fun getProperty(name: String, defVal: Long): Long

    external override fun getProperty(name: String, defVal: String): String

    external override fun getProperty(name: String, defVal: ByteArray?): ByteArray?

    override val additionalFiles: Array<String>?
        external get

    external override fun resolveAdditionalFile(name: String, data: ByteBuffer)

    companion object {
        @JvmStatic
        private external fun close(handle: Int)
    }
}