package app.zxtune

import android.os.Build

object Features {
    object StorageAccessFramework {
        const val REQUIRED_SDK = 30

        @JvmStatic
        fun isEnabled() = Build.VERSION.SDK_INT >= REQUIRED_SDK
    }

    object StorageManagerVolumes {
        const val REQUIRED_SDK = 24

        @JvmStatic
        fun isEnabled() = Build.VERSION.SDK_INT >= REQUIRED_SDK
    }
}
