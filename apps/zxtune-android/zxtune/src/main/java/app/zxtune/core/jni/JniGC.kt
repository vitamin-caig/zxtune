package app.zxtune.core.jni

import app.zxtune.Releaseable
import kotlinx.coroutines.DelicateCoroutinesApi
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.GlobalScope
import kotlinx.coroutines.launch
import java.lang.ref.ReferenceQueue
import java.lang.ref.WeakReference
import java.util.concurrent.atomic.AtomicReference

internal object JniGC {
    private val queue = ReferenceQueue<Any>()
    private val refs = mutableSetOf<HandleReference>()
    private var destroyedObjects = 0

    init {
        @OptIn(DelicateCoroutinesApi::class) GlobalScope.launch(Dispatchers.IO) {
            while (true) {
                cleanup()
            }
        }
    }

    private fun cleanup() = runCatching {
        (queue.remove() as HandleReference).run {
            refs.remove(this)
            if (destroy()) {
                ++destroyedObjects
            }
        }
    }

    fun register(owner: Any, res: Releaseable) = HandleReference(queue, owner, res).apply {
        refs.add(this)
    }.let {
        Releaseable { it.destroy() }
    }

    private class HandleReference(
        queue: ReferenceQueue<Any>,
        owner: Any,
        res: Releaseable,
    ) : WeakReference<Any>(owner, queue) {

        private val handle = AtomicReference(res)

        fun destroy() = handle.getAndSet(null)?.run {
            release()
            true
        } ?: false
    }
}
