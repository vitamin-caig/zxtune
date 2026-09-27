package app.zxtune.ui.utils

import android.app.Activity
import android.content.res.Configuration
import android.graphics.Color
import android.os.Build
import android.view.View
import androidx.core.graphics.Insets
import androidx.core.view.ViewCompat
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat

// The cutout is not a bar, but overlaps them and shares the mask
private val BARS = WindowInsetsCompat.Type.systemBars() or
    WindowInsetsCompat.Type.displayCutout()

/**
 * Android 15+ forces this for anything targeting SDK 35 and above, so opting in explicitly only
 * serves the versions below that - but it keeps a single insets code path on all of them.
 *
 * Nothing pads the content by default: AppCompat opts its sub-decor out of the legacy
 * `fitsSystemWindows` padding (ViewUtils.makeOptionalFitsSystemWindows, from
 * AppCompatDelegateImpl.createSubDecor), so a theme change alone is not enough.
 */
fun Activity.applyEdgeToEdge() {
    // Below API 21 the platform insets the window itself and there is no decor flag to
    // clear, so opting out keeps the framework padding from being doubled up by ours
    if (Build.VERSION.SDK_INT < 21) {
        return
    }

    WindowCompat.setDecorFitsSystemWindows(window, false)
    if (Build.VERSION.SDK_INT < 29) {
        @Suppress("DEPRECATION")
        run {
            // No longer settable from API 29 on, where the bars are always transparent
            window.statusBarColor = Color.TRANSPARENT
            window.navigationBarColor = Color.TRANSPARENT
        }
    }

    // The bars sit on the app background instead of a colorPrimaryDark strip, so the icons
    // have to follow the theme rather than the window
    val isLight = resources.configuration.uiMode and Configuration.UI_MODE_NIGHT_MASK !=
        Configuration.UI_MODE_NIGHT_YES
    WindowInsetsControllerCompat(window, window.decorView).apply {
        isAppearanceLightStatusBars = isLight
        isAppearanceLightNavigationBars = isLight
    }
}

/**
 * The bottom is taken ignoring visibility, so the content does not jump around when the bars
 * slide in and out. [WindowInsetsCompat.Type.ime] is only reported from API 30 on - below that
 * the platform resizes the window for the keyboard itself, so a single maximum covers both.
 *
 * Insets are left unconsumed so the children keep receiving them.
 */
fun View.padWithSystemBars() {
    // Insets arrive repeatedly (rotation, keyboard, sliding bars) and are relative to this
    // view only, so the original padding is added back every time instead of the result of
    // the previous one
    val initial = Insets.of(paddingLeft, paddingTop, paddingRight, paddingBottom)
    ViewCompat.setOnApplyWindowInsetsListener(this) { view, insets ->
        val padding = systemBarPadding(insets)
        view.setPadding(
            initial.left + padding.left,
            initial.top + padding.top,
            initial.right + padding.right,
            initial.bottom + padding.bottom,
        )
        insets
    }
}

internal fun systemBarPadding(insets: WindowInsetsCompat): Insets {
    val bars = insets.getInsets(BARS)
    val bottom = maxOf(
        insets.getInsetsIgnoringVisibility(WindowInsetsCompat.Type.navigationBars()).bottom,
        insets.getInsets(WindowInsetsCompat.Type.ime()).bottom,
    )
    return Insets.of(bars.left, bars.top, bars.right, bottom)
}
