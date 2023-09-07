package app.zxtune

import app.zxtune.core.Identifier
import org.junit.Assert.assertEquals
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class UtilTest {
    @Test
    fun `formatTrackTitle with strings`() {
        assertEquals("fallback", Util.formatTrackTitle("", "", "fallback"))
        assertEquals("left", Util.formatTrackTitle("left", "", "fallback"))
        assertEquals("right", Util.formatTrackTitle("", "right", "fallback"))
        assertEquals("left - right", Util.formatTrackTitle("left", "right", "fallback"))
    }

    @Test
    fun `formatTrackTitle with identifier`() {
        assertEquals(
            "path > subpath", Util.formatTrackTitle(
                "", Identifier.parse("schema://host/path#subpath")
            )
        )
        assertEquals(
            "just title", Util.formatTrackTitle(
                "just title", Identifier.parse("schema://host/path")
            )
        )
        assertEquals(
            "title with track (#123)", Util.formatTrackTitle(
                "title with track", Identifier.parse("schema://host/path#subpath/%23123")
            )
        )
    }

    @Test
    fun formatSize() {
        assertEquals("0", Util.formatSize(0))
        assertEquals("1023", Util.formatSize(1023))
        assertEquals("1.0K", Util.formatSize(1030))
        assertEquals("1.9M", Util.formatSize(2000000))
    }
}
