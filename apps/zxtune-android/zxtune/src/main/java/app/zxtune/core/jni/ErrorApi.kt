package app.zxtune.core.jni

import app.zxtune.core.PropertiesContainer
import app.zxtune.utils.ProgressCallback
import java.nio.ByteBuffer

internal class ErrorApi(error: Throwable) : Api {

    private val error: RuntimeException = RuntimeException(error)

    override fun loadModule(data: ByteBuffer, subPath: String) = throw error

    override fun loadModuleData(data: ByteBuffer, subPath: String, callback: DataCallback) =
        throw error

    override fun detectModules(
        data: ByteBuffer, callback: DetectCallback, progress: ProgressCallback?
    ) = throw error

    override fun enumeratePlugins(visitor: Plugins.Visitor) = throw error

    override fun getOptions(): PropertiesContainer = throw error
}
