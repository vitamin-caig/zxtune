package app.zxtune.net

import androidx.arch.core.executor.testing.InstantTaskExecutorRule
import androidx.lifecycle.Observer
import app.zxtune.Releaseable
import app.zxtune.TestUtils.flushEvents
import kotlinx.coroutines.flow.onSubscription
import kotlinx.coroutines.test.StandardTestDispatcher
import kotlinx.coroutines.test.TestDispatcher
import kotlinx.coroutines.test.runTest
import org.junit.Assert.assertEquals
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.any
import org.mockito.kotlin.doAnswer
import org.mockito.kotlin.inOrder
import org.mockito.kotlin.mock
import org.mockito.kotlin.times
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class NetworkManagerTest {

    val dispatcher = StandardTestDispatcher()

    @Test
    fun `ConnectionState workflow`() = runTest(dispatcher) {
        var networkState = false
        lateinit var callback: NetworkStateCallback
        val handle = mock<Releaseable>()
        val source = mock<NetworkStateSource> {
            on { isNetworkAvailable() } doAnswer {
                networkState
            }
            on { monitorChanges(any()) } doAnswer {
                callback = it.getArgument(0)
                handle
            }
        }
        val state = NetworkManager(source, dispatcher).networkAvailable
        with(state) {
            // initial state
            networkState = false
            assertEquals(false, value)
            networkState = true
            assertEquals(false, value)
            // notification
            flushEvents()
            callback(true)
            flushEvents()
            assertEquals(true, value)
            callback(false)
            flushEvents()
            assertEquals(false, value)
        }

        inOrder(source, handle) {
            // initial state
            verify(source).isNetworkAvailable()
            // notification
            verify(source).monitorChanges(callback)
        }
    }
}
