package app.zxtune.core.jni

import android.content.res.Resources
import androidx.annotation.RawRes
import androidx.test.platform.app.InstrumentationRegistry
import app.zxtune.core.Module
import app.zxtune.io.Io
import app.zxtune.test.R
import org.junit.Before
import org.junit.Test
import org.mockito.kotlin.argThat
import org.mockito.kotlin.eq
import org.mockito.kotlin.mock
import org.mockito.kotlin.verify
import org.mockito.kotlin.verifyNoMoreInteractions

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
}
