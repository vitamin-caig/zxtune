package app.zxtune.fs.ocremix

import android.net.Uri
import androidx.annotation.StringRes
import app.zxtune.R

/**
 * + - folder, else file (terminal)
 *
 * +-Systems
 *   +-${System.title}
 *     +-${Games}
 *     +-${Albums}
 *     +-${Remixes}
 * +-Organizations
 *   +-${Organization.title}
 *     +-${Games}
 *     +-${Albums}
 *     +-${Remixes}
 * +-Games (=$Games)
 *   +-${Game.title}
 *     +-${Remixes}
 *     +-${Albums}
 *     -${Chiptune.filename} - optional
 * +-Albums (=$Albums)
 *   +-${Album.title} ($Album as node) - contains image
 *     -${AlbumTrack.title}
 * +-Remixes (=$Remixes)
 *   -${Remix.title}  - contains reference to primary game to get image
 *
 * ocremix:/ - root
 * ocremix:/Systems
 * ocremix:/Systems/${System.title}?system=${System.id}
 * ocremix:/Companies
 * ocremix:/Companies/${Company.title}
 * ../Games
 * ../Games/${Game.title}
 * ../Remixes
 * ../Albums
 * ../Albums/${Album.title}?album=${Album.id}
 * files:
 * ../${Remix.title}?remix=${Remix.id}&file=${Remix.file}[&game=${Game.Id}]
 * ../${Chiptune.filename}?chiptune=${Chiptune.id}&file=${Chiptune.file}[&game=${Game.Id}]
 * ../${AlbumTrack.title}?album=${Album.Id}&file=${AlbumTrack.file}
 *
 * Chains:
 * (Systems/${System}|Organizations/${Organization})/Games/${Game}/${Chiptune}
 *                                                  /Remixes/${Remix}
 *                                                  /Albums/${Album}/${AlbumTrack}
 *                                                        /Remixes/${Remix}
 *                                                        /Albums/${Album}/${AlbumTrack}
 *
 */

object Identifier {
    private const val SCHEME = "ocremix"

    enum class AggregateType(@StringRes val localized: Int) {
        Systems(R.string.vfs_ocremix_systems_name), //
        Organizations(R.string.vfs_ocremix_organizations_name), //
        Remixes(R.string.vfs_ocremix_remixes_name), //
        Albums(R.string.vfs_ocremix_albums_name), //
        Games(R.string.vfs_ocremix_games_name), //
    }

    sealed interface PathElement {
        val title: String
        val key: Pair<String, String>?
    }

    @JvmInline
    value class AggregateElement(val aggregate: AggregateType) : PathElement {
        override val title
            get() = aggregate.name

        override val key
            get() = null

        companion object {
            fun tryFind(segment: String) = AggregateType.entries.find { it.name == segment }?.let {
                AggregateElement(it)
            }
        }
    }

    @JvmInline
    value class SystemElement(val sys: System) : PathElement {
        override val title
            get() = sys.title
        override val key
            get() = SYSTEM_KEY to sys.id.value

        companion object {
            fun tryFind(uri: Uri, segment: String) = uri.getQueryParameter(SYSTEM_KEY)?.let {
                SystemElement(System(System.Id(it), segment))
            }
        }
    }

    @JvmInline
    value class OrganizationElement(val org: Organization) : PathElement {
        override val title
            get() = org.title
        override val key
            get() = ORGANIZATION_KEY to org.id.value

        companion object {
            fun tryFind(uri: Uri, segment: String) = uri.getQueryParameter(ORGANIZATION_KEY)?.let {
                OrganizationElement(Organization(Organization.Id(it), segment))
            }
        }
    }

    @JvmInline
    value class GameElement(val game: Game) : PathElement {
        override val title
            get() = game.title
        override val key
            get() = GAME_KEY to game.id.value

        companion object {
            fun tryFind(uri: Uri, segment: String) = uri.getQueryParameter(GAME_KEY)?.let {
                GameElement(Game(Game.Id(it), segment))
            }
        }
    }

