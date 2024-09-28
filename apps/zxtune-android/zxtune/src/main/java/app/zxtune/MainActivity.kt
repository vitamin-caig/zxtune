/**
 * @file
 * @brief Main application activity
 * @author vitamin.caig@gmail.com
 */
package app.zxtune

import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import android.os.Build
import android.os.Bundle
import android.os.StrictMode
import android.os.StrictMode.ThreadPolicy
import android.os.StrictMode.VmPolicy
import android.support.v4.media.session.MediaControllerCompat
import android.view.View
import androidx.appcompat.app.AppCompatActivity
import androidx.collection.SparseArrayCompat
import androidx.collection.contains
import androidx.collection.getOrElse
import androidx.lifecycle.lifecycleScope
import androidx.viewpager.widget.ViewPager
import androidx.viewpager.widget.ViewPager.OnPageChangeListener
import app.zxtune.analytics.Analytics
import app.zxtune.device.media.MediaModel
import app.zxtune.ui.ViewPagerAdapter
import app.zxtune.ui.utils.whenLifecycleStarted
import kotlinx.coroutines.flow.last
import kotlinx.coroutines.launch

class MainActivity : AppCompatActivity(R.layout.main_activity) {
    interface PagerTabListener {
        fun onTabVisibilityChanged(isVisible: Boolean)
    }

    private object StubPagerTabListener : PagerTabListener {
        override fun onTabVisibilityChanged(isVisible: Boolean) = Unit
    }

    public override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        setupUi()

        if (savedInstanceState != null) {
            intent = null
        }
        setupMediaController();
        Analytics.sendUiEvent(Analytics.UiAction.OPEN)

        // TODO: move to MainApplication
        if (BuildConfig.BUILD_TYPE != "release") {
            StrictMode.setThreadPolicy(ThreadPolicy.Builder().detectAll().build())
            StrictMode.setVmPolicy(VmPolicy.Builder().detectAll().build())
        }
    }

    public override fun onDestroy() {
        super.onDestroy()

        Analytics.sendUiEvent(Analytics.UiAction.CLOSE)
    }

    private fun setupMediaController() {
        val ctrl = MediaModel.of(this@MainActivity).controller
        whenLifecycleStarted {
            ctrl.collect {
                MediaControllerCompat.setMediaController(this@MainActivity, it)
            }
        }

        intent?.takeIf { Intent.ACTION_VIEW == it.action }?.data?.let { uri ->
            lifecycleScope.launch {
                ctrl.last()?.transportControls?.playFromUri(uri, null)
            }
        }
    }

    private fun setupUi() {
        requestedOrientation = resources.getInteger(R.integer.screen_orientation)
        findViewById<ViewPager>(R.id.view_pager)?.run {
            val adapter = ViewPagerAdapter(this)
            this.adapter = adapter
            addOnPageChangeListener(object : OnPageChangeListener {
                private val listeners = SparseArrayCompat<PagerTabListener>()
                private var prevPos = currentItem

                override fun onPageScrolled(
                    position: Int, positionOffset: Float, positionOffsetPixels: Int
                ) = Unit

                override fun onPageSelected(newPos: Int) {
                    if (newPos != prevPos) {
                        getListener(prevPos).onTabVisibilityChanged(false)
                    }
                    getListener(newPos).onTabVisibilityChanged(true)
                    prevPos = newPos
                }

                override fun onPageScrollStateChanged(state: Int) = Unit
                private fun getListener(idx: Int) = listeners.getOrElse(idx) {
                    createListener(idx).also {
                        listeners.put(idx, it)
                    }
                }

                private fun createListener(idx: Int): PagerTabListener {
                    check(!listeners.contains(idx))
                    val id = (adapter.instantiateItem(this@run, idx) as View).id
                    val frag = supportFragmentManager.findFragmentById(id)
                    return if (frag is PagerTabListener) {
                        frag
                    } else {
                        StubPagerTabListener
                    }
                }
            })
        }
    }

    companion object {
        @JvmField
        val PENDING_INTENT_FLAG: Int =
            if (Build.VERSION.SDK_INT >= 23) PendingIntent.FLAG_IMMUTABLE else 0

        @JvmStatic
        fun createPendingIntent(ctx: Context): PendingIntent {
            val intent = Intent(ctx, MainActivity::class.java)
            intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP or Intent.FLAG_ACTIVITY_SINGLE_TOP)
            return PendingIntent.getActivity(ctx, 0, intent, PENDING_INTENT_FLAG)
        }
    }
}
