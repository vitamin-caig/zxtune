package app.zxtune.core.jni

import app.zxtune.core.Module
import app.zxtune.core.PropertiesContainer
import app.zxtune.core.ResolvingException
import app.zxtune.utils.ProgressCallback
import java.nio.ByteBuffer
import java.util.concurrent.atomic.AtomicReference

/**
 * TODO: make interface and provide by IoC singleton
 */
interface Api {
    /**
     * Loads single module
     *
     * @param data    binary data
     * @param subPath nested data identifier (e.g. file's path in archive)
     * @return instance of Module
     * @throws ResolvingException if file is not found by identifier
     */
    @Throws(ResolvingException::class)
    fun loadModule(data: ByteBuffer, subPath: String): Module

    /**
     * Detects all the modules in data
     *
     * @param data     binary data
     * @param callback called on every found module
     * @param progress optional progress (total=100) tracker
     */
    fun detectModules(data: ByteBuffer, callback: DetectCallback, progress: ProgressCallback?)

    /**
     * Gets all the registered plugins in core
     *
     * @param visitor
     */
    fun enumeratePlugins(visitor: Plugins.Visitor)

    /**
     * Gets library options instance
     *
     * @return
     */
    fun getOptions(): PropertiesContainer

    private object Holder {
        val INSTANCE = AtomicReference<Api?>()
    }

    companion object {
        @JvmStatic
        fun instance(): Api {
            if (Holder.INSTANCE.get() == null) {
                DelayLoadApi(Holder.INSTANCE).initialize {}
            }
            return Holder.INSTANCE.get()!!
        }

        @JvmStatic
        fun load(cb: Runnable) {
            DelayLoadApi(Holder.INSTANCE).initialize(cb)
        }
    }
}
