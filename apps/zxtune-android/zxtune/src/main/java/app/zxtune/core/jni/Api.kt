package app.zxtune.core.jni

import androidx.annotation.VisibleForTesting
import app.zxtune.Logger
import app.zxtune.core.Module
import app.zxtune.core.PropertiesContainer
import app.zxtune.core.ResolvingException
import app.zxtune.utils.ProgressCallback
import kotlinx.coroutines.CoroutineStart
import kotlinx.coroutines.Deferred
import kotlinx.coroutines.DelicateCoroutinesApi
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.GlobalScope
import kotlinx.coroutines.async
import kotlinx.coroutines.runBlocking
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

    @Throws(ResolvingException::class)
    fun loadModuleData(data: ByteBuffer, subPath: String, callback: DataCallback)

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

    companion object {
        private val LOG = Logger(Api::class.java.name)

        // TODO: simplify after instance() removal
        @OptIn(DelicateCoroutinesApi::class)
        private fun createLoader() = GlobalScope.async(Dispatchers.Default, CoroutineStart.LAZY) {
            runCatching {
                JniApi.create()
            }.getOrElse {
                ErrorApi(it)
            }
        }

        @VisibleForTesting
        val loaderCache = AtomicReference<Deferred<Api>>()
        private val loader: Deferred<Api>
            get() = loaderCache.run {
                get() ?: run {
                    compareAndSet(null, createLoader())
                    get()
                }
            }

        @Deprecated("Use new async api")
        @JvmStatic
        fun instance() = runBlocking {
            if (!loader.isCompleted) {
                LOG.d { "Waiting for native library loading" }
                Thread.dumpStack()
                val startWaiting = System.currentTimeMillis()
                loader.invokeOnCompletion {
                    LOG.d { "Finished waiting (${System.currentTimeMillis() - startWaiting}ms)" }
                }
            }
            loader.await()
        }

        fun load() = loader
    }
}
