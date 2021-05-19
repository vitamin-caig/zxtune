package app.zxtune.core

import android.net.Uri
import org.junit.Assert.assertEquals
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

private fun verifyEquals(expected: Identifier, actual: Identifier) {
    assertEquals(expected, actual)
    assertEquals(expected.toString(), actual.toString())
}

@RunWith(RobolectricTestRunner::class)
class IdentifierTest {
    @Test
    fun `test parsing empty`() = with(Identifier.parse(null)) {

        assertEquals("", toString())
        assertEquals(Uri.EMPTY, fullLocation)
        assertEquals(Uri.EMPTY, dataLocation)
        assertEquals("", subPath)
        assertEquals("", displayFilename)

        verifyEquals(this, Identifier.parse(""))
        verifyEquals(this, Identifier(Uri.EMPTY))
        verifyEquals(this, Identifier(Uri.EMPTY, null))
        verifyEquals(this, Identifier(Uri.EMPTY, ""))
        verifyEquals(this, Identifier(Uri.EMPTY, "ignored"))
    }

    @Test
    fun `test parsing file path`() {
        val path = "dir/file"
        with(Identifier.parse(path)) {

            assertEquals(path, toString())
            assertEquals(path, fullLocation.toString())
            assertEquals(path, dataLocation.toString())
            assertEquals("", subPath)
            assertEquals("file", displayFilename)

            verifyEquals(this, Identifier.parse("${path}#"))
            verifyEquals(this, Identifier(Uri.parse(path)))
            verifyEquals(this, Identifier(Uri.parse(path), null))
            verifyEquals(this, Identifier(Uri.parse(path), ""))
        }
    }

    @Test
    fun `test parsing full uri`() {
        val path = "http://host/path?query"
        val subPath = "nested/sub path"
        val subPathEncoded = Uri.encode(subPath)

        with(Identifier.parse("${path}#${subPathEncoded}")) {

            assertEquals("${path}#${subPathEncoded}", toString())
            assertEquals("${path}#${subPathEncoded}", fullLocation.toString())
            assertEquals(path, dataLocation.toString())
            assertEquals("nested/sub path", subPath)
            assertEquals("path > sub path", displayFilename)

            verifyEquals(this, Identifier(Uri.parse(path), subPath))
        }
    }
}
