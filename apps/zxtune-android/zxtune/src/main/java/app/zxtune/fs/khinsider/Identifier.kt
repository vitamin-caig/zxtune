package app.zxtune.fs.khinsider

import android.net.Uri
import androidx.annotation.StringRes
import androidx.core.net.toUri
import app.zxtune.R

/*
 * khinsider:/
 * khinsider:/Random
 * khinsider:/{Top,Series,Platforms,Types,ByLetter,ByYear}/${Scope.Title}?scope=${Scope.Id}
 * ../${Album.Title}?album=${Album.Id}
 * ../${Track.Title}?track=${Track.Id}[&location=${TrackLocation}
 * khinsider:/{Image,Thumb}?location=${ImageLocation}
 */
data class Identifier(
    val category: Category? = null,
    val scope: Scope? = null,
    val album: Album? = null,
    val track: Track? = null,
    val location: FilePath? = null,
) {
    enum class Category(@StringRes val localized: Int = 0) {
        Random(R.string.vfs_khinsider_random_name), //
        Top(R.string.vfs_khinsider_top_name), //
        Series(R.string.vfs_khinsider_series_name), //
        Platforms(R.string.vfs_khinsider_platforms_name), //
        Types(R.string.vfs_khinsider_types_name), //
        AlbumsByLetter(R.string.vfs_khinsider_albumsletters_name), //
        AlbumsByYear(R.string.vfs_khinsider_albumsyears_name), //
        Image, Thumb;

        val isImage
            get() = this == Image || this == Thumb
    }

    fun toUri(): Uri = Uri.Builder().scheme(SCHEME).apply {
        category?.let {
            appendPath(it.name)
        }
        scope?.let {
            appendPath(it.title)
            appendQueryParameter(PARAM_SCOPE, it.id.value)
        }
        album?.let {
            appendPath(it.title)
            appendQueryParameter(PARAM_ALBUM, it.id.value)
        }
        track?.let {
            appendPath(it.title)
            appendQueryParameter(PARAM_TRACK, it.id.value)
        }
        location?.let {
            appendQueryParameter(PARAM_LOCATION, it.value.toString())
        }
    }.build()

    companion object {
        private const val SCHEME = "khinsider"
        private const val PARAM_SCOPE = "scope"
        private const val PARAM_ALBUM = "album"
        private const val PARAM_TRACK = "track"
        private const val PARAM_LOCATION = "location"

        fun find(uri: Uri) = if (SCHEME == uri.scheme) {
            val path = uri.pathSegments.iterator()
            val category = path.nextOrNull()?.let { category ->
                Category.entries.find { it.name == category }
            } ?: return Identifier()
            val location = uri.getQueryParameter(PARAM_LOCATION)?.let {
                FilePath(it.toUri())
            }
            val scope = if (Category.Random != category) {
                path.nextOrNull()?.let { title ->
                    uri.getQueryParameter(PARAM_SCOPE)?.let { id ->
                        Scope(Scope.Id(id), title)
                    }
                } ?: return if (category.isImage) {
                    location?.let {
                        Identifier(category, location = it)
                    }
                } else Identifier(category)
            } else null
            val album = path.nextOrNull()?.let { title ->
                uri.getQueryParameter(PARAM_ALBUM)?.let { id ->
                    Album(Album.Id(id), title)
                }
            } ?: return Identifier(category, scope)
            val track = path.nextOrNull()?.let { title ->
                uri.getQueryParameter(PARAM_TRACK)?.let { id ->
                    Track(Track.Id(id), title)
                }
            }
            Identifier(category, scope, album, track)
        } else null
    }
}

private fun <T> Iterator<T>.nextOrNull() = if (hasNext()) next() else null
