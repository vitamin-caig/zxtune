package app.zxtune

class Logger(val tag: String) {

    fun d(makeMessage: () -> String) {
        if (BuildConfig.DEBUG) {
            android.util.Log.d(tag, makeMessage())
        }
    }

    fun w(error: Throwable, makeMessage: () -> String) {
        if (BuildConfig.DEBUG) {
            android.util.Log.w(tag, makeMessage(), error)
        }
    }
}
