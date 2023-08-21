package app.zxtune.ui.about

import android.app.Application
import androidx.collection.SparseArrayCompat
import androidx.fragment.app.FragmentActivity
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModelProvider
import app.zxtune.PluginsProvider
import java.util.concurrent.Executors

// public for provider
class Model(app: Application) : AndroidViewModel(app) {
    // resId => [description]
    private val _data = MutableLiveData<SparseArrayCompat<ArrayList<String>>>()

    val data: LiveData<SparseArrayCompat<ArrayList<String>>>
        get() {
            if (_data.value == null) {
                asyncLoad()
            }
            return _data
        }

    private fun asyncLoad() = Executors.newCachedThreadPool().execute {
        _data.postValue(load())
    }

    private fun load() = SparseArrayCompat<ArrayList<String>>().apply {
        getApplication<Application>().contentResolver.query(
            PluginsProvider.getUri(), null, null, null, null
        )?.use { cursor ->
            while (cursor.moveToNext()) {
                val type = cursor.getInt(PluginsProvider.Columns.Type.ordinal)
                val descr = cursor.getString(PluginsProvider.Columns.Description.ordinal)
                getOrPut(type, ArrayList()).add(descr)
            }
        }
    }

    companion object {
        fun of(owner: FragmentActivity) = ViewModelProvider(
            owner, ViewModelProvider.AndroidViewModelFactory.getInstance(owner.application)
        )[Model::class.java]
    }
}

private fun <T> SparseArrayCompat<T>.getOrPut(key: Int, default: T) =
    putIfAbsent(key, default) ?: default
