package app.zxtune.utils

fun interface ProgressCallback {
    fun onProgressUpdate(done: Int, total: Int)
}
