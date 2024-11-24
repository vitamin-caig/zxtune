/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.ui.playlist

import android.app.Application
import android.app.Dialog
import android.content.Context
import android.content.DialogInterface
import android.os.Bundle
import android.view.inputmethod.EditorInfo
import android.widget.Button
import android.widget.EditText
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.core.widget.doOnTextChanged
import androidx.fragment.app.DialogFragment
import androidx.fragment.app.activityViewModels
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import app.zxtune.R
import app.zxtune.playlist.ProviderClient
import app.zxtune.ui.utils.FragmentLongArrayProperty
import app.zxtune.ui.utils.whenLifecycleStarted
import kotlinx.coroutines.MainScope
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.flow.flow
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.launch

class PlaylistSaveFragment : DialogFragment() {
    private var ids by FragmentLongArrayProperty
    private val model by activityViewModels<SaveFragmentModel>()

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val ctx = requireContext()
        return AlertDialog.Builder(ctx).setTitle(R.string.save).setView(createEditText(ctx))
            .setPositiveButton(R.string.save) { _, _ ->
                model.save(ids)
            }.create().apply {
                whenLifecycleStarted {
                    trackButton(getButton(DialogInterface.BUTTON_POSITIVE))
                }
            }
    }

    private suspend fun trackButton(okButton: Button) = model.state.collect { state ->
        state.apply(okButton)
    }

    private fun createEditText(ctx: Context) = EditText(ctx).apply {
        imeOptions = EditorInfo.IME_FLAG_NO_EXTRACT_UI
        setSingleLine()

        doOnTextChanged { text, _, _, _ ->
            model.setName(text?.toString() ?: "")
        }
    }

    companion object {
        fun createInstance(ids: LongArray?) = PlaylistSaveFragment().apply {
            this.ids = ids
        }
    }
}

class SaveFragmentModel(application: Application) : AndroidViewModel(application) {
    private val client = ProviderClient.create(application)

    private val usedNames = flow {
        client.getSavedPlaylists()?.let {
            emit(it.keys)
        }
    }
    private val enteredName = MutableStateFlow("")

    fun setName(name: String) {
        enteredName.value = name
    }

    sealed interface ButtonState {
        fun apply(but: Button)

        data object Disabled : ButtonState {
            override fun apply(but: Button) = with(but) {
                isEnabled = false
            }
        }

        data object DoSave : ButtonState {
            override fun apply(but: Button) = with(but) {
                isEnabled = true
                setText(R.string.save)
            }
        }

        data object DoOverwrite : ButtonState {
            override fun apply(but: Button) = with(but) {
                isEnabled = true
                setText(R.string.overwrite)
            }
        }
    }

    val state: Flow<ButtonState> = combine(usedNames, enteredName) { used, entered ->
        when {
            entered.isEmpty() -> ButtonState.Disabled
            used.contains(entered) -> ButtonState.DoOverwrite
            else -> ButtonState.DoSave
        }
    }.stateIn(viewModelScope, SharingStarted.WhileSubscribed(500), ButtonState.Disabled)

    fun save(ids: LongArray?) = MainScope().launch {
        saveBlocking(ids)
    }

    private suspend fun saveBlocking(ids: LongArray?) {
        val ctx = getApplication<Application>()
        val showToast = { txt: String, duration: Int ->
            Toast.makeText(ctx, txt, duration).show()
        }

        runCatching {
            val name =
                requireNotNull(enteredName.value.takeUnless { it.isEmpty() }) { "Invalid name" }
            client.savePlaylist(name, ids)
        }.onFailure {
            showToast(ctx.getString(R.string.save_failed, it.message), Toast.LENGTH_LONG)
        }.onSuccess {
            showToast(ctx.getString(R.string.saved), Toast.LENGTH_SHORT)
        }
    }
}
