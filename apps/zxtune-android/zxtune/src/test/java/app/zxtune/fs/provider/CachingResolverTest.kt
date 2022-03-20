package app.zxtune.fs.provider

import android.net.Uri
import app.zxtune.fs.ShadowVfsArchive
import app.zxtune.fs.VfsDir
import app.zxtune.fs.VfsFile
import app.zxtune.utils.ProgressCallback
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.*
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config

@RunWith(RobolectricTestRunner::class)
@Config(shadows = [ShadowVfsArchive::class])
class CachingResolverTest {

    private val fileUri = mock<Uri>()
    private val dirUri = mock<Uri>()
    private val archivedFileUri = mock<Uri>()
    private val unknownUri = mock<Uri>()

    private val file = mock<VfsFile>()
    private val dir = mock<VfsDir>()
    private val archivedFile = mock<VfsFile>()

    private val progress = mock<ProgressCallback>()

    init {
        ShadowVfsArchive.doResolve.stub {
            on { invoke(eq(fileUri), anyOrNull()) } doReturn file
            on { invoke(eq(dirUri), anyOrNull()) } doReturn dir
            on { invoke(eq(archivedFileUri), any()) } doAnswer {
                it.getArgument<ProgressCallback>(1).onProgressUpdate(3, 4)
                archivedFile
            }
        }
    }

    @Before
    fun setUp() = clearInvocations(ShadowVfsArchive.doResolve, progress)

    @After
    fun tearDown() = verifyNoMoreInteractions(ShadowVfsArchive.doResolve, progress)

    @Test
    fun `cache eviction`() {
        with(CachingResolver(1)) {
            assertEquals(0, cache.size())

            // miss, put
            assertEquals(file, resolve(fileUri))
            // hit
            assertEquals(file, resolve(fileUri, progress))
            // miss, put, evict
            assertEquals(dir, resolve(dirUri, progress))
            // hit
            assertEquals(dir, resolve(dirUri))
            // miss, put, evict
            assertEquals(file, resolve(fileUri, progress))
            // miss
            assertEquals(null, resolve(unknownUri))
            // miss
            assertEquals(null, resolve(unknownUri, progress))
            assertEquals(3, cache.putCount())
            assertEquals(2, cache.hitCount())
            assertEquals(1, cache.size())
            assertEquals(5, cache.missCount())
            assertEquals(2, cache.evictionCount())
        }

        inOrder(ShadowVfsArchive.doResolve, progress) {
            verify(ShadowVfsArchive.doResolve).invoke(fileUri, null)
            verify(ShadowVfsArchive.doResolve).invoke(dirUri, progress)
            verify(ShadowVfsArchive.doResolve).invoke(fileUri, progress)
            verify(ShadowVfsArchive.doResolve).invoke(unknownUri, null)
            verify(ShadowVfsArchive.doResolve).invoke(unknownUri, progress)
        }
    }

    @Test
    fun `forced resolving`() {
        with(CachingResolver(1)) {
            // miss
            assertEquals(null, resolve(archivedFileUri))
            // miss, put
            assertEquals(archivedFile, resolve(archivedFileUri, progress))
            // hit
            assertEquals(archivedFile, resolve(archivedFileUri))

            assertEquals(1, cache.putCount())
            assertEquals(1, cache.hitCount())
            assertEquals(1, cache.size())
            assertEquals(2, cache.missCount())
            assertEquals(0, cache.evictionCount())
        }
        inOrder(ShadowVfsArchive.doResolve, progress) {
            verify(ShadowVfsArchive.doResolve).invoke(archivedFileUri, null)
            verify(ShadowVfsArchive.doResolve).invoke(archivedFileUri, progress)
            verify(progress).onProgressUpdate(3, 4)
        }
    }
}
