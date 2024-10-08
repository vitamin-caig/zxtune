package app.zxtune.core.jni

import android.content.res.Resources
import androidx.annotation.RawRes
import androidx.test.platform.app.InstrumentationRegistry
import app.zxtune.core.Module
import app.zxtune.core.ModuleAttributes
import app.zxtune.core.ResolvingException
import app.zxtune.io.Io
import app.zxtune.test.R
import app.zxtune.use
import org.junit.Assert
import org.junit.Before
import org.junit.Test
import org.mockito.kotlin.argThat
import org.mockito.kotlin.eq
import org.mockito.kotlin.mock
import org.mockito.kotlin.times
import org.mockito.kotlin.verify
import org.mockito.kotlin.verifyNoInteractions
import org.mockito.kotlin.verifyNoMoreInteractions
import java.nio.ByteBuffer

class JniApiTest {

    private lateinit var resources: Resources
    private lateinit var api: JniApi

    private fun readFile(@RawRes id: Int) = resources.openRawResource(id).let {
        Io.readFrom(it, it.available().toLong())
    }

    @Before
    fun setUp() {
        resources = InstrumentationRegistry.getInstrumentation().context.resources
        api = JniApi()
    }

    @Test
    fun testPicturesInArchive() {
        val cb = mock<DetectCallback>()
        val data = readFile(R.raw.archive_pics)
        api.detectModules(data, cb, null)
        verify(cb).onPicture(eq("in/folder.png"), argThat { size == 5873 })
        verify(cb).onPicture(eq("simple.jpg"), argThat { size == 46835 })
        verifyNoMoreInteractions(cb)
    }

    @Test
    fun testPictureInModules() {
        checkHasPicture(api.loadModule(readFile(R.raw.coverart_ogg), ""), 5873)
        checkHasPicture(api.loadModule(readFile(R.raw.coverart_mp3), ""), 5873)
        checkHasPicture(api.loadModule(readFile(R.raw.coverart_flac), ""), 46835)
    }

    private fun checkHasPicture(module: Module, size: Int) = module.use {
        requireNotNull(module.getProperty(ModuleAttributes.PICTURE, null)).run {
            Assert.assertEquals(size, size)
        }
    }

    @Test
    fun testModuleDataUnresolved() {
        val cb = mock<DataCallback>()
        val multitrack = readFile(R.raw.multitrack)
        Assert.assertThrows(ResolvingException::class.java) {
            api.loadModuleData(multitrack, "+unGZIP", cb)
        }
        val packed = readFile(R.raw.gzipped)
        // see commit related changes
        Assert.assertThrows(ResolvingException::class.java) {
            api.loadModuleData(packed, "#1", cb)
        }
        verifyNoInteractions(cb)
    }

    @Test
    fun testModuleDataInArchive() {
        val cb = mock<DataCallback>()
        val data = readFile(R.raw.archive)
        api.loadModuleData(data, "auricom.pt3", cb)
        api.loadModuleData(data, "coop-Jeffie/bass sorrow.pt3", cb)
        verify(cb).onData(argThat { isDirect && !isReadOnly && remaining() == 2253 })
        verify(cb).onData(argThat { isDirect && !isReadOnly && remaining() == 6948 })
        verifyNoMoreInteractions(cb)
    }

    @Test
    fun testModuleDataInCompressed() {
        val cb = mock<DataCallback>()
        val data = readFile(R.raw.gzipped)
        api.loadModuleData(data, "+unGZIP", cb)
        verify(cb).onData(argThat { isDirect && !isReadOnly && remaining() == 83216 })
        verifyNoMoreInteractions(cb)
    }

    @Test
    fun testModuleDataInMultitrack() {
        val cb = mock<DataCallback>()
        val data = readFile(R.raw.multitrack)
        api.loadModuleData(data, "#1", cb)
        api.loadModuleData(data, "#2", cb)
        verify(cb, times(2)).onData(argThat { isDirect && !isReadOnly && remaining() == 10590 })
        verifyNoMoreInteractions(cb)
    }
}
