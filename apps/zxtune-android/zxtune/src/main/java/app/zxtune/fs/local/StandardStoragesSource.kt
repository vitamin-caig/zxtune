package app.zxtune.fs.local

import android.os.Build
import android.os.Environment

object StandardStoragesSource : StoragesSource {
    override fun getStorages(visitor: StoragesSource.Visitor) {
        if (Utils.isMountedStorage(Environment.getExternalStorageState())) {
            PLACES.map(Environment::getExternalStoragePublicDirectory)
                .filter(Utils::isAcessibleStorage)
                .forEach { obj ->
                    visitor.onStorage(obj, "")
                }
        }
    }

    private val PLACES = mutableListOf(
        Environment.DIRECTORY_MUSIC,
        Environment.DIRECTORY_PODCASTS
    ).apply {
        if (Build.VERSION.SDK_INT >= 29) {
            add(Environment.DIRECTORY_AUDIOBOOKS)
        }
    }
}
