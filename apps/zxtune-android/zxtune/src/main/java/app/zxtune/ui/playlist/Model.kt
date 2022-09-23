package app.zxtune.ui.playlist

import android.app.Application
import android.database.Cursor
import androidx.annotation.VisibleForTesting
import androidx.fragment.app.Fragment
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModelProvider
import app.zxtune.TimeStamp.Companion.fromMilliseconds
import app.zxtune.core.Identifier.Companion.parse
import app.zxtune.playlist.Database
import app.zxtune.playlist.ProviderClient
import java.util.*
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors

private fun Entry.matches(filter: String) =
    title.contains(filter, true) || author.contains(filter, true)

private class ImmutableList<T>(private val inner : List<T>) : List<T> by inner

private fun <T> List<T>.toImmutable() : List<T> = (this as? ImmutableList<T>) ?: ImmutableList(this)

// public for provider
class Model @VisibleForTesting internal constructor(
    application: Application,
    private val client: ProviderClient,
    private val async: ExecutorService
) : AndroidViewModel(application) {

    class State(
        private val fullEntries: List<Entry> = emptyList(),
        val filter: String? = null,
        filtered: List<Entry>? = null,
    ) {
        private val filteredEntries = filtered?.toImmutable()

        val entries
            get() = filteredEntries ?: fullEntries

        fun withContent(newContent: List<Entry>) =
            if (filter.isNullOrBlank() || newContent.isEmpty()) {
                State(newContent)
            } else {
                State(newContent, filter, newContent.filter { it.matches(filter) })
            }

        fun withFilter(newFilter: String?) = when {
            newFilter.isNullOrBlank() || fullEntries.isEmpty() -> State(fullEntries)
            newFilter == filter -> State(fullEntries, filter, filteredEntries)
            filter != null && newFilter.startsWith(filter) -> State(
                fullEntries,
                newFilter,
                filteredEntries?.filter { it.matches(newFilter) })
            else -> State(fullEntries, newFilter, fullEntries.filter { it.matches(newFilter) })
        }

        @VisibleForTesting
        override fun equals(other: Any?) = true == (other as? State)?.let {
            it.fullEntries == fullEntries && it.filter == filter && it.filteredEntries == filteredEntries
        }
    }

    private lateinit var mutableState: MutableLiveData<State>

    init {
        client.registerObserver {
            if (this::mutableState.isInitialized) {
                loadAsync()
            }
        }
    }

    constructor(application: Application) : this(
        application,
        ProviderClient.create(application),
        Executors.newSingleThreadExecutor()
    )

    public override fun onCleared() = client.unregisterObserver()

    val state: LiveData<State>
        get() {
            if (!this::mutableState.isInitialized) {
                mutableState = MutableLiveData(State())
                loadAsync()
            }
            return mutableState
        }

    private fun loadAsync() = async.execute(this::load)

    private fun load() = client.query(null)?.use { cursor ->
        ArrayList<Entry>(cursor.count).apply {
            while (cursor.moveToNext()) {
                add(createItem(cursor))
            }
        }.let {
            mutableState.postValue(requireNotNull(mutableState.value).withContent(it))
        }
    } ?: Unit

    fun filter(rawFilter: String) = requireNotNull(mutableState.value).let { current ->
        val filter = rawFilter.trim()
        if (current.filter != filter) {
            async.execute {
                mutableState.postValue(current.withFilter(filter))
            }
        }
    }

    fun sort(by: ProviderClient.SortBy, order: ProviderClient.SortOrder) =
        async.execute { client.sort(by, order) }

    fun move(id: Long, delta: Int) = async.execute { client.move(id, delta) }

    fun deleteAll() = async.execute { client.deleteAll() }

    fun delete(ids: LongArray) = async.execute { client.delete(ids) }

    companion object {
        @JvmStatic
        fun of(owner: Fragment): Model = ViewModelProvider(
            owner,
            ViewModelProvider.AndroidViewModelFactory.getInstance(owner.requireActivity().application)
        )[Model::class.java]

        private fun createItem(cursor: Cursor) = Entry(
            cursor.getLong(Database.Tables.Playlist.Fields._id.ordinal),
            parse(cursor.getString(Database.Tables.Playlist.Fields.location.ordinal)),
            cursor.getString(Database.Tables.Playlist.Fields.title.ordinal),
            cursor.getString(Database.Tables.Playlist.Fields.author.ordinal),
            fromMilliseconds(cursor.getLong(Database.Tables.Playlist.Fields.duration.ordinal))
        )
    }
}
