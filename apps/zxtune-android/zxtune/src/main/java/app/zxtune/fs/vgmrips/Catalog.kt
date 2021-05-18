package app.zxtune.fs.vgmrips

import android.content.Context
import app.zxtune.fs.http.MultisourceHttpProvider
import app.zxtune.utils.ProgressCallback
import java.io.IOException

interface Catalog {

    interface Visitor<T> {
        fun accept(obj: T)
    }

    interface Grouping {
        @Throws(IOException::class)
        fun query(visitor: Visitor<Group>)

        @Throws(IOException::class)
        fun queryPacks(id: String, visitor: Visitor<Pack>, progress: ProgressCallback)
    }

    fun companies(): Grouping
    fun composers(): Grouping
    fun chips(): Grouping
    fun systems(): Grouping

    @Throws(IOException::class)
    fun findPack(id: String, visitor: Visitor<Track>): Pack?

    @Throws(IOException::class)
    fun findRandomPack(visitor: Visitor<Track>): Pack?

    companion object {
        @JvmStatic
        fun create(context: Context, http: MultisourceHttpProvider): Catalog {
            val remote = RemoteCatalog(http)
            val db = Database(context)
            return CachingCatalog(remote, db)
        }
    }
}
