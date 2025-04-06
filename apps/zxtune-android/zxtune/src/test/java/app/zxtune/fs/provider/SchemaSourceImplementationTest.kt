package app.zxtune.fs.provider

import android.net.Uri
import app.zxtune.fs.*
import org.junit.Assert.assertEquals
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.*
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config
import org.robolectric.annotation.Implements
import java.io.File

@RunWith(RobolectricTestRunner::class)
@Config(shadows = [ShadowVfsArchive::class, ShadowVfs::class])
class SchemaSourceImplementationTest {

    private val iconDir = object : TestDir(1) {
        override fun getExtension(id: String) = if (id == VfsExtensions.ICON) {
            1
        } else {
            super.getExtension(id)
        }
    }
    private val feedDir = object : TestDir(2) {
        override fun getExtension(id: String) = if (id == VfsExtensions.FEED) {
            mock<Iterator<VfsFile>>()
        } else {
            super.getExtension(id)
        }
    }
    private val simpleDir = TestDir(3)

    private val simpleFile = TestFile(3, "Unused")
    private val cachedFile = TestFile(4, "Cached")
    private val badCacheFile = TestFile(5, "BadCache")

    private val underTest = SchemaSourceImplementation()

    init {
        ShadowVfsArchive.doGetModulesCount.stub {
            on { invoke(any()) } doAnswer {
                val args = it.getArgument<Array<Uri>>(0)
                Array(args.size) { idx ->
                    if (idx == 0) {
                        null
                    } else {
                        idx - 1
                    }
                }
            }
        }
        val cacheFile = mock<File> {
            on { isFile } doReturn true
        }
        ShadowVfs.doGetCache.stub {
            on { invoke(cachedFile) } doReturn cacheFile
            on { invoke(badCacheFile) } doReturn mock()
        }
    }

    @Test
    fun `resolve all`() {
        (underTest.resolved(iconDir) as Schema.Listing.Dir).run {
            assertEquals(iconDir.uri, uri)
            assertEquals(iconDir.name, name)
            assertEquals(iconDir.description, description)
            assertEquals(null, icon)
            assertEquals(false, hasFeed)
        }
        (underTest.resolved(feedDir) as Schema.Listing.Dir).run {
            assertEquals(feedDir.uri, uri)
            assertEquals(feedDir.name, name)
            assertEquals(feedDir.description, description)
            assertEquals(null, icon)
            assertEquals(true, hasFeed)
        }
        (underTest.resolved(simpleFile) as Schema.Listing.File).run {
            assertEquals(simpleFile.uri, uri)
            assertEquals(simpleFile.name, name)
            assertEquals(simpleFile.description, description)
            assertEquals(simpleFile.size, details)
            assertEquals(Schema.Listing.File.Type.UNKNOWN, type)
        }
        assertEquals(null, underTest.resolved(mock()))
    }

    @Test
    fun `parents all`() = underTest.parents(listOf(feedDir, simpleDir, iconDir)).let {
        assertEquals(3, it.size)
        it[0].run {
            assertEquals(feedDir.uri, uri)
            assertEquals(feedDir.name, name)
            assertEquals(null, icon)
        }
        it[1].run {
            assertEquals(simpleDir.uri, uri)
            assertEquals(simpleDir.name, name)
            assertEquals(null, icon)
        }
        it[2].run {
            assertEquals(iconDir.uri, uri)
            assertEquals(iconDir.name, name)
            assertEquals(1, icon)
        }
    }

    @Test
    fun `directories all`() = underTest.directories(listOf(simpleDir, iconDir, feedDir)).let {
        assertEquals(3, it.size)
        it[0].run {
            assertEquals(simpleDir.uri, uri)
            assertEquals(simpleDir.name, name)
            assertEquals(simpleDir.description, description)
            assertEquals(null, icon)
            assertEquals(false, hasFeed)
        }
        it[1].run {
            assertEquals(iconDir.uri, uri)
            assertEquals(iconDir.name, name)
            assertEquals(iconDir.description, description)
            assertEquals(1, icon)
            assertEquals(false, hasFeed)
        }
        it[2].run {
            assertEquals(feedDir.uri, uri)
            assertEquals(feedDir.name, name)
            assertEquals(feedDir.description, description)
            assertEquals(null, icon)
            assertEquals(true, hasFeed)
        }
    }

    @Test
    fun `files all`() = underTest.files(listOf(cachedFile, simpleFile, badCacheFile)).let {
        assertEquals(3, it.size)
        it[0].run {
            assertEquals(cachedFile.uri, uri)
            assertEquals(cachedFile.name, name)
            assertEquals(cachedFile.description, description)
            assertEquals(cachedFile.size, details)
            assertEquals(Schema.Listing.File.Type.UNKNOWN, type)
        }
        it[1].run {
            assertEquals(simpleFile.uri, uri)
            assertEquals(simpleFile.name, name)
            assertEquals(simpleFile.description, description)
            assertEquals(simpleFile.size, details)
            assertEquals(Schema.Listing.File.Type.UNSUPPORTED, type)
        }
        it[2].run {
            assertEquals(badCacheFile.uri, uri)
            assertEquals(badCacheFile.name, name)
            assertEquals(badCacheFile.description, description)
            assertEquals(badCacheFile.size, details)
            assertEquals(Schema.Listing.File.Type.TRACK, type)
        }
    }
}

@Implements(Vfs::class)
object ShadowVfs {
    val doGetCache = mock<(VfsFile) -> File?>()

    @JvmStatic
    fun getCache(file: VfsFile) = doGetCache(file)
}
