package app.zxtune.coverart

import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.net.Uri
import app.zxtune.core.Identifier
import app.zxtune.core.Module
import app.zxtune.core.ModuleAttributes
import app.zxtune.fs.VfsExtensions
import app.zxtune.fs.VfsObject
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertSame
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.any
import org.mockito.kotlin.anyOrNull
import org.mockito.kotlin.argThat
import org.mockito.kotlin.doReturn
import org.mockito.kotlin.eq
import org.mockito.kotlin.inOrder
import org.mockito.kotlin.mock
import org.mockito.kotlin.stub
import org.mockito.kotlin.times
import org.mockito.kotlin.verify
import org.mockito.kotlin.verifyNoMoreInteractions
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config
import org.robolectric.annotation.Implementation
import org.robolectric.annotation.Implements

@RunWith(RobolectricTestRunner::class)
@Config(shadows = [ShadowBitmapFactory::class])
class CoverartServiceTest {

    private val rootUri = Uri.parse("schema:/host")
    private val dirUri = rootUri.buildUpon().appendPath("path").build()
    private val moduleUri = dirUri.buildUpon().appendPath("module").build()
    private val moduleId = Identifier(moduleUri)
    private val archiveUri = dirUri.buildUpon().appendPath("archive").build()
    private val archiveId = Identifier(archiveUri)
    private val archivedModuleId = Identifier(archiveUri, "sub/path")
    private val picPath = "sub/image"
    private val archivedPicId = Identifier(archiveUri, picPath)
    private val picUri = Uri.parse("http://host/path")
    private val picBlob = ByteArray(1)
    private val db = mock<Database>()
    private val module = mock<Module>()
    private val bitmap = mock<Bitmap>()
    private val obj = mock<VfsObject>()

    private val underTest = CoverartService(db)

    @After
    fun tearDown() {
        verifyNoMoreInteractions(db, module, bitmap, ShadowBitmapFactory.mock, obj)
    }

    @Test
    fun `test Location`() {
        val png = Location("image.png").apply {
            assertEquals("", dir)
            assertEquals("image.png", name)
            assertEquals(1, priority)
        }
        val jpg = Location("images/folder.jpg").apply {
            assertEquals("images", dir)
            assertEquals("folder.jpg", name)
            assertEquals(203, priority)
        }
        val jpeg = Location("sub/folder/cover.jpeg").apply {
            assertEquals("sub/folder", dir)
            assertEquals("cover.jpeg", name)
            assertEquals(302, priority)
        }

        png.run {
            assertEquals(0, directDistanceTo(png))
            assertEquals(1, directDistanceTo(jpg))
            assertEquals(2, directDistanceTo(jpeg))
        }
        jpg.run {
            assertEquals(-1, directDistanceTo(png))
            assertEquals(0, directDistanceTo(jpg))
            assertEquals(null, directDistanceTo(jpeg))
            assertEquals(null, directDistanceTo(Location("images2/folder.jpg")))
            assertEquals(null, directDistanceTo(Location("imag/folder.jpg")))
        }
        jpeg.run {
            assertEquals(-2, directDistanceTo(png))
            assertEquals(null, directDistanceTo(jpg))
            assertEquals(0, directDistanceTo(jpeg))
        }
    }

    @Test
    fun `test ImagesSet`() {
        val underTest =
            ImagesSet(listOf("dir/sub/art/front.jpg", "dir/sub/art/back.jpg", "dir/image.png"))
        assertEquals("dir/sub/art/front.jpg", underTest.selectAlbumArt("dir/sub/track.mp3"))
        assertEquals("dir/image.png", underTest.selectAlbumArt("dir/track.ogg"))
        assertEquals(null, underTest.selectAlbumArt("dir2/track.wav"))
    }

    @Test
    fun `service cleanupFor`() {
        underTest.cleanupFor(archiveUri)
        verify(db).remove(archiveUri)
    }

