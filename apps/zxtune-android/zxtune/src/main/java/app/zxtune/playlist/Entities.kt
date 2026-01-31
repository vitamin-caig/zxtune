package app.zxtune.playlist

import android.content.ContentValues
import androidx.annotation.VisibleForTesting
import app.zxtune.TimeStamp
import app.zxtune.core.Identifier

// TODO: see app.zxtune.fs.ocremix.Entities notes
typealias IdType = Long
typealias IdArrayType = LongArray

data class Playlist(val id: Id, val title: String) {
    data class Id(val value: IdType)

    data class Sorting(val by: By, val order: Order) {
        enum class By(val field: String) {
            TITLE("title"), AUTHOR("author"), DURATION("duration"),
        }

        enum class Order(val sql: String) {
            ASCENDING("ASC"), DESCENDING("DESC")
        }
    }

    companion object {
        val DEFAULT_ID = Id(0)
    }
}

data class Track(
    val id: Id,
    val meta: Metadata,
) {
    data class Id(val value: IdType)

    @JvmInline
    value class IdSet(val storage: IdArrayType) {
        init {
            assert(storage.size == storage.toSet().size)
        }

        constructor(size: Int, init: (Int) -> Id) : this(IdArrayType(size) {
            init(it).value
        })

        val size
            get() = storage.size

        @VisibleForTesting
        operator fun get(idx: Int) = Id(storage[idx])

        @VisibleForTesting
        fun forEach(block: (Id) -> Unit) = storage.forEach {
            block(Id(it))
        }

        companion object {
            @VisibleForTesting
            fun of(vararg ids: IdType) = IdSet(longArrayOf(*ids))

            @VisibleForTesting
            fun of(vararg ids: Id) = IdSet(ids.size) {
                ids[it]
            }
        }
    }

    data class Metadata(
        val location: Identifier,
        val title: String,
        val author: String,
        val duration: TimeStamp,
    )

    data class Statistics(val count: Long, val locations: Long, val duration: TimeStamp)
}

object IO {
    @VisibleForTesting
    enum class TrackColumns {
        ID, LOCATION, TITLE, AUTHOR, DURATION
    }

    fun Track.Metadata.toContentValues() = ContentValues().apply {
        put(TrackColumns.LOCATION.name, Converters.writeLocation(location))
        put(TrackColumns.TITLE.name, title)
        put(TrackColumns.AUTHOR.name, author)
        put(TrackColumns.DURATION.name, Converters.writeTimeStamp(duration))
    }

    fun ContentValues.toTrackMetadata() = Track.Metadata(
        location = Converters.readLocation(getAsString(TrackColumns.LOCATION.name)),
        title = getAsString(TrackColumns.TITLE.name),
        author = getAsString(TrackColumns.AUTHOR.name),
        duration = Converters.readTimeStamp(getAsLong(TrackColumns.DURATION.name)),
    )
}
