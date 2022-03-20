package app.zxtune.fs.dbhelpers

import java.io.IOException

interface FetchCommand<T> {
    fun fetchFromCache(): T?

    @Throws(IOException::class)
    fun updateCache(): T
}