    @Test
    fun `addEmbedded cached`() {
        db.stub {
            on { hasEmbedded(any()) } doReturn true
        }
        underTest.addEmbedded(archivedModuleId, module)
        verify(db).hasEmbedded(archivedModuleId)
    }

    @Test
    fun `addEmbedded absent`() {
        underTest.addEmbedded(archivedModuleId, module)
        inOrder(db, module) {
            verify(db).hasEmbedded(archivedModuleId)
            verify(module).getProperty(ModuleAttributes.PICTURE, null)
        }
    }

    @Test
    fun `addEmbedded invalid`() {
        module.stub {
            on { getProperty(any(), anyOrNull<ByteArray>()) } doReturn picBlob
        }
        underTest.addEmbedded(archivedModuleId, module)
        inOrder(db, module, ShadowBitmapFactory.mock) {
            verify(db).hasEmbedded(archivedModuleId)
            verify(module).getProperty(ModuleAttributes.PICTURE, null)
            verify(ShadowBitmapFactory.mock).invoke(picBlob, 0, picBlob.size)
        }
    }

    @Test
    fun `addEmbedded small`() {
        module.stub {
            on { getProperty(any(), anyOrNull<ByteArray>()) } doReturn picBlob
        }
        ShadowBitmapFactory.mock.stub {
            on { invoke(any(), any(), any()) } doReturn bitmap
        }
        bitmap.stub {
            on { width } doReturn 100
            on { height } doReturn 200
            on { compress(any(), any(), any()) } doReturn true
        }
        underTest.addEmbedded(archivedModuleId, module)
        inOrder(db, module, ShadowBitmapFactory.mock, bitmap) {
            verify(db).hasEmbedded(archivedModuleId)
            verify(module).getProperty(ModuleAttributes.PICTURE, null)
            verify(ShadowBitmapFactory.mock).invoke(picBlob, 0, picBlob.size)
            verify(bitmap).width
            verify(bitmap).height
            verify(bitmap).allocationByteCount
            verify(bitmap).compress(eq(Bitmap.CompressFormat.JPEG), eq(90), any())
            verify(bitmap).recycle()
            verify(db).addEmbedded(eq(archivedModuleId), argThat<ByteArray> { isEmpty() })
        }
    }

    @Test
    fun `imageFor cases`() {
        db.stub {
            on { query(any()) } doReturn null doReturn Reference(
                archivedModuleId.dataLocation, archivedModuleId.subPath
            ) doReturn Reference(
                archivedModuleId.dataLocation, archivedModuleId.subPath, externalPicture = picUri
            ) doReturn Reference(
                archivedModuleId.dataLocation, archivedModuleId.subPath, embeddedPicture = picBlob
            )
        }
        assertSame("unknown", archiveUri, underTest.imageFor(archiveUri))
        assertEquals("no images", null, underTest.imageFor(archiveUri))
        assertSame("external image", picUri, underTest.imageFor(archiveUri))
        assertSame("embedded image", picBlob, underTest.imageFor(archiveUri))

        verify(db, times(4)).query(argThat { dataLocation == archiveUri && subPath == "" })
    }

    @Test
    fun `coverArtOf cases`() {
        db.stub {
            on { hasEmbedded(any()) } doReturn false doReturn true
        }

        assertEquals("unknown", null, underTest.coverArtOf(archivedModuleId))
        assertEquals("known", archivedModuleId.fullLocation, underTest.coverArtOf(archivedModuleId))

        verify(db, times(2)).hasEmbedded(archivedModuleId)
    }

    @Test
    fun `albumArtOf cached`() {
        db.stub {
            on { query(any()) } doReturn Reference(
                archivedModuleId.dataLocation, archivedModuleId.subPath, externalPicture = picUri
            )
        }
        assertSame(picUri, underTest.albumArtOf(archivedModuleId, obj))

        verify(db).query(archivedModuleId)
    }

