package app.zxtune.net

import androidx.arch.core.executor.testing.InstantTaskExecutorRule
import androidx.lifecycle.Observer
import app.zxtune.Releaseable
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

    @get:Rule
    val instantTaskExecutorRule = InstantTaskExecutorRule()

    @Test
    fun `ConnectionState workflow`() {
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
        val observer = mock<Observer<Boolean>>()
        with(NetworkManager(source).networkAvailable) {
            // simple call
            networkState = false
            assertEquals(false, value)
            networkState = true
            assertEquals(true, value)
            // subscription
            networkState = false
            observeForever(observer)
            assertEquals(false, value)
            networkState = true
            assertEquals(false, value) // not synced
            callback(true)
            assertEquals(true, value)
            networkState = false
            assertEquals(true, value) // not synced
            callback(false)
            assertEquals(false, value) // not synced
            removeObserver(observer)
            // simple calls
            networkState = true
            assertEquals(true, value)
        }

        inOrder(source, handle, observer) {
            // simple call
            verify(source, times(2)).isNetworkAvailable()
            // subsciprion
            verify(source).monitorChanges(callback)
            verify(source).isNetworkAvailable()
            verify(observer).onChanged(true)
            verify(observer).onChanged(false)
            verify(handle).release()
            // simple call
            verify(source).isNetworkAvailable()
        }
    }
}
