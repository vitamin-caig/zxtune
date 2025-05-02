package app.zxtune.fs.provider

import android.net.Uri
import app.zxtune.fs.*
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.*
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class SearchOperationTest {

    private val uri = Uri.parse("schema://host")

    private val query = "Query"

    private val resolver = mock<Resolver>()
    private val schema = mock<SchemaSource> {
        on { files(any()) } doAnswer {
            it.getArgument<List<VfsFile>>(0).map { o -> makeFileObject(o) }
        }
    }
    private val callback = mock<AsyncQueryOperation.Callback>()

    @Before
    fun setUp() = reset(resolver, callback)

    @After
    fun tearDown() = verifyNoMoreInteractions(resolver, schema, callback)

    @Test
    fun `no resolved dir`() = with(SearchOperation(uri, resolver, schema, callback, query)) {
        assertEquals(null, status())
        call().run {
            assertEquals(0, count)
        }
        verify(resolver).resolve(uri)
        verify(schema).files(argThat { isEmpty() })
        assertEquals(null, status())
    }

    @Test
    fun `simple dir`() {
        val nestedFileMatched = object : TestFile(1, "Unused") {
            override val description = "subquery match"
        }
        val nestedObjectMatched = makeFileObject(nestedFileMatched)
        val nestedFileNotMatched = TestFile(2, "Query")
        val nestedDir = object : TestDir(3) {
            override fun enumerate(visitor: VfsDir.Visitor) {
                visitor.onFile(nestedFileNotMatched)
                visitor.onFile(nestedFileMatched)
            }
        }
        val rootFileMatched = object : TestFile(4, "Unused") {
            override val name = "Query"
        }
        val rootObjectMatched = makeFileObject(rootFileMatched)
        val rootFileNotMatched = TestFile(5, "Query")
        val rootDir = object : TestDir(6) {
            override fun enumerate(visitor: VfsDir.Visitor) {
                visitor.onFile(rootFileMatched)
                visitor.onDir(nestedDir)
                visitor.onFile(rootFileNotMatched)
            }
        }
        resolver.stub {
            on { resolve(any()) } doReturn rootDir
        }
        with(SearchOperation(uri, resolver, schema, callback, query)) {
            assertEquals(null, status())
            call().run {
                assertEquals(2, count)
                moveToNext()
                assertEquals(rootObjectMatched, Schema.Object.parse(this) as Schema.Listing.File)
                moveToNext()
                assertEquals(nestedObjectMatched, Schema.Object.parse(this) as Schema.Listing.File)
            }
            assertEquals(null, status())
        }
        inOrder(resolver, callback, schema) {
            verify(resolver).resolve(uri)
            verify(callback).checkForCancel()
            verify(callback).onStatusChanged()
            verify(callback, times(3)).checkForCancel()
            verify(callback).onStatusChanged()
            verify(schema).files(arrayListOf(rootFileMatched, nestedFileMatched))
        }
    }

    @Test
    fun `search engine and partial status`() {
        val searchEngine = mock<VfsExtensions.SearchEngine>()
        val root = object : TestDir(1) {
            override fun getExtension(id: String): Any? = if (id == VfsExtensions.SEARCH_ENGINE) {
                searchEngine
            } else {
                super.getExtension(id)
            }
        }
        val file1 = TestFile(2, "Unused")
        val object1 = makeFileObject(file1)
        val file2 = TestFile(3, "Unused")
        val object2 = makeFileObject(file2)
        val file3 = TestFile(4, "Unused")
        val object3 = makeFileObject(file3)
        resolver.stub {
            on { resolve(any()) } doReturn root
        }
        with(SearchOperation(uri, resolver, schema, callback, query)) {
            assertEquals(null, status())
            searchEngine.stub {
                on { find(any(), any()) } doAnswer {
                    it.getArgument<VfsExtensions.SearchEngine.Visitor>(1).run {
                        onFile(file1)
                        onFile(file2)
                        (this@with).status()!!.run {
                            assertEquals(2, count)
                            moveToNext()
                            assertEquals(object1, Schema.Object.parse(this) as Schema.Listing.File)
                            moveToNext()
                            assertEquals(object2, Schema.Object.parse(this) as Schema.Listing.File)
                        }
                        onFile(file3)
                    }
                }
            }
            call().run {
                assertEquals(1, count)
                moveToNext()
                assertEquals(object3, Schema.Object.parse(this) as Schema.Listing.File)
            }
            assertEquals(null, status())
        }
        inOrder(resolver, searchEngine, callback, schema) {
            verify(resolver).resolve(uri)
            verify(searchEngine).find(eq(query.lowercase()), any())
            verify(callback, times(2)).onStatusChanged()
            verify(schema).files(listOf(file1, file2))
            verify(callback).onStatusChanged()
            verify(schema).files(listOf(file3))
        }
    }

    companion object {
        private fun makeFileObject(file: VfsFile) = with(file) {
            Schema.Listing.File(uri, name, description, null, size, Schema.Listing.File.Type.UNKNOWN)
        }
    }
}
