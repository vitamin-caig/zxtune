package app.zxtune.ui.utils

import android.animation.Animator
import android.view.ViewPropertyAnimator
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.FlowCollector
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.flow.transformLatest
import kotlinx.coroutines.launch
import kotlinx.coroutines.suspendCancellableCoroutine
import kotlin.coroutines.resume
import kotlin.experimental.ExperimentalTypeInference
import kotlin.properties.ReadOnlyProperty
import kotlin.reflect.KProperty

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
        private var canceled = false
        override fun onAnimationStart(animation: Animator) = Unit
        override fun onAnimationEnd(animation: Animator) {
            if (!canceled && cont.isActive) cont.resume(this@await)
        }

        override fun onAnimationCancel(animation: Animator) {
            canceled = true
        }

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
