package app.zxtune.fs.local

import android.net.Uri
import app.zxtune.assertThrows
import org.junit.Assert.assertEquals
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class IdentifierTest {
    private val rootIdentifier = Identifier("someroot", "")
    private val level1Identifier = Identifier("someroot", "some path")
    private val level3Identifier = Identifier("someroot", "1/2/3")

    @Test
    fun `empty identifier`() = with(Identifier.EMPTY) {
        assertEquals("", root)
        assertEquals("", path)
        assertEquals("", name)
        assertEquals(Identifier.EMPTY, parent)
        assertEquals("file://", fsUri.toString())
        assertEquals("content://com.android.externalstorage.documents/root/", rootUri.toString())
        assertEquals(
            "content://com.android.externalstorage.documents/document/",
            documentUri.toString()
        )
        assertEquals(
            "content://com.android.externalstorage.documents/tree/someroot/document/",
            getDocumentUriUsingTree(rootIdentifier).toString()
        )
        assertEquals(
            "content://com.android.externalstorage.documents/tree/someroot%3Asome%20path/document/",
            getDocumentUriUsingTree(level1Identifier).toString()
        )
        Unit
    }

    @Test
    fun `root identifier`() = with(rootIdentifier) {
        assertEquals("someroot", root)
        assertEquals("", path)
        assertEquals("someroot", name)
        assertEquals(Identifier.EMPTY, parent)
        assertEquals("file://someroot", fsUri.toString())
        assertEquals(
            "content://com.android.externalstorage.documents/root/someroot",
            rootUri.toString()
        )
        assertEquals(
            "content://com.android.externalstorage.documents/document/someroot",
            documentUri.toString()
        )
        assertEquals(
            "content://com.android.externalstorage.documents/tree/someroot%3Asome%20path/document/someroot",
            getDocumentUriUsingTree(level1Identifier).toString()
        )
        assertEquals(
            "content://com.android.externalstorage.documents/tree/someroot%3A1%2F2%2F3/document/someroot/children",
            getTreeChildDocumentUri(level3Identifier).toString()
        )
    }

    @Test
    fun `level 1 identifier`() = with(level1Identifier) {
        assertEquals("someroot", root)
        assertEquals("some path", path)
        assertEquals("some path", name)
        assertEquals(Identifier("someroot", ""), parent)
        assertEquals("file://someroot/some%20path", fsUri.toString())
        assertEquals(
            "content://com.android.externalstorage.documents/root/someroot",
            rootUri.toString()
        )
        assertEquals(
            "content://com.android.externalstorage.documents/document/someroot%3Asome%20path",
            documentUri.toString()
        )
        assertEquals(
            "content://com.android.externalstorage.documents/tree/someroot/document/someroot%3Asome%20path",
            getDocumentUriUsingTree(rootIdentifier).toString()
        )
        assertEquals(
            "content://com.android.externalstorage.documents/tree/someroot%3A1%2F2%2F3/document/someroot%3Asome%20path/children",
            getTreeChildDocumentUri(level3Identifier).toString()
        )
    }

    @Test
    fun `level 3 identifier`() = with(level3Identifier) {
        assertEquals("someroot", root)
        assertEquals("1/2/3", path)
        assertEquals("3", name)
        assertEquals(Identifier("someroot", "1/2"), parent)
        assertEquals("file://someroot/1/2/3", fsUri.toString())
        assertEquals(
            "content://com.android.externalstorage.documents/root/someroot",
            rootUri.toString()
        )
        assertEquals(
            "content://com.android.externalstorage.documents/document/someroot%3A1%2F2%2F3",
            documentUri.toString()
        )
        assertEquals(
            "content://com.android.externalstorage.documents/tree/someroot/document/someroot%3A1%2F2%2F3",
            getDocumentUriUsingTree(rootIdentifier).toString()
        )
        assertEquals(
            "content://com.android.externalstorage.documents/tree/someroot%3Asome%20path/document/someroot%3A1%2F2%2F3/children",
            getTreeChildDocumentUri(level1Identifier).toString()
        )
    }

    @Test
    fun `legacy identifier`() = with(Identifier("", "sub/path")) {
        assertEquals("", root)
        assertEquals("sub/path", path)
        assertEquals("path", name)
        assertEquals(Identifier("", "sub"), parent)
        assertEquals("file:///sub/path", fsUri.toString())
        assertEquals(
            "content://com.android.externalstorage.documents/root/",
            rootUri.toString()
        )
        assertEquals(
            "content://com.android.externalstorage.documents/document/%3Asub%2Fpath",
            documentUri.toString()
        )
        assertEquals(
            "content://com.android.externalstorage.documents/tree/someroot/document/%3Asub%2Fpath",
            getDocumentUriUsingTree(rootIdentifier).toString()
        )
        assertEquals(
            "content://com.android.externalstorage.documents/tree/someroot%3Asome%20path/document/%3Asub%2Fpath/children",
            getTreeChildDocumentUri(level1Identifier).toString()
        )
    }

    @Test
    fun `test fromFsUri`() {
        assertEquals(null, Identifier.fromFsUri(Uri.EMPTY))
        assertEquals(null, Identifier.fromFsUri(Uri.parse("http://authority")))
        assertEquals(
            Identifier("", "some sub/path"),
            Identifier.fromFsUri(Uri.parse("file:/some%20sub/path"))
        )
    }

    @Test
    fun `test fromDocumentId`() {
        assertEquals(Identifier.EMPTY, Identifier.fromDocumentId(""))
        assertEquals(Identifier("some root", ""), Identifier.fromDocumentId("some root"))
        assertEquals(Identifier(":some root", ""), Identifier.fromDocumentId(":some root"))
        assertEquals(
            Identifier("someroot", "some/path"),
            Identifier.fromDocumentId("someroot:some/path")
        )
    }

    @Test
    fun `test fromTreeDocumentUri`() {
        assertThrows<java.lang.IllegalArgumentException> {
            Identifier.fromTreeDocumentUri(Uri.parse("http://example.org"))
        }
        assertEquals(
            Identifier("someroot", "some/path"), Identifier.fromTreeDocumentUri(
                Uri.parse("content://com.android.externalstorage.documents/tree/someroot%3Asome%2Fpath/document/%3Aunused")
            )
        )
    }
}
