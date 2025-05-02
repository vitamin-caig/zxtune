package app.zxtune.coverart

import android.net.Uri
import androidx.room.Room
import androidx.test.core.app.ApplicationProvider
import app.zxtune.core.Identifier
import org.junit.After
import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class DatabaseTest {
    private lateinit var underTest: Database
    private val blob1 = byteArrayOf(1, 2, 3)
    private val blob2 = byteArrayOf(3, 4, 5, 6)
    private val blob3 = byteArrayOf(4, 5, 6, 7, 8)
    private val archiveUri = Uri.parse("schema://archive/path")
    private val blob1Id = Identifier(archiveUri, "blob1")
    private val blob2Id = Identifier(archiveUri, "blob2")
    private val imageId = Identifier.parse("schema://image/path")
    private val folderId = Identifier.parse("schema://some/folder")
    private val absentId = Identifier.parse("schema://absent/item")

    @Before
    fun setUp() {
        underTest = Database(
            Room.inMemoryDatabaseBuilder(
                ApplicationProvider.getApplicationContext(), DatabaseDelegate::class.java
            ).allowMainThreadQueries().build()
        )
    }

    @After
    fun tearDown() {
        underTest.close()
    }

    @Test
    fun `image hash test`() {
        assertEquals(0, ImageHash.of(ByteArray(0)))
        assertEquals(0x69eccf4553d99e8b, ImageHash.of("A".toByteArray()))
        assertEquals(0x7480ab600854897f, ImageHash.of(byteArrayOf(0, 1, 2, 3, 4, 5)))
    }

    @Test
    fun `integration test`() = with(underTest) {
        addImage(blob1Id, blob1)
        addImage(blob2Id, blob2)
        addImage(imageId, blob3)
        addBoundImage(folderId, imageId.dataLocation)

        assertArrayEquals(arrayOf("blob1", "blob2"), listArchiveImages(archiveUri).toTypedArray())
        assertArrayEquals(arrayOf(""), listArchiveImages(imageId.dataLocation).toTypedArray())
        assertTrue(listArchiveImages(folderId.dataLocation).isEmpty())
        assertTrue(listArchiveImages(absentId.dataLocation).isEmpty())

        assertArrayEquals(blob1, queryImage(blob1Id)?.pic?.data)
        assertEquals(ImageHash.of(blob1), findEmbedded(blob1Id)?.pic)

        assertArrayEquals(blob2, queryImage(blob2Id)?.pic?.data)
        assertEquals(ImageHash.of(blob2), findEmbedded(blob2Id)?.pic)

        assertArrayEquals(blob3, queryImage(imageId)?.pic?.data)
        assertEquals(ImageHash.of(blob3), findEmbedded(imageId)?.pic)

        assertEquals(imageId.dataLocation, queryImage(folderId)?.url)
        assertEquals(null, findEmbedded(folderId))

        assertEquals(null, queryImage(absentId))
        assertEquals(null, findEmbedded(absentId))

        val archiveId = Identifier(archiveUri)
        assertEquals(null, queryImage(archiveId))
        assertEquals(null, findEmbedded(archiveId))

        setNoImage(archiveId)
        requireNotNull(queryImage(archiveId)).run {
            assertEquals(null, pic)
            assertEquals(null, url)
        }
        requireNotNull(findEmbedded(archiveId)).run {
            assertEquals(null, pic)
            assertEquals(null, url)
        }

        remove(archiveUri)
        assertTrue(listArchiveImages(archiveUri).isEmpty())
        assertArrayEquals(arrayOf(""), listArchiveImages(imageId.dataLocation).toTypedArray())
    }
}
