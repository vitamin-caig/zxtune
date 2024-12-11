/**
 * @file
 * @brief Visualizer view component
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.ui.views

import android.content.Context
import android.content.res.TypedArray
import android.graphics.Canvas
import android.graphics.Paint
import android.graphics.PixelFormat
import android.graphics.PorterDuff
import android.graphics.Rect
import android.os.Build
import android.util.AttributeSet
import android.view.SurfaceHolder
import android.view.SurfaceView
import androidx.annotation.ColorInt
import androidx.core.content.res.use
import app.zxtune.Logger
import app.zxtune.R
import app.zxtune.analytics.Analytics
import app.zxtune.playback.Visualizer
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock
import kotlinx.coroutines.withContext
import kotlin.math.max
import kotlin.math.min

private val LOG = Logger("SpectrumAnalyzer")

class SpectrumAnalyzerView @JvmOverloads constructor(
    context: Context, attrs: AttributeSet? = null, defStyleAttr: Int = 0
) : SurfaceView(context, attrs, defStyleAttr) {
    private val visualizer = context.theme.obtainStyledAttributes(
        attrs, R.styleable.AnalyzerView, defStyleAttr, 0
    ).use {
        createSpectrumVisualizer(it)
    }
    private val surfaceLock = Mutex(true)
    private val visibilityLock = Mutex(false)
    private val callback = object : SurfaceHolder.Callback {
        override fun surfaceChanged(holder: SurfaceHolder, format: Int, w: Int, h: Int) {
            val padLeft = paddingLeft
            val padRight = paddingRight
            val padTop = paddingTop
            val padBottom = paddingBottom
            val padHorizontal = padLeft + padRight
            val padVertical = padTop + padBottom
            val visibleRect = if (padHorizontal < w && padVertical < h) {
                Rect(padLeft, padTop, w - padRight, h - padBottom)
            } else {
                Rect(0, 0, w, h)
            }
            LOG.d { "surfaceChanged(${w}x${h} visible=${visibleRect})" }
            visualizer.sizeChanged(visibleRect)
        }

        override fun surfaceCreated(holder: SurfaceHolder) = surfaceLock.unlock().also {
            LOG.d { "surfaceCreated" }
        }

        override fun surfaceDestroyed(holder: SurfaceHolder) =
            runBlocking { surfaceLock.lock() }.also {
                LOG.d { "surfaceDestroyed" }
            }
    }
    private val vblank = object {
        private val maxFrameDuration = 1_000_000L / UPDATE_FPS
        var lostFrames = 0
            private set
        var doneFrames = 0
        private var totalDuration = 0L
        val avgDuration
            get() = totalDuration / doneFrames.coerceAtLeast(1)
        private var lastFrameTime = 0L

        suspend fun sync() {
            val now = now()
            val passed = now - lastFrameTime
            if (passed > maxFrameDuration) {
                ++lostFrames;
                lastFrameTime = now
            } else {
                delay((maxFrameDuration - passed) / 1_000)
                totalDuration += passed
                ++doneFrames
                lastFrameTime = now()
            }
        }

        private fun now() = System.nanoTime() / 1_000
    }

    init {
        setZOrderOnTop(true)
        setLayerType(LAYER_TYPE_HARDWARE, null)
        holder.run {
            setFormat(PixelFormat.TRANSPARENT)
            addCallback(callback)
        }
    }

    private fun createSpectrumVisualizer(a: TypedArray) = SpectrumVisualizer(
        barColor = a.getColor(R.styleable.AnalyzerView_spectrumBarsColor, 0xffffff),
        maxBarsCount = a.getInteger(R.styleable.AnalyzerView_spectrumMaxBarsCount, 48),
        minBarWidth = a.getDimensionPixelSize(R.styleable.AnalyzerView_spectrumMinBarWidth, 3)
    )

    suspend fun drawFrom(src: Visualizer) = withContext(Dispatchers.Default) {
        LOG.d { "drawFrom($src)" }
        while (visualizer.update(src)) {
            vblank.sync()
            visibilityLock.withLock {
                surfaceLock.withLock {
                    holder.onCanvas { canvas ->
                        canvas.drawColor(0, PorterDuff.Mode.CLEAR)
                        visualizer.draw(canvas)
                    }
                }
            }
        }
        LOG.d { "drawFrom finished" }
    }

    suspend fun setIsUpdating(isUpdating: Boolean) {
        if (isUpdating == visibilityLock.holdsLock(this)) {
            if (isUpdating) {
                visibilityLock.unlock()
            } else {
                visibilityLock.lock(this)
            }
        }
    }

    fun reportStatistics() {
        val avgDuration = vblank.avgDuration
        LOG.d {
            "${vblank.doneFrames}@${avgDuration}uS done, ${vblank.lostFrames} lost"
        }
        Analytics.sendEvent(
            "ui/visualizer",
            "bars" to visualizer.bars,
            "w" to visualizer.visibleRect.width(),
            "h" to visualizer.visibleRect.height(),
            "frames" to vblank.doneFrames,
            "resync" to vblank.lostFrames,
            "avg" to avgDuration,
            "hw" to useHardwareCanvas,
        )
    }

    companion object {
        const val UPDATE_FPS = 30
    }
}

private class SpectrumVisualizer(
    @ColorInt barColor: Int, private val maxBarsCount: Int, private val minBarWidth: Int
) {
    var bars = maxBarsCount
        private set
    private val levels = ByteArray(maxBarsCount)
    val visibleRect = Rect()
    private val barRect = Rect()
    private val paint = Paint().apply {
        color = barColor
    }
    private var barWidth = minBarWidth
    private var values = IntArray(1)

    fun sizeChanged(visible: Rect) {
        visibleRect.set(visible)
        barRect.bottom = visibleRect.bottom
        val width = visibleRect.width()
        barWidth = max(width / maxBarsCount, minBarWidth)
        bars = max(width / barWidth, 1)
        values = IntArray(bars)
    }

    fun draw(canvas: Canvas) {
        barRect.left = visibleRect.left
        barRect.right = barRect.left + barWidth - BAR_PADDING
        val height = visibleRect.height()
        for (level in values) {
            if (level != 0) {
                barRect.top = visibleRect.top + height - level * height / MAX_LEVEL
                canvas.drawRect(barRect, paint)
            }
            barRect.offset(barWidth, 0)
        }
    }

    fun update(src: Visualizer): Boolean {
        val bars = runCatching {
            src.getSpectrum(levels)
        }.getOrNull() ?: return false
        return if (bars != 0) {
            update(bars)
            true
        } else {
            update()
        }
    }

    private fun update(channels: Int) {
        var band = 0
        val lim = min(channels, values.size)
        while (band < lim) {
            values[band] = max(values[band] - FALL_SPEED, levels[band].toInt())
            ++band
        }
    }

    private fun update(): Boolean {
        var result = false
        for (band in values.indices) {
            if (values[band] >= FALL_SPEED) {
                values[band] -= FALL_SPEED
                result = true
            } else {
                values[band] = 0
            }
        }
        return result
    }

    companion object {
        private const val FALL_SPEED = 2
        private const val MAX_LEVEL = 100
        private const val BAR_PADDING = 1
    }
}

private val useHardwareCanvas
    get() = Build.VERSION.SDK_INT >= 26

private val SurfaceHolder.canvas: Canvas?
    get() = if (useHardwareCanvas) lockHardwareCanvas() else lockCanvas()

private fun SurfaceHolder.onCanvas(block: (Canvas) -> Unit) = runCatching {
    val canvas = canvas ?: return@runCatching
    block(canvas)
    unlockCanvasAndPost(canvas)
}.onFailure {
    LOG.w(it) { "Failed to draw on canvas" }
}
