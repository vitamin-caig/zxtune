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
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors

// public for provider
class Model @VisibleForTesting internal constructor(
    application: Application,
    private val client: ProviderClient,
    private val async: ExecutorService
) : AndroidViewModel(application) {

    private lateinit var state: MutableLiveData<List<Entry>>

    init {
        client.registerObserver {
            if (this::state.isInitialized) {
                loadAsync()
            }
        }
    }

    constructor(application: Application) : this(
        application,
        ProviderClient(application),
        Executors.newSingleThreadExecutor()
    )

    public override fun onCleared() = client.unregisterObserver()

    val items: LiveData<List<Entry>>
        get() {
            if (!this::state.isInitialized) {
                state = MutableLiveData()
                loadAsync()
            }
            return state
        }

    private fun loadAsync() = async.execute(this::load)

    private fun load() = client.query(null)?.use { cursor ->
        ArrayList<Entry>(cursor.count).apply {
            while (cursor.moveToNext()) {
                add(createItem(cursor))
            }
        }.let {
            state.postValue(it)
        }
    } ?: Unit

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
