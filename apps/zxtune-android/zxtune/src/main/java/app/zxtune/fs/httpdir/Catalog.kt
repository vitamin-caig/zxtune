package app.zxtune.fs.httpdir

import android.content.Context
import app.zxtune.fs.http.MultisourceHttpProvider
import java.io.IOException

interface Catalog {

    interface DirVisitor {
        fun acceptDir(name: String, description: String)

        fun acceptFile(name: String, description: String, size: String)
    }

    /**
     * @param path    resource location
     * @param visitor result receiver
     */
    @Throws(IOException::class)
    fun parseDir(path: Path, visitor: DirVisitor)

    companion object {
        @JvmStatic
        fun create(ctx: Context, http: MultisourceHttpProvider, id: String): Catalog {
            val remote = RemoteCatalog(http)
            return create(ctx, remote, id)
        }

        @JvmStatic
        fun create(ctx: Context, remote: RemoteCatalog, id: String): Catalog =
            CachingCatalog(ctx, remote, id)
    }
}
