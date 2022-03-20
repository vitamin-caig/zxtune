package app.zxtune.auth

import android.annotation.SuppressLint
import android.content.Context
import android.provider.Settings
import androidx.core.content.edit
import app.zxtune.Logger

object Auth {
    data class UserInfo(val identifier: String, val isInitial: Boolean)

    @JvmStatic
    fun getUserInfo(ctx: Context) =
        ctx.getSharedPreferences("auth", Context.MODE_PRIVATE).run {
            for (key in arrayOf(KEY_V2, KEY_V1)) {
                getString(key, null).takeIf { !it.isNullOrEmpty() }?.let { stored ->
                    LOG.d { "Use stored $key=$stored" }
                    return@run UserInfo(stored, false)
                }
            }
            val generated = getStableIdentifier(ctx).also {
                edit { putString(KEY_V2, it) }
                LOG.d { "Generate $KEY_V2=$it" }
            }
            UserInfo(generated, true)
        }

    private const val KEY_V1 = "userid_v1"
    private const val KEY_V2 = "userid_v2"

    private val LOG = Logger(Auth::class.java.name)

    @SuppressLint("HardwareIds")
    private fun getStableIdentifier(ctx: Context) =
        Settings.Secure.getString(ctx.contentResolver, Settings.Secure.ANDROID_ID)
}
