package app.zxtune.fs.local

import android.os.Environment
import java.io.File

object Utils {
    internal fun isMountedStorage(state: String) =
        state == Environment.MEDIA_MOUNTED || state == Environment.MEDIA_MOUNTED_READ_ONLY

    @JvmStatic // for shadowing
    fun isAcessibleStorage(obj: File) = obj.exists() && obj.isDirectory
}
