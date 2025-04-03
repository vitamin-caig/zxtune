package app.zxtune.coverart

import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.Rect
import android.net.Uri
import androidx.core.graphics.BitmapCompat
import app.zxtune.core.Identifier
import app.zxtune.core.Module
import app.zxtune.core.ModuleAttributes
import app.zxtune.fs.VfsExtensions
import app.zxtune.fs.VfsObject
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.any
import org.mockito.kotlin.anyOrNull
import org.mockito.kotlin.argThat
import org.mockito.kotlin.doAnswer
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

    // Packed and/or multitrack module should be treated as simple track file
    private val complexModuleId = Identifier(moduleUri, "+unPACK/+unGZIP/#2")
    private val archiveUri = dirUri.buildUpon().appendPath("archive").build()
    private val archiveId = Identifier(archiveUri)
    private val archivedModuleId = Identifier(archiveUri, "sub/path")
    private val picPath = "sub/image.png"
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
            assertEquals(403, priority)
        }
        val jpeg = Location("sub/folder/cover.jpeg").apply {
            assertEquals("sub/folder", dir)
            assertEquals("cover.jpeg", name)
            assertEquals(502, priority)
        }
        Location("some/path/DSC_1234.JPG").apply {
            assertEquals("some/path", dir)
            assertEquals("DSC_1234.JPG", name)
            assertEquals(0, priority)
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
        val underTest = ImagesSet(
            listOf(
                "dir/sub/art/front.jpg", "dir/sub/art/back.jpg", "dir/image.png", "IMG_3456.JPG"
            )
        )
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
            on { findEmbedded(any()) } doReturn mock()
        }
        underTest.addEmbedded(archivedModuleId, module)
        verify(db).findEmbedded(archivedModuleId)
    }

    @Test
    fun `addEmbedded absent`() {
        db.stub {
            on { findEmbedded(any()) } doReturn null
        }
        underTest.addEmbedded(archivedModuleId, module)
        inOrder(db, module) {
            verify(db).findEmbedded(archivedModuleId)
            verify(module).getProperty(ModuleAttributes.PICTURE, null)
        }
    }

    @Test
    fun `addEmbedded invalid`() {
        db.stub {
            on { findEmbedded(any()) } doReturn null
        }
        module.stub {
            on { getProperty(any(), anyOrNull<ByteArray>()) } doReturn picBlob
        }
        underTest.addEmbedded(archivedModuleId, module)
        inOrder(db, module, ShadowBitmapFactory.mock) {
            verify(db).findEmbedded(archivedModuleId)
            verify(module).getProperty(ModuleAttributes.PICTURE, null)
            verify(ShadowBitmapFactory.mock).invoke(picBlob, 0, picBlob.size)
        }
    }

    @Test
    fun `addEmbedded small`() {
        db.stub {
            on { addImage(any(), any()) } doAnswer {
                Picture(it.getArgument<ByteArray>(1))
            }
            on { addIcon(any(), any())} doAnswer {
                Picture(it.getArgument<ByteArray>(1))
            }
            on { findEmbedded(any()) } doReturn null
        }
        module.stub {
            on { getProperty(any(), anyOrNull<ByteArray>()) } doReturn picBlob
        }
        ShadowBitmapFactory.mock.stub {
            on { invoke(any(), any(), any()) } doReturn bitmap
        }
        bitmap.stub {
            // avoid resize check...
            on { width } doReturn 60
            on { height } doReturn 90
            on { compress(any(), any(), any()) } doReturn true
        }
        underTest.addEmbedded(archivedModuleId, module)
        inOrder(db, module, ShadowBitmapFactory.mock, bitmap) {
            verify(db).findEmbedded(archivedModuleId)
            verify(module).getProperty(ModuleAttributes.PICTURE, null)
            verify(ShadowBitmapFactory.mock).invoke(picBlob, 0, picBlob.size)
            // log
            verify(bitmap).width
            verify(bitmap).height
            // publish image
            repeat(2) {
                verify(bitmap).width
                verify(bitmap).height
                verify(bitmap).hasAlpha()
                verify(bitmap).allocationByteCount
                verify(bitmap).compress(eq(Bitmap.CompressFormat.JPEG), eq(90), any())
            }
            verify(bitmap).recycle() // as image+as icon
            verify(db).addImage(eq(archivedModuleId), argThat<ByteArray> { isEmpty() })
            verify(db).addIcon(eq(archivedModuleId), argThat<ByteArray> { isEmpty() })
        }
    }

    @Test
    fun `imageFor cases`() {
        db.stub {
            on { queryImage(any()) } doReturn null doReturn PicOrUrl(null, null) doReturn PicOrUrl(
                picUri
            ) doReturn PicOrUrl(picBlob)
        }
        assertEquals("unknown", null, underTest.imageFor(archiveUri))
        assertEquals("no images", PicOrUrl(null, null), underTest.imageFor(archiveUri))
        assertEquals("external image", PicOrUrl(picUri), underTest.imageFor(archiveUri))
        assertEquals("embedded image", PicOrUrl(picBlob), underTest.imageFor(archiveUri))

        verify(db, times(4)).queryImage(argThat { dataLocation == archiveUri && subPath == "" })
    }

    @Test
    fun `coverArtOf cases`() {
        val embeddedPicId = 1L
        db.stub {
            on { findEmbedded(any()) } doReturn null doReturn Reference.Target(embeddedPicId)
        }

        assertEquals("unknown", null, underTest.coverArtUriOf(archivedModuleId))
        assertEquals(
            "known", Reference.Target(embeddedPicId), underTest.coverArtOf(archivedModuleId)
        )

        verify(db, times(2)).findEmbedded(archivedModuleId)
    }

    @Test
    fun `albumArtOf cached`() {
        db.stub {
            on { queryImageReferences(any()) } doReturn Reference.Target(picUri)
        }
        assertEquals(Reference.Target(picUri), underTest.albumArtOf(archivedModuleId, obj))

        verify(db).queryImageReferences(archivedModuleId)
    }

    @Test
    fun `albumArtOf archive with cached image`() {
        db.stub {
            on { queryImageReferences(any()) } doReturn null doReturn Reference.Target(picUri)
        }
        assertEquals(Reference.Target(picUri), underTest.albumArtOf(archivedModuleId, obj))

        inOrder(db) {
            verify(db).queryImageReferences(archivedModuleId)
            verify(db).queryImageReferences(archiveId)
        }
    }

    @Test(expected = IllegalArgumentException::class)
    fun `archiveArtOf not archive`() {
        underTest.archiveArtOf(archiveId, obj)
    }

    @Test
    fun `archiveArtOf cached archive with no images`() {
        db.stub {
            on { queryImageReferences(any()) } doReturn null doReturn Reference.Target(null, null)
        }
        assertNull(underTest.archiveArtOf(archivedModuleId, obj)?.toPictureUrl())

        verify(db).queryImageReferences(archivedModuleId)
        verify(db).queryImageReferences(archiveId)
    }

    @Test
    fun `archiveArtOf archive with no images`() {
        assertNull(underTest.archiveArtOf(archivedModuleId, obj)?.toPictureUrl())

        inOrder(db, obj) {
            verify(db).queryImageReferences(archivedModuleId)
            verify(db).queryImageReferences(archiveId)
            verify(db).listArchiveImages(archiveUri)
            verify(obj).getExtension(VfsExtensions.COVER_ART_URI)
            verify(db).setNoImage(archiveId)
        }
    }

    @Test
    fun `archiveArtOf archive with no images but with coverart`() {
        obj.stub {
            on { getExtension(VfsExtensions.COVER_ART_URI) } doReturn picUri
        }
        assertEquals(Reference.Target(picUri), underTest.archiveArtOf(archivedModuleId, obj))

        inOrder(db, obj) {
            verify(db).queryImageReferences(archivedModuleId)
            verify(db).queryImageReferences(archiveId)
            verify(db).listArchiveImages(archiveUri)
            verify(obj).getExtension(VfsExtensions.COVER_ART_URI)
            verify(db).addBoundImage(archiveId, picUri)
        }
    }

    @Test
    fun `archiveArtOf archive with sole image`() {
        db.stub {
            on { listArchiveImages(any()) } doReturn listOf(picPath)
        }
        assertEquals(
            Reference.Target(archivedPicId.fullLocation),
            underTest.archiveArtOf(archivedModuleId, obj)
        )

        inOrder(db) {
            verify(db).queryImageReferences(archivedModuleId)
            verify(db).queryImageReferences(archiveId)
            verify(db).listArchiveImages(archiveUri)
            verify(db).addInferred(archivedModuleId, archivedPicId.fullLocation)
        }
    }

    @Test
    fun `archiveArtOf archive with many images`() {
        db.stub {
            on { listArchiveImages(any()) } doReturn listOf(picPath, "sub/sub/image")
        }
        assertEquals(
            Reference.Target(archivedPicId.fullLocation),
            underTest.archiveArtOf(archivedModuleId, obj)
        )

        inOrder(db) {
            verify(db).queryImageReferences(archivedModuleId)
            verify(db).queryImageReferences(archiveId)
            verify(db).listArchiveImages(archiveUri)
            verify(db).addInferred(archivedModuleId, archivedPicId.fullLocation)
            // image is not set for archive - depends on module
        }
    }

    @Test
    fun `albumArtOf no images at all`() {
        assertNull(underTest.albumArtOf(archivedModuleId, obj))

        inOrder(db, obj) {
            verify(db).queryImageReferences(archivedModuleId)
            verify(db).queryImageReferences(archiveId)
            verify(db).listArchiveImages(archiveUri)
            verify(obj).getExtension(VfsExtensions.COVER_ART_URI)
            verify(db).setNoImage(archiveId)
        }
    }

    @Test
    fun `albumArtOf cached dir coverart`() {
        obj.stub {
            on { uri } doReturn moduleUri doReturn dirUri doReturn rootUri
            on { parent } doReturn obj
        }
        db.stub {
            on { queryImageReferences(any()) } doReturn null doReturn null doReturn Reference.Target(
                picUri
            )
        }
        assertEquals(Reference.Target(picUri), underTest.albumArtOf(moduleId, obj))
        inOrder(db, obj) {
            // module file
            verify(obj).uri
            verify(db).queryImageReferences(moduleId)
            verify(obj).getExtension(VfsExtensions.COVER_ART_URI)
            verify(obj).parent
            // dir
            verify(obj).uri
            verify(db).queryImageReferences(Identifier(dirUri))
            verify(obj).getExtension(VfsExtensions.COVER_ART_URI)
            verify(obj).parent
            // root
            verify(obj).uri
            verify(db).queryImageReferences(Identifier(rootUri))
        }
    }

    @Test
    fun `albumArtOf parent dir coverart for complex module`() {
        obj.stub {
            on { uri } doReturn moduleUri doReturn dirUri
            on { parent } doReturn obj
            on { getExtension(any()) } doReturn null doReturn picUri
        }
        assertEquals(Reference.Target(picUri), underTest.albumArtOf(complexModuleId, obj))
        inOrder(db, obj) {
            // check module itself
            verify(obj).uri
            verify(db).queryImageReferences(Identifier(moduleUri))
            verify(obj).getExtension(VfsExtensions.COVER_ART_URI)
            verify(obj).parent
            verify(obj).uri
            verify(db).queryImageReferences(Identifier(dirUri))
            verify(obj).getExtension(VfsExtensions.COVER_ART_URI)
            verify(db).findEmbedded(Identifier(picUri))
            verify(db).addInferred(complexModuleId, picUri)
            verify(db).addBoundImage(Identifier(dirUri), picUri)
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
