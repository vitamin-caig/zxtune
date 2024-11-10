package app.zxtune.device

import android.content.Context
import android.net.Uri
import android.os.Environment
import android.os.storage.StorageManager
import android.os.storage.StorageVolume
import androidx.arch.core.executor.testing.InstantTaskExecutorRule
import androidx.core.net.toUri
import androidx.documentfile.provider.DocumentFile
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.Observer
import app.zxtune.Features
import app.zxtune.TestUtils.mockCollectorOf
import app.zxtune.fs.local.Identifier
import app.zxtune.fs.local.Utils
import app.zxtune.preferences.ProviderClient
import kotlinx.coroutines.test.TestScope
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.ArgumentMatchers.anyString
import org.mockito.kotlin.any
import org.mockito.kotlin.anyOrNull
import org.mockito.kotlin.argThat
import org.mockito.kotlin.doReturn
import org.mockito.kotlin.mock
import org.mockito.kotlin.reset
import org.mockito.kotlin.stub
import org.mockito.kotlin.verify
import org.mockito.kotlin.verifyNoMoreInteractions
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config
import org.robolectric.annotation.Implementation
import org.robolectric.annotation.Implements
import org.robolectric.shadows.ShadowEnvironment
import java.io.File

@RunWith(RobolectricTestRunner::class)
@Config(shadows = [ShadowDocumentFile::class, ShadowUtils::class])
class PersistentStorageTest {

    @get:Rule
    val instantTaskExecutorRule = InstantTaskExecutorRule()

    private val scope = TestScope()

    private val SUBDIR = "subdir"

    private val storageManager = mock<StorageManager>()
    private val ctx = mock<Context> {
        on { getSystemService(StorageManager::class.java) } doReturn storageManager
    }
    private val client = mock<ProviderClient>()
    private lateinit var storage: File
    private lateinit var subdir: File
    private val nonprimaryVolume = mock<StorageVolume>()
    private val primaryUnmountedVolume = mock<StorageVolume> {
        on { isPrimary } doReturn true
        on { state } doReturn Environment.MEDIA_UNMOUNTED
        on { getDescription(anyOrNull()) } doReturn "primary_unmounted"
    }
    private val primaryVolume = mock<StorageVolume> {
        on { isPrimary } doReturn true
        on { state } doReturn Environment.MEDIA_MOUNTED
        on { getDescription(anyOrNull()) } doReturn "primary_mounted"
    }
    private lateinit var underTest: PersistentStorage
    private val stateObserver = mock<Observer<PersistentStorage.State>>()
    private val intentObserver by lazy {
        scope.mockCollectorOf(underTest.setupIntent)
    }

    @Before
    fun setUp() {
        storage = File(System.getProperty("java.io.tmpdir", "."), toString()).apply {
            require(mkdirs())
        }
        ShadowEnvironment.setExternalStorageDirectory(storage.toPath())
        subdir = File(storage, SUBDIR)
        underTest = PersistentStorage(ctx, client)
        assertEquals(true, storage.exists())
        assertEquals(false, subdir.exists())
    }

    @After
    fun tearDown() {
        verifyNoMoreInteractions(intentObserver)
        underTest.state.removeObserver(stateObserver)
        storage.deleteRecursively()
        reset(storageManager)
    }

    @Test
    @Config(sdk = [Features.StorageAccessFramework.REQUIRED_SDK - 1])
    fun `nonexisting legacy storage`() {
        client.stub {
            on { getLive(any(), anyString()) } doReturn MutableLiveData(subdir.absolutePath)
        }
        underTest.state.observeForever(stateObserver)
        requireNotNull(underTest.state.value).run {
            assertEquals(subdir.toUri(), location?.uri)
            assertEquals(File(storage, "ZXTune").toUri(), defaultLocationHint)
        }
        assertEquals(true, subdir.exists())
        underTest.subdirectory("dir").run {
            assertEquals(null, tryGet())
            assertEquals(File(subdir, "dir").toUri(), tryGet(createIfAbsent = true)?.uri)
        }
        verify(intentObserver).invoke(null)
    }

    @Test
    @Config(sdk = [Features.StorageAccessFramework.REQUIRED_SDK - 1])
    fun `nonspecified legacy storage`() {
        client.stub {
            on { getLive(any(), anyString()) } doReturn MutableLiveData("")
        }
        underTest.state.observeForever(stateObserver)
        requireNotNull(underTest.state.value).run {
            assertEquals(defaultLocationHint, location?.uri)
            assertEquals(File(storage, "ZXTune").toUri(), defaultLocationHint)
        }
        underTest.subdirectory("dir").run {
            assertEquals(null, tryGet())
            assertEquals(File(storage, "ZXTune/dir").toUri(), tryGet(createIfAbsent = true)?.uri)
        }
        verify(intentObserver).invoke(null)
    }