    @JvmInline
    value class AlbumElement(val album: Album) : PathElement {
        override val title
            get() = album.title
        override val key
            get() = ALBUM_KEY to album.id.value

        companion object {
            fun tryFind(uri: Uri, segment: String) = uri.getQueryParameter(ALBUM_KEY)?.let {
                AlbumElement(Album(Album.Id(it), segment))
            }
        }
    }

    @JvmInline
    value class RemixElement(val remix: Remix) : PathElement {
        override val title
            get() = remix.title
        override val key
            get() = REMIX_KEY to remix.id.value

        companion object {
            fun tryFind(uri: Uri, segment: String) = uri.getQueryParameter(REMIX_KEY)?.let {
                RemixElement(Remix(Remix.Id(it), segment))
            }
        }
    }

    @JvmInline
    value class FileElement(val path: FilePath) : PathElement {
        override val title
            get() = path.displayName
        override val key
            get() = FILE_KEY to path.value

        companion object {
            fun tryFind(path: FilePath, lastPathSegment: String) =
                FileElement(path).takeIf { lastPathSegment == it.title }
        }
    }

    class PictureElement(val type: Type, val path: FilePath) : PathElement {
        enum class Type {
            Image, Thumb,
        }

        override val title
            get() = type.name
        override val key
            get() = FILE_KEY to path.value

        companion object {
            fun tryFind(path: FilePath, lastPathSegment: String) =
                Type.entries.find { it.name == lastPathSegment }?.let {
                    PictureElement(it, path)
                }
        }
    }

    // Root
    fun forRoot(): Uri.Builder = Uri.Builder().scheme(SCHEME)
    fun isFromRoot(uri: Uri) = SCHEME == uri.scheme

    fun forElementsChain(chain: Array<PathElement>): Uri = forRoot().apply {
        chain.forEach { entity ->
            appendPath(entity.title)
            entity.key?.let { (key, value) ->
                appendQueryParameter(key, value)
            }
        }
    }.build()

    fun findElementsChain(uri: Uri) = ArrayList<PathElement>().apply {
        val path = uri.pathSegments
        for (idx in 0..<path.size step 2) {
            val aggregateOrFilename = path[idx]
            if (idx == path.size - 1) {
                // shorthand
                val path = uri.filePath
                // TODO: use let since kotlin 2.2
                if (path != null) {
                    val asPath =
                        FileElement.tryFind(path, aggregateOrFilename) ?: PictureElement.tryFind(
                            path, aggregateOrFilename
                        )
                    if (asPath != null) {
                        add(asPath)
                        break
                    }
                    return emptyArray<PathElement>()
                }
            }
            val aggr =
                AggregateElement.tryFind(aggregateOrFilename) ?: return emptyArray<PathElement>()
            add(aggr)
            path.getOrNull(idx + 1)?.let { title ->
                val ent = when (aggr.aggregate) {
                    AggregateType.Systems -> SystemElement.tryFind(uri, title)
                    AggregateType.Organizations -> OrganizationElement.tryFind(uri, title)
                    AggregateType.Games -> GameElement.tryFind(uri, title)
                    AggregateType.Albums -> AlbumElement.tryFind(uri, title)
                    AggregateType.Remixes -> RemixElement.tryFind(uri, title)
                } ?: return emptyArray<PathElement>()
                add(ent)
            }
        }
    }.toTypedArray()

    fun forImage(path: FilePath) =
        forElementsChain(arrayOf(PictureElement(PictureElement.Type.Image, path)))

    fun forThumb(path: FilePath) =
        forElementsChain(arrayOf(PictureElement(PictureElement.Type.Thumb, path)))

    private const val SYSTEM_KEY = "sys"
    private const val ORGANIZATION_KEY = "org"
    private const val GAME_KEY = "game"
    private const val ALBUM_KEY = "album"
    private const val REMIX_KEY = "remix"
    private const val FILE_KEY = "file"

    private val Uri.filePath
        get() = getQueryParameter(FILE_KEY)?.let {
            FilePath(it)
        }
}
