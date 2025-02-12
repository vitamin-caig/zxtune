package app.zxtune.fs.api

import android.net.Uri
import app.zxtune.BuildConfig
import org.junit.Assert.assertEquals
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class ProxyTest {
    @Test
    fun `test proxy urls`() {
        assertEquals(
            "${BuildConfig.PROXY_ROOT}/somehost",
            Proxy.uriFor(Uri.parse("https://somehost")).toString()
        )
        assertEquals(
            "${BuildConfig.PROXY_ROOT}/somehost/",
            Proxy.uriFor(Uri.parse("https://somehost/")).toString()
        )
        assertEquals(
            "${BuildConfig.PROXY_ROOT}/somehost/some/path", Proxy.uriFor(
                Uri.parse("https://somehost/some/path")
            ).toString()
        )
        assertEquals(
            "${BuildConfig.PROXY_ROOT}/somehost/some/path?a=b&a=c&b=d", Proxy.uriFor(
                Uri.parse("https://somehost/some/path?a=b&a=c&b=d")
            ).toString()
        )
        assertEquals(
            "${BuildConfig.PROXY_ROOT}/somehost/encoded%20path?encoded%20query=%23", Proxy.uriFor(
                Uri.parse("https://somehost/encoded%20path?encoded%20query=%23")
            ).toString()
        )
    }
}