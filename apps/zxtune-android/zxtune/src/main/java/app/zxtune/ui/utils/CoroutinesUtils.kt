package app.zxtune.ui.utils

import android.animation.Animator
import android.content.ContentResolver
import android.database.ContentObserver
import android.database.Cursor
import android.net.Uri
import android.os.CancellationSignal
import android.os.OperationCanceledException
import android.view.ViewPropertyAnimator
import androidx.tracing.trace
import androidx.tracing.traceAsync
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.channels.awaitClose
import kotlinx.coroutines.channels.trySendBlocking
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.FlowCollector
import kotlinx.coroutines.flow.callbackFlow
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.flow.transformLatest
import kotlinx.coroutines.launch
import kotlinx.coroutines.suspendCancellableCoroutine
import kotlin.coroutines.cancellation.CancellationException
import kotlin.coroutines.resume
import kotlin.coroutines.resumeWithException
import kotlin.experimental.ExperimentalTypeInference
import kotlin.properties.ReadOnlyProperty
import kotlin.reflect.KProperty

fun ContentResolver.observeChanges(
    uri: Uri, notifyForDescendants: Boolean = true
) = callbackFlow {
    traceAsync("Observe $uri", uri.hashCode()) {
        val notify = {
            trySendBlocking(Unit)
            Unit
        }
        val observer = object : ContentObserver(null) {
            override fun onChange(selfChange: Boolean) = notify()
        }
        registerContentObserver(uri, notifyForDescendants, observer)
        notify()
        awaitClose {
            unregisterContentObserver(observer)
        }
    }
}

suspend fun <T> ContentResolver.query(
    uri: Uri,
    projection: Array<String>? = null,
    selection: String? = null,
    selectionArgs: Array<String>? = null,
    sortOrder: String? = null,
    convert: (Cursor) -> T,
) = suspendCancellableCoroutine { cont ->
    val signal = CancellationSignal()
    cont.invokeOnCancellation {
        signal.cancel()
    }
    try {
        val res = trace("Query $uri") {
            query(uri, projection, selection, selectionArgs, sortOrder, signal)?.use {
                convert(it)
            }
        }
        cont.resume(res)
    } catch (e: Throwable) {
        if (e is OperationCanceledException) {
            throw CancellationException(e)
        } else {
            cont.resumeWithException(e)
        }
    }
}

fun <T> flowValueOf(flow: Flow<T>, scope: CoroutineScope, initial: T) =
    object : ReadOnlyProperty<Any, T> {
        private var state: T = initial

        init {
            scope.launch {
                flow.collect {
                    state = it
                }
            }
        }

        override operator fun getValue(thisRef: Any, property: KProperty<*>) = state
    }

// https://github.com/Kotlin/kotlinx.coroutines/issues/1484
@OptIn(ExperimentalTypeInference::class, ExperimentalCoroutinesApi::class)
inline fun <reified T, R> combineTransformLatest(
    vararg flows: Flow<T>,
    @BuilderInference noinline transform: suspend FlowCollector<R>.(Array<T>) -> Unit
) = combine(*flows) { it }.transformLatest(transform)


@OptIn(ExperimentalTypeInference::class)
fun <T1, T2, R> combineTransformLatest(
    flow: Flow<T1>,
    flow2: Flow<T2>,
    @BuilderInference transform: suspend FlowCollector<R>.(T1, T2) -> Unit
) = combineTransformLatest(flow, flow2) { args ->
    @Suppress("UNCHECKED_CAST") transform(
        args[0] as T1, args[1] as T2
    )
}

suspend fun ViewPropertyAnimator.await() = suspendCancellableCoroutine { cont ->
    val listener = object : Animator.AnimatorListener {
        override fun onAnimationStart(animation: Animator) = Unit
        override fun onAnimationEnd(animation: Animator) = cont.resume(this@await)
        override fun onAnimationCancel(animation: Animator) = Unit
        override fun onAnimationRepeat(animation: Animator) = Unit
    }
    setListener(listener)
    cont.invokeOnCancellation {
        setListener(null)
        // may be not the main thread...
        runCatching {
            cancel()
        }
    }
}
