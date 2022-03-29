package app.zxtune.fs

import android.content.ContentResolver
import android.content.Context
import android.content.Intent
import android.database.MatrixCursor
import android.net.Uri
import android.os.Environment
import android.os.storage.StorageManager
import android.os.storage.StorageVolume
import android.provider.DocumentsContract
import app.zxtune.fs.local.Identifier
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.*
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config
import java.io.File

@RunWith(RobolectricTestRunner::class)
@Config(sdk = [VfsRootLocalStorageAccessFramework.REQUIRED_SDK_LEVEL])
class VfsRootLocalStorageAccessFrameworkTest {

    private val resolver = mock<ContentResolver>()
    private val storageManager = mock<StorageManager>()
    private val context = mock<Context> {
        on { contentResolver } doReturn resolver
        on { getSystemService(StorageManager::class.java) } doReturn storageManager
    }

    private val underTest = VfsRootLocalStorageAccessFramework(context)

    private val visitor = mock<VfsDir.Visitor>()

    @Before
    fun setUp() = reset(resolver, storageManager, visitor)

    @After
    fun tearDown() = verifyNoMoreInteractions(resolver, storageManager, context, visitor)

    @Test
    fun `no storages`() {
        assertEquals(null, underTest.parent)
        underTest.enumerate(visitor)
        assertEquals(underTest, underTest.resolve(Uri.parse("file:")))

        inOrder(context, storageManager, visitor) {
            verify(context).getSystemService(StorageManager::class.java)
            verify(storageManager).storageVolumes
            verify(visitor).onItemsCount(0)
        }
    }

    @Test
    fun `phantom dir`() {
        val id = Identifier("storage", "path/subpath")
        val permittedId1 = Identifier("storage", "path/subpath/dir/accessible")
        val permittedId2 = Identifier("storage", "path/subpath/access")
        resolver.stub {
            on { getType(any()) } doReturn DocumentsContract.Document.MIME_TYPE_DIR
            on { query(any(), any(), anyOrNull(), anyOrNull()) } doThrow SecurityException()
            on { persistedUriPermissions } doAnswer {
                arrayOf(permittedId1, permittedId2).map { permId ->
                    mock {
                        on { uri } doReturn permId.treeDocumentUri
                        on { isReadPermission } doReturn true
                    }
                }
            }
        }
        (underTest.resolve(id.fsUri) as VfsDir).run {
            assertEquals("subpath", name)
            assertEquals(id.documentUri, permissionQueryUri)
            enumerate(visitor)
        }
        val captor = argumentCaptor<VfsDir>()
        inOrder(context, resolver, visitor) {
            verify(context).contentResolver
            // resolve
            verify(resolver).getType(id.documentUri)
            // permission query uri + enumerate
            verify(resolver, times(2)).persistedUriPermissions
            verify(visitor).onItemsCount(2)
            verify(visitor, times(2)).onDir(captor.capture())
        }
        verifyNoMoreInteractions(resolver)
        clearInvocations(resolver)

        assertEquals(2, captor.allValues.size)
        captor.firstValue.run {
            assertEquals("dir", name)
            assertEquals(permittedId1.parent.fsUri, uri)
            assertEquals(permittedId1.parent.documentUri, permissionQueryUri)
        }
        captor.secondValue.run {
            assertEquals("access", name)
            assertEquals(permittedId2.fsUri, uri)
            assertEquals(null, permissionQueryUri)
        }
        verify(resolver, times(2)).persistedUriPermissions
    }

    @Test
    fun `legacy file access`() {
        (underTest.resolve(Uri.parse("file:///some/path")) as VfsFile).run {
            assertEquals("path", name)
            assertEquals("/some/path", file!!.absolutePath)
        }
    }

    @Test
    fun `modern file access`() {
        val mountpoint = File("/mnt/storage")
        val rootId = Identifier.fromRoot("stor")
        val id = Identifier("stor", "path/to/file")
        resolver.stub {
            on { getType(any()) } doReturn "binary/type"
        }
        val volume = mock<StorageVolume> {
            on { createOpenDocumentTreeIntent() } doAnswer {
                Intent("unused").putExtra(DocumentsContract.EXTRA_INITIAL_URI, rootId.rootUri)
            }
            on { state } doReturn Environment.MEDIA_MOUNTED
            on { directory } doReturn mountpoint
        }
        storageManager.stub {
            on { storageVolumes } doAnswer {
                listOf(volume)
            }
        }
        (underTest.resolve(id.fsUri) as VfsFile).run {
            assertEquals("file", name)
            assertEquals("/mnt/storage/path/to/file", file!!.absolutePath)
        }
        inOrder(context, resolver, storageManager) {
            verify(context).contentResolver
            verify(resolver).getType(id.documentUri)
            verify(context).getSystemService(StorageManager::class.java)
            verify(storageManager).storageVolumes
        }
    }