    @Test
    @Config(sdk = [Features.StorageAccessFramework.REQUIRED_SDK - 1])
    fun `existing legacy storage`() {
        client.stub {
            on { getLive(any(), anyString()) } doReturn MutableLiveData(storage.absolutePath)
        }
        underTest.state.observeForever(stateObserver)
        requireNotNull(underTest.state.value).run {
            assertEquals(storage.toUri(), location?.uri)
            assertEquals(File(storage, "ZXTune").toUri(), defaultLocationHint)
        }
        underTest.subdirectory(SUBDIR).run {
            assertEquals(null, tryGet())
            assertEquals(subdir.toUri(), tryGet(createIfAbsent = true)?.uri)
            assertEquals(subdir.toUri(), tryGet()?.uri)
        }
        verify(intentObserver).invoke(null)
    }

    @Test
    @Config(sdk = [Features.StorageAccessFramework.REQUIRED_SDK])
    fun `nonexisting SAF storage`() {
        val storage = Identifier("notexist", "dir").documentUri
        storageManager.stub {
            on { storageVolumes } doReturn listOf(nonprimaryVolume)
        }
        client.stub {
            on { getLive(any(), anyString()) } doReturn MutableLiveData(storage.toString())
        }
        underTest.state.observeForever(stateObserver)
        requireNotNull(underTest.state.value).run {
            assertEquals(storage, location?.uri)
            assertEquals(Uri.EMPTY, defaultLocationHint)
        }
        underTest.subdirectory("dir").run {
            assertEquals(null, tryGet())
            // subdir does not exist, so its subdir cannot be created in RawDocumentFile
            assertEquals(null, tryGet(createIfAbsent = true)?.uri)
        }
        verify(intentObserver).invoke(argThat { data == Uri.EMPTY })
    }

    @Test
    @Config(sdk = [Features.StorageAccessFramework.REQUIRED_SDK])
    fun `nonspecified SAF storage`() {
        storageManager.stub {
            on { storageVolumes } doReturn listOf(primaryVolume)
        }
        client.stub {
            on { getLive(any(), anyString()) } doReturn MutableLiveData("")
        }
        val defaultLocation = Identifier("primary_mounted", "ZXTune").documentUri
        underTest.state.observeForever(stateObserver)
        requireNotNull(underTest.state.value).run {
            assertEquals(null, location?.uri)
            assertEquals(defaultLocation, defaultLocationHint)
        }
        underTest.subdirectory("dir").run {
            assertEquals(null, tryGet())
            assertEquals(null, tryGet(createIfAbsent = true)?.uri)
        }
        verify(intentObserver).invoke(argThat { data == defaultLocation })
    }

    @Test
    @Config(sdk = [Features.StorageAccessFramework.REQUIRED_SDK])
    fun `existing SAF storage`() {
        val storedLocation = Identifier("primary", "ZXTune").documentUri
        storageManager.stub {
            on { storageVolumes } doReturn listOf(primaryUnmountedVolume, primaryVolume)
        }
        client.stub {
            on { getLive(any(), anyString()) } doReturn MutableLiveData(storedLocation.toString())
        }
        underTest.state.observeForever(stateObserver)
        requireNotNull(underTest.state.value).run {
            assertEquals(storedLocation, location?.uri)
            assertEquals(Identifier("primary_mounted", "ZXTune").documentUri, defaultLocationHint)
        }
        underTest.subdirectory("dir").run {
            assertEquals(null, tryGet())
            // use the same object in createDir
            assertEquals(storedLocation, tryGet(createIfAbsent = true)?.uri)
        }
        verify(intentObserver).invoke(null)
    }
}

@Implements(DocumentFile::class)
class ShadowDocumentFile {
    companion object {
        @Implementation
        @JvmStatic
        fun fromTreeUri(ctx: Context, uri: Uri) = mock<DocumentFile> {
            val exists = !uri.toString().contains("notexist")
            on { exists() } doReturn exists
            on { getUri() } doReturn uri
            on { isDirectory } doReturn exists
            on { createDirectory(any()) } doReturn if (exists) mock else null
        }
    }
}

@Implements(Utils::class)
class ShadowUtils {
    companion object {
        @Implementation
        @JvmStatic
        fun rootId(obj: StorageVolume): String = obj.getDescription(null)
    }
}
