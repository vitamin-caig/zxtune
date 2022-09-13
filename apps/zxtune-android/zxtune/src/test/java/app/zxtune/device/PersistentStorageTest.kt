package app.zxtune.device

import android.content.Context
import android.net.Uri
import androidx.arch.core.executor.testing.InstantTaskExecutorRule
import androidx.core.net.toFile
import androidx.core.net.toUri
import androidx.documentfile.provider.DocumentFile
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.Observer
import app.zxtune.preferences.ProviderClient
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotEquals
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.ArgumentMatchers.anyString
import org.mockito.kotlin.any
import org.mockito.kotlin.doReturn
import org.mockito.kotlin.mock
import org.mockito.kotlin.stub
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config
import org.robolectric.annotation.Implementation
import org.robolectric.annotation.Implements
import java.io.File

@RunWith(RobolectricTestRunner::class)
@Config(shadows = [ShadowProviderClient::class, ShadowDocumentFile::class], sdk = [30])
class PersistentStorageTest {

    @get:Rule
    val instantTaskExecutorRule = InstantTaskExecutorRule()

    private val SUBDIR = "subdir"

    private val ctx = mock<Context>()
    private lateinit var storage: File
    private lateinit var subdir: File
    private lateinit var underTest: PersistentStorage
    private val observer = mock<Observer<PersistentStorage.State>>()

    @Before
    fun setUp() {
        storage = File(System.getProperty("java.io.tmpdir", "."), toString()).apply {
            require(mkdirs())
        }
        subdir = File(storage, SUBDIR)
        underTest = PersistentStorage(ctx)
        assertEquals(true, storage.exists())
        assertEquals(false, subdir.exists())
    }

    @After
    fun tearDown() {
        underTest.state.removeObserver(observer)
        storage.deleteRecursively()
    }

    @Test
    fun `nonexisting storage`() {
        val client = mock<ProviderClient> {
            on { getLive(any(), anyString()) } doReturn MutableLiveData(subdir.absolutePath)
        }
        ShadowProviderClient.factory.stub {
            on { invoke(any()) } doReturn client
        }
        underTest.state.observeForever(observer)
        requireNotNull(underTest.state.value).run {
            assertEquals(null, location)
            // avoid not mocked storagemanager request
            //assertNotEquals(null, defaultLocationHint)
        }
        // avoid not mocked storagemanager request
        //assertNotEquals(null, underTest.setupIntent.value)
        underTest.subdirectory("dir").run {
            assertEquals(null, tryGet())
            assertEquals(null, tryGet(createIfAbsent = true))
        }
    }

    @Test
    fun `existing storage`() {
        val client = mock<ProviderClient> {
            on { getLive(any(), anyString()) } doReturn MutableLiveData(storage.toUri().toString())
        }
        ShadowProviderClient.factory.stub {
            on { invoke(any()) } doReturn client
        }
        underTest.state.observeForever(observer)
        requireNotNull(underTest.state.value).run {
            assertNotEquals(null, location)
            // avoid not mocked storagemanager request
            //assertEquals(null, defaultLocationHint)
        }
        assertEquals(null, underTest.setupIntent.value)
        underTest.subdirectory(SUBDIR).run {
            assertEquals(null, tryGet())
            assertNotEquals(null, tryGet(createIfAbsent = true))
            assertNotEquals(null, tryGet())

            // TODO: test recreated on demand
            /*require(subdir.delete() && !subdir.exists())
            assertEquals(null, readonly)
            assertNotEquals(null, readwrite)
            assertNotEquals(null, readonly)*/
        }
    }
}

@Implements(ProviderClient.Companion::class)
class ShadowProviderClient {
    @Implementation
    fun create(ctx: Context) = factory(ctx)

    companion object {
        val factory = mock<(Context) -> ProviderClient>()
    }
}

@Implements(DocumentFile::class)
class ShadowDocumentFile {
    companion object {
        @Implementation
        @JvmStatic
        fun fromTreeUri(ctx: Context, uri: Uri) = DocumentFile.fromFile(uri.toFile())
    }
}
