package app.zxtune.utils

object StubProgressCallback : ProgressCallback {
    override fun onProgressUpdate(done: Int, total: Int) {}

    //TODO: remove
    @JvmStatic
    fun instance() = StubProgressCallback
}
