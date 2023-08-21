package app.zxtune.ui.about

import android.content.Context
import android.content.res.Configuration
import android.os.Build
import android.text.TextUtils
import android.util.DisplayMetrics

@Suppress("DEPRECATION")
internal class InfoBuilder constructor(private val context: Context) {
    private val strings = StringBuilder()

    fun buildOSInfo() = strings.run {
        addString("Android ${Build.VERSION.RELEASE} (API v${Build.VERSION.SDK_INT})")
        if (Build.VERSION.SDK_INT >= 21) {
            addString("${Build.MODEL} (${TextUtils.join("/", Build.SUPPORTED_ABIS)})")
        } else {
            addString("${Build.MODEL} (${Build.CPU_ABI}/${Build.CPU_ABI2})")
        }
    }

    fun buildConfigurationInfo() {
        val config = context.resources.configuration
        val metrics = context.resources.displayMetrics
        strings.run {
            addWord(getLayoutSize(config.screenLayout))
            addWord(getLayoutRatio(config.screenLayout))
            addWord(getOrientation(config.orientation))
            addWord(getScreenTrait("w", config.screenWidthDp))
            addWord(getScreenTrait("h", config.screenHeightDp))
            addWord(getScreenTrait("sw", config.smallestScreenWidthDp))
            addWord(getDensity(metrics.densityDpi))
            if (Build.VERSION.SDK_INT >= 24) {
                addWord(config.locales.toLanguageTags())
            } else {
                addWord(config.locale.toString())
            }
        }
    }

    val result
        get() = strings.toString()

    companion object {
        private fun getLayoutSize(layout: Int) =
            when (layout and Configuration.SCREENLAYOUT_SIZE_MASK) {
                Configuration.SCREENLAYOUT_SIZE_XLARGE -> "xlarge"
                Configuration.SCREENLAYOUT_SIZE_LARGE -> "large"
                Configuration.SCREENLAYOUT_SIZE_NORMAL -> "normal"
                Configuration.SCREENLAYOUT_SIZE_SMALL -> "small"
                else -> "size-undef"
            }

        private fun getLayoutRatio(layout: Int) =
            when (layout and Configuration.SCREENLAYOUT_LONG_MASK) {
                Configuration.SCREENLAYOUT_LONG_YES -> "long"
                Configuration.SCREENLAYOUT_LONG_NO -> "notlong"
                else -> "ratio-undef"
            }

        private fun getOrientation(orientation: Int) = when (orientation) {
            Configuration.ORIENTATION_LANDSCAPE -> "land"
            Configuration.ORIENTATION_PORTRAIT -> "port"
            Configuration.ORIENTATION_SQUARE -> "square"
            else -> "orientation-undef"
        }

        private fun getScreenTrait(trait: String, value: Int) = "${trait}${value}dp"

        private fun getDensity(density: Int) = when (density) {
            DisplayMetrics.DENSITY_LOW -> "ldpi"
            DisplayMetrics.DENSITY_MEDIUM -> "mdpi"
            DisplayMetrics.DENSITY_HIGH -> "hdpi"
            DisplayMetrics.DENSITY_XHIGH -> "xhdpi"
            480 -> "xxhdpi"
            640 -> "xxxhdpi"
            else -> "${density}dpi"
        }
    }
}

private fun StringBuilder.addString(s: String) = apply {
    append(s)
    append('\n')
}

private fun StringBuilder.addWord(s: String) = apply {
    append(s)
    append(' ')
}
