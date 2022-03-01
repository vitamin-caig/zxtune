package app.zxtune.ui

import androidx.recyclerview.widget.AsyncDifferConfig
import org.junit.rules.TestWatcher
import org.junit.runner.Description
import java.util.concurrent.Executor

class AsyncDifferInMainThreadRule : TestWatcher() {

    private val field =
        AsyncDifferConfig.Builder::class.java.getDeclaredField("sDiffExecutor").apply {
            isAccessible = true
        }

    private var oldValue: Any? = null

    override fun starting(description: Description) {
        super.starting(description)
        oldValue = field.get(null)
        field.set(null, Executor { it.run() })
    }

    override fun finished(description: Description?) {
        super.finished(description)
        field.set(null, oldValue)
    }
}