    @Test
    fun `albumArtOf archive with cached image`() {
        db.stub {
            on { query(any()) } doReturn null doReturn Reference(
                Uri.EMPTY, "unused", externalPicture = picUri
            )
        }
        assertSame(picUri, underTest.albumArtOf(archivedModuleId, obj))

        inOrder(db) {
            verify(db).query(archivedModuleId)
            verify(db).query(archiveId)
        }
    }

    @Test
    fun `archiveArtOf not archive`() {
        assertNull(underTest.archiveArtOf(archiveId))
    }

    @Test
    fun `archiveArtOf cached archive with no images`() {
        db.stub {
            on { query(any()) } doReturn Reference(Uri.EMPTY, "unused")
        }
        assertNull(underTest.archiveArtOf(archivedModuleId))

        verify(db).query(archiveId)
    }

    @Test
    fun `archiveArtOf archive with no images`() {
        assertNull(underTest.archiveArtOf(archivedModuleId))

        inOrder(db) {
            verify(db).query(archiveId)
            verify(db).listArchiveImages(archiveUri)
            verify(db).setArchiveImage(archiveUri, null)
        }
    }

    @Test
    fun `archiveArtOf archive with sole image`() {
        db.stub {
            on { listArchiveImages(any()) } doReturn listOf(picPath)
        }
        assertEquals(archivedPicId.fullLocation, underTest.archiveArtOf(archivedModuleId))

        inOrder(db) {
            verify(db).query(archiveId)
            verify(db).listArchiveImages(archiveUri)
        }
    }

    @Test
    fun `archiveArtOf archive with many images`() {
        db.stub {
            on { listArchiveImages(any()) } doReturn listOf(picPath, "sub/sub/image")
        }
        assertEquals(archivedPicId.fullLocation, underTest.archiveArtOf(archivedModuleId))

        inOrder(db) {
            verify(db).query(archiveId)
            verify(db).listArchiveImages(archiveUri)
            // image is not set for archive - depends on module
        }
    }

    @Test
    fun `albumArtOf no images at all`() {
        assertNull(underTest.albumArtOf(archivedModuleId, obj))

        inOrder(db, obj) {
            verify(db).query(archivedModuleId)
            verify(db).query(archiveId)
            verify(db).listArchiveImages(archiveUri)
            verify(db).setArchiveImage(archiveUri, null)
            verify(obj).parent
        }
    }

    @Test
    fun `albumArtOf cached dir coverart`() {
        obj.stub {
            on { uri } doReturn dirUri doReturn rootUri
            on { parent } doReturn obj
        }
        db.stub {
            on { query(any()) } doReturn null doReturn null doReturn Reference(
                Uri.EMPTY, "unused", externalPicture = picUri
            )
        }
        assertEquals(picUri, underTest.albumArtOf(moduleId, obj))
        inOrder(db, obj) {
            verify(db).query(moduleId)
            verify(obj).parent
            verify(obj).uri
            verify(db).query(Identifier(dirUri))
            verify(obj).getExtension(VfsExtensions.COVER_ART_URI)
            verify(obj).parent
            verify(obj).uri
            verify(db).query(Identifier(rootUri))
        }
    }

    @Test
    fun `albumArtOf parent dir coverart`() {
        obj.stub {
            on { uri } doReturn dirUri
            on { parent } doReturn obj
            on { getExtension(any()) } doReturn picUri
        }
        assertEquals(picUri, underTest.albumArtOf(moduleId, obj))
        inOrder(db, obj) {
            verify(db).query(moduleId)
            verify(obj).parent
            verify(obj).uri
            verify(db).query(Identifier(dirUri))
            verify(obj).getExtension(VfsExtensions.COVER_ART_URI)
            verify(db).addExternal(moduleId, picUri)
            verify(db).addExternal(Identifier(dirUri), picUri)
        }
    }
}

@Implements(BitmapFactory::class)
object ShadowBitmapFactory {
    val mock: (ByteArray, Int, Int) -> Bitmap? = mock()

    @JvmStatic
    @Implementation
    fun decodeByteArray(data: ByteArray, offset: Int, size: Int) = mock.invoke(data, offset, size)
}
