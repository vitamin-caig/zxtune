package app.zxtune.fs

import app.zxtune.MainApplication
import kotlinx.coroutines.CoroutineStart
import kotlinx.coroutines.DelicateCoroutinesApi
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.GlobalScope
import kotlinx.coroutines.async
import kotlinx.coroutines.runBlocking

// TODO: temporary solution to move Vfs loading to IO thread
private object Loader {
    @JvmStatic
    val vfs
        get() = runBlocking {
            vfsLoader.await()
        }

    @JvmStatic
    val archive
        get() = runBlocking {
            archiveLoader.await()
        }

    @OptIn(DelicateCoroutinesApi::class)
    private val vfsLoader = GlobalScope.async(Dispatchers.IO, CoroutineStart.LAZY) {
        Vfs(MainApplication.getGlobalContext())
    }

    @OptIn(DelicateCoroutinesApi::class)
    private val archiveLoader = GlobalScope.async(Dispatchers.IO, CoroutineStart.LAZY) {
        VfsArchive(MainApplication.getGlobalContext())
    }
}
