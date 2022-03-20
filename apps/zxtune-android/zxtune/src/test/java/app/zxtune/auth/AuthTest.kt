package app.zxtune.auth

import android.content.ContentResolver
import android.content.Context
import android.content.SharedPreferences
import android.provider.Settings
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.*
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config
import org.robolectric.annotation.Implements

@RunWith(RobolectricTestRunner::class)
@Config(shadows = [ShadowSecureSettings::class])
class AuthTest {
    private val editor = mock<SharedPreferences.Editor>()
    private val prefs = mock<SharedPreferences>()
    private val ctx = mock<Context> {
        on { getSharedPreferences(any(), any()) } doReturn prefs
        on { contentResolver } doReturn mock()
    }

    @Before
    fun setUp() {
        clearInvocations(ctx)
        reset(editor, prefs, ShadowSecureSettings.getString)
        prefs.stub {
            on { edit() } doReturn editor
        }
    }

    @After
    fun tearDown() = verifyNoMoreInteractions(editor, prefs, ctx, ShadowSecureSettings.getString)

    @Test
    fun `stored v1 id`() {
        val userIdV1 = "stored v1 identifier"
        prefs.stub {
            on { getString("userid_v1", null) } doReturn userIdV1
        }
        Auth.getUserInfo(ctx).run {
            assertEquals(false, isInitial)
            assertEquals(userIdV1, identifier)
        }
        inOrder(ctx, prefs) {
            verify(ctx).getSharedPreferences("auth", Context.MODE_PRIVATE)
            verify(prefs).getString("userid_v2", null)
            verify(prefs).getString("userid_v1", null)
        }
    }

    @Test
    fun `stored v2 id`() {
        val userIdV2 = "stored v2 identifier"
        prefs.stub {
            on { getString("userid_v2", null) } doReturn userIdV2
        }
        Auth.getUserInfo(ctx).run {
            assertEquals(false, isInitial)
            assertEquals(userIdV2, identifier)
        }
        inOrder(ctx, prefs) {
            verify(ctx).getSharedPreferences("auth", Context.MODE_PRIVATE)
            verify(prefs).getString("userid_v2", null)
        }
    }

    @Test
    fun `generated v2 id`() {
        val generatedV2 = "generated v2 identifier"
        ShadowSecureSettings.getString.stub {
            on { invoke(any()) } doReturn generatedV2
        }
        Auth.getUserInfo(ctx).run {
            assertEquals(true, isInitial)
            assertEquals(generatedV2, identifier)
        }
        inOrder(editor, prefs, ctx, ShadowSecureSettings.getString) {
            verify(ctx).getSharedPreferences("auth", Context.MODE_PRIVATE)
            verify(prefs).getString("userid_v2", null)
            verify(prefs).getString("userid_v1", null)
            verify(ctx).contentResolver
            verify(ShadowSecureSettings.getString).invoke("android_id")
            verify(prefs).edit()
            verify(editor).putString("userid_v2", generatedV2)
            verify(editor).apply()
        }
    }
}

@Implements(Settings.Secure::class)
class ShadowSecureSettings {
    companion object {
        val getString = mock<(String) -> String>()

        @Suppress("unused")
        @JvmStatic
        fun getString(resolver: ContentResolver, name: String) = getString(name)
    }
}
