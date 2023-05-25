package app.zxtune.ui

import android.app.Application
import android.content.Context
import android.content.Intent
import android.content.res.Configuration
import android.os.Build
import android.os.Bundle
import android.text.TextUtils
import android.util.DisplayMetrics
import android.widget.SimpleExpandableListAdapter
import androidx.appcompat.app.AppCompatActivity
import androidx.collection.ArrayMap
import androidx.collection.SparseArrayCompat
import androidx.databinding.DataBindingUtil
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModelProvider
import app.zxtune.BuildConfig
import app.zxtune.PluginsProvider
import app.zxtune.analytics.Analytics
import app.zxtune.databinding.AboutBinding
import java.util.concurrent.Executors

class AboutActivity : AppCompatActivity() {
    companion object {
        @JvmStatic
        fun createIntent(ctx: Context) = Intent(ctx, AboutActivity::class.java)
    }

    private lateinit var binding: AboutBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = DataBindingUtil.setContentView(this, app.zxtune.R.layout.about)
        binding.run {
            aboutTitle.text = getAppInfo()
            aboutSystem.text = getSystemInfo()
            aboutPager.adapter = ViewPagerAdapter(aboutPager)
        }

        Model.of(this).data.observe(this, this::fillPlugins)

        Analytics.sendUiEvent(Analytics.UI_ACTION_ABOUT)
    }

    private fun getAppInfo() =
        "${getString(app.zxtune.R.string.app_name)} b${BuildConfig.VERSION_CODE} (${BuildConfig.FLAVOR})"

    private fun getSystemInfo() = InfoBuilder(this).apply {
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
            this,
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
            fun of(owner: AppCompatActivity) = ViewModelProvider(
                owner, ViewModelProvider.AndroidViewModelFactory.getInstance(owner.application)
            )[Model::class.java]
        }
    }
}

@Suppress("DEPRECATION")
private class InfoBuilder constructor(private val context: Context) {
    private val strings = StringBuilder()

    fun buildOSInfo() = strings.run {
        addString("Android ${Build.VERSION.RELEASE} (API v${Build.VERSION.SDK_INT})")
        if (Build.VERSION.SDK_INT >= 21) {
            addString("${Build.MODEL} (${TextUtils.join("/", Build.SUPPORTED_ABIS)})")
        } else {
            addString("${Build.MODEL} (${Build.CPU_ABI}/${Build.CPU_ABI2}")
        }
    }

    fun buildConfigurationInfo() {
        val config = context.resources.configuration
        val metrics = context.resources.displayMetrics
        strings.run {
            addWord(getLayoutSize(config.screenLayout))
            addWord(getLayoutRatio(config.screenLayout))
            addWord(getOrientation(config.orientation))
            addWord(getDensity(metrics.densityDpi))
            if (Build.VERSION.SDK_INT >= 24) {
                addWord(config.locales.toLanguageTags())
            } else {
                addWord(config.locale.toString())
            }
        }
    }

    val result
        get() = strings.toString()

    companion object {
        private fun getLayoutSize(layout: Int) =
            when (layout and Configuration.SCREENLAYOUT_SIZE_MASK) {
                Configuration.SCREENLAYOUT_SIZE_XLARGE -> "xlarge"
                Configuration.SCREENLAYOUT_SIZE_LARGE -> "large"
                Configuration.SCREENLAYOUT_SIZE_NORMAL -> "normal"
                Configuration.SCREENLAYOUT_SIZE_SMALL -> "small"
                else -> "size-undef"
            }

        private fun getLayoutRatio(layout: Int) =
            when (layout and Configuration.SCREENLAYOUT_LONG_MASK) {
                Configuration.SCREENLAYOUT_LONG_YES -> "long"
                Configuration.SCREENLAYOUT_LONG_NO -> "notlong"
                else -> "ratio-undef"
            }

        private fun getOrientation(orientation: Int) = when (orientation) {
            Configuration.ORIENTATION_LANDSCAPE -> "land"
            Configuration.ORIENTATION_PORTRAIT -> "port"
            Configuration.ORIENTATION_SQUARE -> "square"
            else -> "orientation-undef"
        }

        private fun getDensity(density: Int) = when (density) {
            DisplayMetrics.DENSITY_LOW -> "ldpi"
            DisplayMetrics.DENSITY_MEDIUM -> "mdpi"
            DisplayMetrics.DENSITY_HIGH -> "hdpi"
            DisplayMetrics.DENSITY_XHIGH -> "xhdpi"
            480 -> "xxhdpi"
            640 -> "xxxhdpi"
            else -> "${density}dpi"
        }
    }
}

private fun StringBuilder.addString(s: String) = apply {
    append(s)
    append('\n')
}

private fun StringBuilder.addWord(s: String) = apply {
    append(s)
    append(' ')
}

private fun <T> SparseArrayCompat<T>.getOrPut(key: Int, default: T) =
    putIfAbsent(key, default) ?: default