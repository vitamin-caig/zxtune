package app.zxtune.core.jni

import androidx.annotation.VisibleForTesting
import app.zxtune.Logger
import app.zxtune.core.Module
import app.zxtune.core.PropertiesContainer
import app.zxtune.core.ResolvingException
import app.zxtune.utils.NativeLoader
import app.zxtune.utils.ProgressCallback
import java.nio.ByteBuffer

private val LOG = Logger(JniApi::class.java.name)

internal class JniApi : Api {
    init {
        LOG.d { "Initialize" }
        NativeLoader.loadLibrary(LIBRARY_NAME) { this.forcedInit() }
    }

    private val loggingOptions = LoggingOptionsAdapter(JniOptions)

    private external fun forcedInit()

    @Throws(ResolvingException::class)
    external override fun loadModule(data: ByteBuffer, subPath: String): Module

    @Throws(ResolvingException::class)
    external override fun loadModuleData(data: ByteBuffer, subPath: String, callback: DataCallback)

    external override fun detectModules(
        data: ByteBuffer, callback: DetectCallback, progress: ProgressCallback?
    )

    external override fun enumeratePlugins(visitor: Plugins.Visitor)

    override fun getOptions(): PropertiesContainer = loggingOptions

    companion object {
        private const val LIBRARY_NAME = "zxtune"

        var create: () -> Api = ::JniApi
            @VisibleForTesting set

        fun loadLibraryForTest() {
            System.loadLibrary(LIBRARY_NAME)
        }
    }
}

private class LoggingOptionsAdapter(private val delegate: PropertiesContainer) :
    PropertiesContainer by delegate {
    override fun setProperty(name: String, value: Long) {
        LOG.d { "setProperty($name, $value)" }
        delegate.setProperty(name, value)
    }

    override fun setProperty(name: String, value: String) {
        LOG.d { "setProperty($name, $value)" }
        delegate.setProperty(name, value)
    }
}
