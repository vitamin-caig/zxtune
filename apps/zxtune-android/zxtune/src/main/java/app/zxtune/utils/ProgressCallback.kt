package app.zxtune.utils

import androidx.annotation.Keep

@Keep
fun interface ProgressCallback {
    fun onProgressUpdate(done: Int, total: Int)
}
