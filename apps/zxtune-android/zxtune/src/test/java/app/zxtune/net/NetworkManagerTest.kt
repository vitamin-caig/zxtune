package app.zxtune.net

import android.content.Context
import androidx.arch.core.executor.testing.InstantTaskExecutorRule
import androidx.lifecycle.Observer
import org.junit.Assert.assertEquals
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.*
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config
import org.robolectric.annotation.Implementation
import org.robolectric.annotation.Implements

@RunWith(RobolectricTestRunner::class)
@Config(shadows = [ShadowNetworkStateSource::class])
class NetworkManagerTest {

    @get:Rule
    val instantTaskExecutorRule = InstantTaskExecutorRule()

    @Test
    fun `ConnectionState workflow`() {
        var networkState = false
        val source = mock<NetworkStateSource> {
            on { isNetworkAvailable() } doAnswer {
                networkState
            }
        }
        lateinit var callback: NetworkStateCallback
        ShadowNetworkStateSource.factory.stub {
            on { invoke(any(), any()) } doAnswer {
                callback = it.getArgument(1)
                source
            }
        }
        NetworkManager.initialize(mock())
        val observer = mock<Observer<Boolean>>()
        with(NetworkManager.networkAvailable) {
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

        inOrder(ShadowNetworkStateSource.factory, source, observer) {
            verify(ShadowNetworkStateSource.factory).invoke(any(), any())
            // simple call
            verify(source, times(2)).isNetworkAvailable()
            // subsciprion
            verify(source).startMonitoring()
            verify(source).isNetworkAvailable()
            verify(observer).onChanged(true)
            verify(observer).onChanged(false)
            verify(source).stopMonitoring()
            // simple call
            verify(source).isNetworkAvailable()
        }
    }
}

@Implements(NetworkStateSource.Companion::class)
class ShadowNetworkStateSource {
    @Implementation
    fun create(ctx: Context, cb: NetworkStateCallback) = factory(ctx, cb)

    companion object {
        val factory = mock<(Context, NetworkStateCallback) -> NetworkStateSource>()
    }
}
