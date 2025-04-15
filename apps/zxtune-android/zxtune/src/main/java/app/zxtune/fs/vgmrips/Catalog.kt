package app.zxtune.fs.vgmrips

import android.content.Context
import androidx.core.util.Consumer
import app.zxtune.fs.http.MultisourceHttpProvider
import app.zxtune.utils.ProgressCallback
import java.io.IOException

interface Catalog {

    fun interface Visitor<T> : Consumer<T>

    interface Grouping {
        @Throws(IOException::class)
        fun query(visitor: Visitor<Group>)

        @Throws(IOException::class)
        fun queryPacks(id: Group.Id, visitor: Visitor<Pack>, progress: ProgressCallback)
    }

    fun companies(): Grouping
    fun composers(): Grouping
    fun chips(): Grouping
    fun systems(): Grouping

    @Throws(IOException::class)
    fun findPack(id: Pack.Id): Pack?

    @Throws(IOException::class)
    fun findRandomPack(visitor: Visitor<FilePath>): Pack?

    companion object {
        @JvmStatic
        fun create(context: Context, http: MultisourceHttpProvider): Catalog {
            val remote = RemoteCatalog(http)
            val db = Database(context)
            return CachingCatalog(remote, db)
        }
    }
}
