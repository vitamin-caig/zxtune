package app.zxtune

import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.launch
import kotlinx.coroutines.test.TestScope
import kotlinx.coroutines.test.UnconfinedTestDispatcher
import kotlinx.coroutines.test.advanceUntilIdle
import org.mockito.MockedConstruction
import org.mockito.Mockito
import org.mockito.kotlin.KStubbing
import org.mockito.kotlin.mock
import org.mockito.kotlin.stub
import org.robolectric.Robolectric

object TestUtils {
    inline fun <reified T : Any> construct(crossinline stubbing: KStubbing<T>.(T) -> Unit): MockedConstruction<T> =
        Mockito.mockConstruction(T::class.java) { mock, _ ->
            mock.stub(stubbing)
        }

    inline val <reified T> MockedConstruction<T>.constructedInstance: T
        get() = constructed().run {
            first().also {
                require(size == 1)
            }
        }

    @OptIn(ExperimentalCoroutinesApi::class)
    fun TestScope.flushEvents() {
        advanceUntilIdle()
        Robolectric.flushForegroundThreadScheduler()
    }

    @OptIn(ExperimentalCoroutinesApi::class)
    fun <T> TestScope.mockCollectorOf(flow: Flow<T>) = mock<(T) -> Unit>().also { cb ->
        backgroundScope.launch(UnconfinedTestDispatcher(testScheduler)) {
            flow.collect {
                cb(it)
            }
        }
    }
}

// TODO: migrate to JUnit after upgrade
internal inline fun <reified T> assertThrows(cmd: () -> Unit): T = try {
    cmd()
    fail("${T::class.java.name} was not thrown")
} catch (e: Throwable) {
    when (e) {
        is AssertionError -> throw e
        is T -> e
        else -> fail("Unexpected exception ${e::class.java.name}: ${e.message}")
    }
}

fun fail(msg: String): Nothing = throw AssertionError(msg)
