package app.zxtune.coverart

import android.widget.ImageView

fun ImageView.withFadeout(cmd: (ImageView) -> Unit) {
    animate().alpha(0f).withEndAction {
        cmd(this)
        animate().alpha(1.0f)
    }
}