    @Test
    fun `root storages`() {
        val id1 = Identifier.fromRoot("root")
        val vol1 = mock<StorageVolume> {
            on { state } doReturn Environment.MEDIA_MOUNTED
            on { getDescription(any()) } doReturn "Root"
            on { createOpenDocumentTreeIntent() } doAnswer {
                Intent("unused").putExtra(DocumentsContract.EXTRA_INITIAL_URI, id1.rootUri)
            }
        }
        val id2 = Identifier.fromRoot("readonly")
        val vol2 = mock<StorageVolume> {
            on { state } doReturn Environment.MEDIA_MOUNTED_READ_ONLY
            on { getDescription(any()) } doReturn "ReadOnly"
            on { createOpenDocumentTreeIntent() } doAnswer {
                Intent("unused").putExtra(DocumentsContract.EXTRA_INITIAL_URI, id2.rootUri)
            }
        }
        val id3 = Identifier.fromRoot("unmounted")
        val vol3 = mock<StorageVolume> {
            on { state } doReturn Environment.MEDIA_UNMOUNTED
            on { createOpenDocumentTreeIntent() } doAnswer {
                Intent("unused").putExtra(DocumentsContract.EXTRA_INITIAL_URI, id3.rootUri)
            }
        }
        storageManager.stub {
            on { storageVolumes } doAnswer {
                listOf(vol1, vol2, vol3)
            }
        }
        underTest.enumerate(visitor)
        (underTest.resolve(id2.fsUri) as VfsDir).run {
            assertEquals("readonly", name)
        }
        val captor = argumentCaptor<VfsDir>()
        inOrder(context, storageManager, visitor) {
            verify(context).getSystemService(StorageManager::class.java)
            verify(storageManager).storageVolumes
            verify(visitor).onItemsCount(3)
            verify(visitor, times(2)).onDir(captor.capture())
        }
        assertEquals(2, captor.allValues.size)
        captor.firstValue.run {
            assertEquals("root", name)
            assertEquals("Root", description)
            assertEquals(underTest, parent)
            assertEquals(id1.fsUri, uri)
            assertEquals(id1.rootUri, permissionQueryUri)
        }
        captor.secondValue.run {
            assertEquals(underTest, parent)
            assertEquals(id2.fsUri, uri)
            assertEquals("readonly", name)
            assertEquals("ReadOnly", description)
            assertEquals(id2.rootUri, permissionQueryUri)
        }
    }

    @Test
    fun `real dir`() {
        val permId = Identifier("root", "")
        val id = Identifier("root", "dir")
        val subDir = Identifier("root", "dir/subdir")
        val subFile = Identifier("root", "dir/subfile")
        resolver.stub {
            on { getType(any()) } doReturn DocumentsContract.Document.MIME_TYPE_DIR
            on { query(any(), any(), anyOrNull(), anyOrNull()) } doAnswer {
                MatrixCursor(it.getArgument<Array<String>>(1)).apply {
                    newRow().apply {
                        add(DocumentsContract.Document.COLUMN_DOCUMENT_ID, subDir.documentId)
                        add(
                            DocumentsContract.Document.COLUMN_MIME_TYPE,
                            DocumentsContract.Document.MIME_TYPE_DIR
                        )
                        add(DocumentsContract.Document.COLUMN_SUMMARY, "Directory")
                    }
                    newRow().apply {
                        add(DocumentsContract.Document.COLUMN_DOCUMENT_ID, subFile.documentId)
                        add(DocumentsContract.Document.COLUMN_MIME_TYPE, "binary/octet")
                        add(DocumentsContract.Document.COLUMN_SUMMARY, "File")
                        add(DocumentsContract.Document.COLUMN_SIZE, 32768)
                    }
                }
            }
            on { persistedUriPermissions } doAnswer {
                listOf(
                    mock {
                        on { uri } doReturn permId.treeDocumentUri
                        on { isReadPermission } doReturn true
                    }
                )
            }
        }
        (underTest.resolve(id.fsUri) as VfsDir).run {
            assertEquals("dir", name)
            assertEquals(null, permissionQueryUri)
            enumerate(visitor)
        }
        inOrder(context, resolver, visitor)
        {
            verify(context).contentResolver
            // resolve
            verify(resolver).getType(id.documentUri)
            // permission query uri
            verify(resolver).persistedUriPermissions
            // enumerate
            verify(resolver).query(
                eq(id.getTreeChildDocumentUri(permId)),
                any(),
                eq(null),
                eq(null)
            )
            verify(visitor).onItemsCount(2)
            verify(visitor).onDir(argThat {
                assertEquals("subdir", name)
                assertEquals("Directory", description)
                assertEquals(subDir.fsUri, uri)
                assertEquals(null, permissionQueryUri)
                true
            })
            verify(visitor).onFile(argThat {
                assertEquals("subfile", name)
                assertEquals("File", description)
                assertEquals("32.0K", size)
                assertEquals(subFile.fsUri, uri)
                true
            })
        }
    }
}
