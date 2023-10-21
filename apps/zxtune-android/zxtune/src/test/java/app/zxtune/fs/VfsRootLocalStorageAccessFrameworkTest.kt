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
import androidx.documentfile.provider.DocumentFile
import app.zxtune.Features
import app.zxtune.fs.local.Identifier
import app.zxtune.fs.local.ShadowDocumentFile
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertSame
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.*
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config
import java.io.File
import java.io.FileDescriptor

@RunWith(RobolectricTestRunner::class)
@Config(sdk = [Features.StorageAccessFramework.REQUIRED_SDK], shadows = [ShadowDocumentFile::class])
class VfsRootLocalStorageAccessFrameworkTest {

    private val resolver = mock<ContentResolver>()
    private val storageManager = mock<StorageManager>()
    private val context = mock<Context> {
        on { contentResolver } doReturn resolver
        on { getSystemService(StorageManager::class.java) } doReturn storageManager
    }
    private val documentFile = mock<DocumentFile>()

    private val underTest = VfsRootLocalStorageAccessFramework(context)

    private val visitor = mock<VfsDir.Visitor>()

    @Before
    fun setUp() {
        reset(resolver, storageManager, documentFile, visitor)
        ShadowDocumentFile.document = documentFile
    }

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
            assertEquals(id.documentUri, permissionQueryIntent!!.data)
            enumerate(visitor)
        }
        val captor = argumentCaptor<VfsDir>()
        inOrder(context, resolver, visitor) {
            verify(context).contentResolver
            // resolve(try real, try phantom) + permission query uri + enumerate
            verify(resolver, times(4)).persistedUriPermissions
            verify(visitor).onItemsCount(2)
            verify(visitor, times(2)).onDir(captor.capture())
        }
        verify(context).packageName //intent, somewhere in the middle of uri permissions requests
        verifyNoMoreInteractions(resolver, context)
        clearInvocations(resolver, context)

        assertEquals(2, captor.allValues.size)
        captor.firstValue.run {
            assertEquals("dir", name)
            assertEquals(permittedId1.parent.fsUri, uri)
            assertEquals(permittedId1.parent.documentUri, permissionQueryIntent!!.data)
        }
        captor.secondValue.run {
            assertEquals("access", name)
            assertEquals(permittedId2.fsUri, uri)
            assertEquals(null, permissionQueryIntent)
        }
        verify(resolver, times(2)).persistedUriPermissions
        // intent creating
        verify(context).packageName
    }

    @Test
    fun `legacy file access`() {
        val id1 = Identifier.fromRoot("root")
        val vol1 = mock<StorageVolume> {
            on { state } doReturn Environment.MEDIA_MOUNTED
            on { createOpenDocumentTreeIntent() } doAnswer {
                Intent("unused").putExtra(DocumentsContract.EXTRA_INITIAL_URI, id1.rootUri)
            }
            on { directory } doAnswer {
                File("/some/path")
            }
        }
        val id2 = Identifier.fromRoot("root2")
        val vol2 = mock<StorageVolume> {
            on { state } doReturn Environment.MEDIA_MOUNTED
            on { createOpenDocumentTreeIntent() } doAnswer {
                Intent("unused").putExtra(DocumentsContract.EXTRA_INITIAL_URI, id2.rootUri)
            }
            on { directory } doAnswer {
                File("/dir/mountpoint")
            }
        }
        storageManager.stub {
            on { storageVolumes } doAnswer {
                listOf(vol1, vol2)
            }
        }

        // don't care about result really
        assertEquals(null, underTest.resolve(Uri.parse("file:/dir/mountpoint/sub/path")))
        assertEquals(null, underTest.resolve(Uri.parse("file:/unknown/path/no/volumes")))
        (underTest.resolve(Uri.parse("file:/some/path")) as VfsDir).run {
            assertEquals("root", name)
        }
        inOrder(context, storageManager, resolver) {
            verify(context).getSystemService(StorageManager::class.java)
            // subpath
            verify(storageManager).storageVolumes
            verify(context).contentResolver
            verify(resolver, times(2)).persistedUriPermissions
            // unknown + volume root
            verify(storageManager, times(2)).storageVolumes
        }
    }

    @Test
    fun `modern file access`() {
        val permId = Identifier("stor", "path")
        val id = Identifier("stor", "path/to/file")
        val descriptor = FileDescriptor()
        resolver.stub {
            on { persistedUriPermissions } doAnswer {
                listOf(mock {
                    on { uri } doReturn permId.treeDocumentUri
                    on { isReadPermission } doReturn true
                })
            }
            on { openFileDescriptor(any(), any()) } doAnswer {
                mock {
                    on { fileDescriptor } doReturn descriptor
                }
            }
        }
        documentFile.stub {
            on { uri } doReturn id.getDocumentUriUsingTree(permId)
        }
        (underTest.resolve(id.fsUri) as VfsFile).run {
            assertEquals("file", name)
            assertEquals(null, file)
            assertSame(descriptor, fileDescriptor)
        }
        inOrder(context, resolver) {
            verify(context).contentResolver
            // findAncestor
            verify(resolver).persistedUriPermissions
            verify(resolver).openFileDescriptor(id.getDocumentUriUsingTree(permId), "r")
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
            assertEquals(id1.rootUri, permissionQueryIntent!!.data)
        }
        captor.secondValue.run {
            assertEquals(underTest, parent)
            assertEquals(id2.fsUri, uri)
            assertEquals("readonly", name)
            assertEquals("ReadOnly", description)
            assertEquals(id2.rootUri, permissionQueryIntent!!.data)
        }
        // intent creating
        verify(context, times(2)).packageName
    }

    @Test
    fun `real dir`() {
        val permId = Identifier("root", "")
        val id = Identifier("root", "dir")
        val subDir = Identifier("root", "dir/subdir")
        val subFile = Identifier("root", "dir/subfile")
        resolver.stub {
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
                listOf(mock {
                    on { uri } doReturn permId.treeDocumentUri
                    on { isReadPermission } doReturn true
                })
            }
        }
        documentFile.stub {
            on { uri } doReturn id.getDocumentUriUsingTree(permId)
            on { isDirectory } doReturn true
        }
        (underTest.resolve(id.fsUri) as VfsDir).run {
            assertEquals("dir", name)
            assertEquals(null, permissionQueryIntent)
            enumerate(visitor)
        }
        inOrder(context, resolver, visitor) {
            verify(context).contentResolver
            // resolve
            verify(resolver).persistedUriPermissions
            // enumerate
            verify(resolver).query(
                eq(id.getTreeChildDocumentUri(permId)), any(), eq(null), eq(null)
            )
            verify(visitor).onItemsCount(2)
            verify(visitor).onDir(argThat {
                assertEquals("subdir", name)
                assertEquals("Directory", description)
                assertEquals(subDir.fsUri, uri)
                assertEquals(null, permissionQueryIntent)
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

    @Test
    fun `document file`() {
        val docUri = Uri.parse("content://authority/path")
        val doc = mock<DocumentFile> {
            on { isFile } doReturn true
            on { uri } doReturn docUri
        }
        ShadowDocumentFile.document = doc
        (underTest.resolve(docUri) as VfsFile).run {
            assertEquals(docUri, uri)
        }
        verify(context).contentResolver
    }
}
