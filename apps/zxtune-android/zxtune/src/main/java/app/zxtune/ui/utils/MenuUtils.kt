package app.zxtune.ui.utils

import android.content.Intent
import android.view.Menu
import android.view.MenuItem
import androidx.annotation.IdRes

internal fun Menu.item(@IdRes id: Int) = requireNotNull(findItem(id))

internal fun MenuItem.bindOnClick(cmd: (() -> Unit)?) {
    isEnabled = cmd != null
    setOnMenuItemClickListener(cmd?.run {
        MenuItem.OnMenuItemClickListener {
            invoke()
            true
        }
    })
}

internal fun MenuItem.bindIntent(intent: Intent?) {
    isEnabled = intent != null
    this.intent = intent
}
