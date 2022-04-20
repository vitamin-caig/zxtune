package app.zxtune.fs.local

import android.content.ContentResolver
import android.content.Context
import android.net.Uri
import androidx.documentfile.provider.DocumentFile
import app.zxtune.fs.file
import app.zxtune.fs.fileDescriptor
import app.zxtune.fs.inputStream
import org.junit.Assert.assertEquals
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.doReturn
import org.mockito.kotlin.inOrder
import org.mockito.kotlin.mock
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config
import org.robolectric.annotation.Implementation
import org.robolectric.annotation.Implements


@RunWith(RobolectricTestRunner::class)
@Config(shadows = [ShadowDocumentFile::class])
class DocumentTest {

    private val resolver = mock<ContentResolver>()
    private val context = mock<Context> {
        on { contentResolver } doReturn resolver
    }
    private val docUri = Uri.parse("shcme://host/path")

    @Test
    fun `not a document`() {
        ShadowDocumentFile.document = null
        assertEquals(null, Document.tryResolve(context, docUri))
    }

    @Test
    fun `not a file`() {
        ShadowDocumentFile.document = mock()
        assertEquals(null, Document.tryResolve(context, docUri))
    }

    @Test
    fun `proper document`() {
        val doc = mock<DocumentFile> {
            on { isFile } doReturn true
            on { uri } doReturn docUri
            on { name } doReturn "DocName"
            on { length() } doReturn 24576L
        }
        ShadowDocumentFile.document = doc
        requireNotNull(Document.tryResolve(context, docUri)).run {
            assertEquals(null, inputStream)
            assertEquals(docUri, uri)
            assertEquals(null, file)
            assertEquals("DocName", name)
            // not a null really, just check mock calls
            assertEquals(null, fileDescriptor)
            assertEquals("24.0K", size)
            assertEquals(null, parent)
        }

        inOrder(doc, resolver) {
            verify(doc).uri
            verify(doc).name
            verify(resolver).openFileDescriptor(docUri, "r")
            verify(doc).length()
        }
    }
}

@Implements(DocumentFile::class)
class ShadowDocumentFile {
    companion object {
        var document: DocumentFile? = null

        @Implementation
        @JvmStatic
        fun isDocumentUri(ctx: Context, uri: Uri) = document != null

        @Implementation
        @JvmStatic
        fun fromSingleUri(ctx: Context, uri: Uri) = document
    }
}
