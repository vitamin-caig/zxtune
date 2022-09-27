package app.zxtune.fs.local

import android.content.ContentResolver
import android.content.Context
import android.net.Uri
import android.provider.MediaStore
import androidx.documentfile.provider.DocumentFile
import app.zxtune.fs.file
import app.zxtune.fs.fileDescriptor
import app.zxtune.fs.inputStream
import org.junit.Assert.assertEquals
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.*
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config
import org.robolectric.annotation.Implementation
import org.robolectric.annotation.Implements

@RunWith(RobolectricTestRunner::class)
@Config(shadows = [ShadowDocumentFile::class, ShadowMediaStore::class], sdk = [29])
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
            verify(doc, times(2)).uri
            verify(doc).name
            verify(resolver).openFileDescriptor(docUri, "r")
            verify(doc).length()
        }
    }

    @Test
    fun `media uri`() {
        val doc = mock<DocumentFile> {
            on { isFile } doReturn true
            on { uri } doReturn docUri
            on { name } doReturn "DocName"
            on { length() } doReturn 24576L
        }
        ShadowDocumentFile.document = doc
        ShadowMediaStore.getDocUri = mock {
            on { invoke(any()) } doReturn docUri
        }
        requireNotNull(
            Document.tryResolve(
                context,
                Uri.parse("content://0@media/external/music/2")
            )
        ).run {
            assertEquals(null, inputStream)
            assertEquals(docUri, uri)
            assertEquals(null, file)
            assertEquals("DocName", name)
            // not a null really, just check mock calls
            assertEquals(null, fileDescriptor)
            assertEquals("24.0K", size)
            assertEquals(null, parent)
        }

        // Multiple calls of doc.uri interleaved with other calls are not supported by inOrder
        /*inOrder(doc, ShadowMediaStore.getDocUri, resolver)*/ run {
            verify(doc, times(2)).uri
            verify(ShadowMediaStore.getDocUri).invoke(Uri.parse("content://media/external/music/2"))
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
        fun isDocumentUri(ctx: Context, uri: Uri) = fromSingleUri(ctx, uri) != null

        @Implementation
        @JvmStatic
        fun fromSingleUri(ctx: Context, uri: Uri) = document?.takeIf { uri == it.uri }
    }
}

@Implements(MediaStore::class)
class ShadowMediaStore {
    companion object {
        lateinit var getDocUri: (Uri) -> Uri

        @Implementation
        @JvmStatic
        fun getDocumentUri(ctx: Context, uri: Uri) = getDocUri(uri)
    }
}
