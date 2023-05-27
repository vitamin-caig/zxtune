package app.zxtune.ui.utils

import android.content.Intent
import android.view.Menu
import android.view.MenuItem
import androidx.annotation.IdRes

internal fun Menu.item(@IdRes id: Int) = requireNotNull(findItem(id))

internal var MenuItem.enableIf: Intent?
    get() = intent
    set(data) {
        isEnabled = data != null
        intent = data
    }
