package app.zxtune.ui

import android.os.Bundle
import android.view.View
import android.widget.SimpleExpandableListAdapter
import androidx.collection.ArrayMap
import androidx.collection.SparseArrayCompat
import androidx.databinding.DataBindingUtil
import androidx.fragment.app.DialogFragment
import androidx.fragment.app.FragmentActivity
import app.zxtune.BuildConfig
import app.zxtune.PluginsProvider
import app.zxtune.R
import app.zxtune.databinding.AboutBinding
import app.zxtune.ui.about.InfoBuilder
import app.zxtune.ui.about.Model

class AboutFragment : DialogFragment(R.layout.about) {
    companion object {
        fun show(activity: FragmentActivity) =
            AboutFragment().show(activity.supportFragmentManager, null)
    }

    private lateinit var binding: AboutBinding

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        binding = requireNotNull(DataBindingUtil.bind(view))
        binding.run {
            aboutSystem.text = getSystemInfo()
            aboutPager.adapter = ViewPagerAdapter(aboutPager)
        }

        Model.of(requireActivity()).data.observe(this, this::fillPlugins)
    }

    override fun onCreateDialog(savedInstanceState: Bundle?) =
        super.onCreateDialog(savedInstanceState).apply {
            setTitle(getAppInfo())
        }

    private fun getAppInfo() =
        "${getString(app.zxtune.R.string.app_name)} b${BuildConfig.VERSION_CODE} (${BuildConfig.FLAVOR})"

    private fun getSystemInfo() = InfoBuilder(requireContext()).apply {
        buildOSInfo()
        buildConfigurationInfo()
    }.result

    private fun fillPlugins(data: SparseArrayCompat<ArrayList<String>>) {
        val keyType = "type"
        val keyDescr = "descr"
        val groups = ArrayList<ArrayMap<String, String>>(data.size())
        val children = ArrayList<ArrayList<ArrayMap<String, String>>>()
        for (idx in 0 until data.size()) {
            val type = data.keyAt(idx)
            groups.add(makeRowMap(keyType, getPluginTypeString(type)))
            val child = ArrayList<ArrayMap<String, String>>()
            for (plugin in data.valueAt(idx)) {
                child.add(makeRowMap(keyDescr, "\t${plugin}"))
            }
            children.add(child)
        }
        val groupFrom = arrayOf(keyType)
        val groupTo = intArrayOf(android.R.id.text1)
        val childFrom = arrayOf(keyDescr)
        val childTo = intArrayOf(android.R.id.text1)
        val adapter = SimpleExpandableListAdapter(
            requireContext(),
            groups,
            android.R.layout.simple_expandable_list_item_1,
            groupFrom,
            groupTo,
            children,
            android.R.layout.simple_list_item_1,
            childFrom,
            childTo
        )
        binding.aboutPlugins.setAdapter(adapter)
    }

    private fun getPluginTypeString(type: Int): String {
        val resId = PluginsProvider.Types.values()[type].nameId()
        return getString(resId)
    }

    private fun makeRowMap(key: String, value: String) = ArrayMap<String, String>().apply {
        put(key, value)
    }
}
