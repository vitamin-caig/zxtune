/**
 * @file
 * @brief Dialog fragment with playlist statistics
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.ui.playlist

import android.app.Dialog
import android.os.Bundle
import android.widget.ArrayAdapter
import androidx.appcompat.app.AlertDialog
import androidx.core.os.bundleOf
import androidx.fragment.app.DialogFragment
import app.zxtune.R
import app.zxtune.analytics.Analytics
import app.zxtune.playlist.ProviderClient
import app.zxtune.ui.utils.whenLifecycleStarted
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

class PlaylistStatisticsFragment : DialogFragment() {

    private lateinit var adapter: ArrayAdapter<String>

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val ctx = requireContext()
        adapter = ArrayAdapter<String>(ctx, android.R.layout.simple_list_item_1)

        /*
         When subscribing to lifecycle-aware components such as LiveData, never use viewLifecycleOwner
         as the LifecycleOwner in a DialogFragment that uses Dialog objects. Instead, use the
         DialogFragment itself.
         */
        whenLifecycleStarted { loadStatistics() }
        return AlertDialog.Builder(
            ctx
        ).setTitle(R.string.statistics).setAdapter(adapter, null).create()
    }

    private suspend fun loadStatistics() = adapter.run {
        clear()
        loadStatistics(getIdentifiers())?.let { stat ->
            val tracks = resources.getQuantityString(R.plurals.tracks, stat.total, stat.total)
            val duration = stat.duration.toString()
            addAll(
                getString(R.string.statistics_tracks) + ": " + tracks,
                getString(R.string.statistics_duration) + ": " + duration
            )
        } ?: add("Failed to load...")
    }

    private fun getIdentifiers() = arguments?.getLongArray(IDS_KEY)

    private suspend fun loadStatistics(ids: LongArray?) = withContext(Dispatchers.IO) {
        ProviderClient.create(requireContext()).statistics(ids)
    }

    companion object {
        private const val IDS_KEY = "ids"

        fun createInstance(ids: LongArray?): DialogFragment = PlaylistStatisticsFragment().apply {
            ids?.let {
                arguments = bundleOf(IDS_KEY to ids)
            }
        }.also {
            Analytics.sendPlaylistEvent(Analytics.PlaylistAction.STATISTICS, ids?.size ?: 0)
        }
    }
}
