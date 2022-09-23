package app.zxtune.analytics.internal

import android.net.Uri
import org.junit.Assert.assertEquals
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class UrlsBuilderTest {

    @Test
    fun `test no parameters`() = with(UrlsBuilder("some/type", 123)) {
        assertEquals("some/type?ts=123", result)
    }

    @Test
    fun `test all`() = with(UrlsBuilder("/full/complex/type", 4567)) {
        addParam("nullString", null)
        addParam("emptyString", "")
        addParam("string", "with spaces")
        addParam("defaultLong", -1)
        addParam("long", 123_456_789)
        addUri(Uri.EMPTY)
        addUri(Uri.parse("file://should/be/stripped"))
        addUri(Uri.parse("http://example.com?query=value"))
        addUri(Uri.parse("content://some.package.name/should/be/stripped?this=too#andthis"))

        assertEquals(
            "/full/complex/type?ts=4567" +
                    "&string=with%20spaces&long=123456789&" +
                    "uri=root&" +
                    "uri=file&" +
                    "uri=http%3A%2F%2Fexample.com%3Fquery%3Dvalue&" +
                    "uri=content%3A%2F%2Fsome.package.name", result
        )
    }

    @Test
    fun `test escaped url`() = with(UrlsBuilder("type", 89)) {
        addUri(Uri.parse("http://example.com/escaped%20url"))

        assertEquals("type?ts=89&uri=http%3A%2F%2Fexample.com%2Fescaped%2520url", result)
    }
}
