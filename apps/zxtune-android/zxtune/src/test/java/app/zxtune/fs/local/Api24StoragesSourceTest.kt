package app.zxtune.fs.local

import android.content.Context
import android.os.Environment
import android.os.UserHandle
import android.os.storage.StorageManager
import android.os.storage.StorageVolume
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.*
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config
import org.robolectric.shadows.StorageVolumeBuilder
import java.io.File

@RunWith(RobolectricTestRunner::class)
class Api24StoragesSourceTest {

    private val service = mock<StorageManager>()
    private val ctx = mock<Context> {
        on { getSystemService(StorageManager::class.java) } doReturn service
    }
    private lateinit var underTest: StoragesSource
    private val visitor = mock<StoragesSource.Visitor>()

    @Before
    fun setUp() {
        reset(service, visitor)
        underTest = Api24StoragesSource.create(ctx)
    }

    @After
    fun tearDown() {
        verify(service).storageVolumes
        verifyNoMoreInteractions(service, visitor)
    }

    @Test
    fun `no volumes`() = underTest.getStorages(visitor)

    @Test
    fun `mounted volumes`() {
        val vol1Path = File("/path/to/storage")
        val vol1Desc = "Volume 1"
        val vol2Path = File("/another")
        val vol2Desc = "Another volume"
        service.stub {
            on { storageVolumes } doAnswer {
                ArrayList<StorageVolume>().apply {
                    add(makeVolume(vol1Path, vol1Desc))
                    add(makeVolume(vol2Path, vol2Desc))
                }
            }
        }
        underTest.getStorages(visitor)
        inOrder(visitor) {
            verify(visitor).onStorage(vol1Path, vol1Desc)
            verify(visitor).onStorage(vol2Path, vol2Desc)
        }
    }

    @Test
    fun `different states`() {
        val states = Environment::class.java.fields.filter { it.name.startsWith("MEDIA_") }
            .map { it.get(null) as String }
        service.stub {
            on { storageVolumes } doAnswer {
                states.map {
                    makeVolume(File("/state/$it"), "State $it", it)
                }
            }
        }
        underTest.getStorages(visitor)
        verify(visitor).onStorage(File("/state/mounted"), "State mounted")
        verify(visitor).onStorage(File("/state/mounted_ro"), "State mounted_ro")
    }

    companion object {
        private fun makeVolume(
            path: File,
            desc: String,
            state: String = Environment.MEDIA_MOUNTED
        ) = StorageVolumeBuilder(
            path.absolutePath.replace('/', '_'),
            path,
            desc,
            UserHandle.getUserHandleForUid(-1),
            state
        ).build()
    }
}
