package app.zxtune.core.jni

import androidx.annotation.VisibleForTesting
import app.zxtune.Logger
import app.zxtune.core.Module
import app.zxtune.core.PropertiesContainer
import app.zxtune.utils.ProgressCallback
import java.nio.ByteBuffer
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicReference
import java.util.concurrent.locks.ReentrantLock
import kotlin.concurrent.withLock

private val LOG = Logger(DelayLoadApi::class.java.name)

internal class DelayLoadApi @VisibleForTesting constructor(
    private val ref: AtomicReference<Api>,
    private val factory: () -> Api
) : Api {

    private val lock = ReentrantLock()
    private val signal = lock.newCondition()

    constructor(ref: AtomicReference<Api>) : this(ref, ::JniApi)

    fun initialize(cb: Runnable) = if (ref.compareAndSet(null, this)) {
        loadAsync(cb)
    } else {
        cb.run()
    }

    private fun loadAsync(cb: Runnable) {
        Thread({ loadSync(cb) }, "JniLoad").start()
    }

    private fun loadSync(cb: Runnable) {
        emplace(factory)
        cb.run()
    }

    private fun waitForLoad(): Api {
        LOG.d { "Waiting for native library loading" }
        Thread.dumpStack()
        val startWaiting = System.currentTimeMillis()
        emplace {
            while (isUnset()) {
                lock.withLock {
                    if (isUnset()) {
                        signal.await(1000, TimeUnit.MILLISECONDS)
                    }
                }
            }
            ref.get()
        }
        LOG.d { "Finished waiting (${System.currentTimeMillis() - startWaiting}ms)" }
        return ref.get()
    }

    private fun isUnset() = ref.compareAndSet(this, this)

    private fun emplace(provider: () -> Api) {
        try {
            set(provider())
        } catch (e: Throwable) {
            set(ErrorApi(e))
        }
    }

    private fun set(api: Api) = lock.withLock {
        ref.set(api)
        signal.signalAll()
    }

    override fun loadModule(data: ByteBuffer, subPath: String) =
        waitForLoad().loadModule(data, subPath)

    override fun detectModules(
        data: ByteBuffer,
        callback: DetectCallback,
        progress: ProgressCallback?
    ) = waitForLoad().detectModules(data, callback, progress)

    override fun enumeratePlugins(visitor: Plugins.Visitor) =
        waitForLoad().enumeratePlugins(visitor)

    override fun getOptions() = waitForLoad().getOptions()
}

private class ErrorApi(error: Throwable) : Api {

    private val error: RuntimeException = RuntimeException(error)

    override fun loadModule(data: ByteBuffer, subPath: String): Module = throw error

    override fun detectModules(
        data: ByteBuffer,
        callback: DetectCallback,
        progress: ProgressCallback?
    ) = throw error

    override fun enumeratePlugins(visitor: Plugins.Visitor) = throw error

    override fun getOptions(): PropertiesContainer = throw error
}
