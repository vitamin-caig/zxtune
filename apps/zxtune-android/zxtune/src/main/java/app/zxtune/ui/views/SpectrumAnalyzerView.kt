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
import android.os.Handler
import android.os.HandlerThread
import android.util.AttributeSet
import android.view.SurfaceHolder
import android.view.SurfaceView
import androidx.annotation.ColorInt
import androidx.core.content.res.use
import app.zxtune.Logger
import app.zxtune.R
import app.zxtune.playback.Visualizer
import app.zxtune.playback.stubs.VisualizerStub
import kotlin.math.max
import kotlin.math.min

private val LOG = Logger("SpectrumAnalyzer")

class SpectrumAnalyzerView @JvmOverloads constructor(
    context: Context, attrs: AttributeSet? = null, defStyleAttr: Int = 0
) : SurfaceView(context, attrs, defStyleAttr), SurfaceHolder.Callback {
    private val source = DispatchingVisualizer()
    private val visualizer: SpectrumVisualizer
    private val renderer: RenderThread

    init {
        visualizer = context.theme.obtainStyledAttributes(
            attrs, R.styleable.AnalyzerView, defStyleAttr, 0
        ).use {
            createSpectrumVisualizer(it)
        }
        renderer = RenderThread(source, visualizer)
        init()
    }

    private fun createSpectrumVisualizer(a: TypedArray) = SpectrumVisualizer(
        barColor = a.getColor(R.styleable.AnalyzerView_spectrumBarsColor, 0xffffff),
        maxBarsCount = a.getInteger(R.styleable.AnalyzerView_spectrumMaxBarsCount, 48),
        minBarWidth = a.getDimensionPixelSize(R.styleable.AnalyzerView_spectrumMinBarWidth, 3)
    )

    private fun init() {
        setZOrderOnTop(true)
        setLayerType(LAYER_TYPE_HARDWARE, null)
        holder.run {
            setFormat(PixelFormat.TRANSPARENT)
            addCallback(this@SpectrumAnalyzerView)
        }
    }

    fun setSource(src: Visualizer?) {
        if (source.setDelegate(src)) {
            renderer.update()
        }
    }

    fun setIsPlaying(playing: Boolean) {
        if (source.setIsUpdating(playing)) {
            renderer.update()
        }
    }

    fun setIsUpdating(updating: Boolean) {
        renderer.isActive = updating
    }

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
        visualizer.sizeChanged(visibleRect)
    }

    override fun surfaceCreated(holder: SurfaceHolder) = renderer.setHolder(holder)

    override fun surfaceDestroyed(holder: SurfaceHolder) = renderer.setHolder(null)

    private class RenderThread(private val src: Visualizer, private val dst: SpectrumVisualizer) {
        private val thread by lazy {
            HandlerThread("Visualizer").apply {
                start()
            }
        }
        private val handler by lazy {
            Handler(thread.looper)
        }

        private val buffer = ByteArray(dst.maxBarsCount)
        private var drawTask: DrawTask? = null
        var isActive: Boolean = true
            set(value) {
                if (value != field) {
                    field = value
                    if (value) {
                        drawTask?.schedule()
                    } else {
                        drawTask?.cancel()
                    }
                }
            }

        fun setHolder(holder: SurfaceHolder?) {
            handler.postAtFrontOfQueue {
                drawTask?.cancel()
                drawTask = holder?.let {
                    DrawTask(it)
                }
                update()
            }
        }

        fun update() {
            if (isActive) {
                drawTask?.schedule()
            }
        }

        private inner class DrawTask(private val holder: SurfaceHolder) : Runnable {
            override fun run() {
                schedule()
                if (update()) {
                    draw()
                } else {
                    cancel()
                }
            }

            fun schedule() {
                handler.postDelayed(this, (1000 / UPDATE_FPS).toLong())
            }

            fun cancel() = handler.removeCallbacks(this)

            private fun update(): Boolean {
                val bars = src.getSpectrum(buffer)
                return if (bars != 0) {
                    dst.update(bars, buffer)
                    true
                } else {
                    dst.update()
                }
            }

            private fun draw() = holder.onCanvas { canvas ->
                canvas.drawColor(0, PorterDuff.Mode.CLEAR)
                dst.draw(canvas)
            }
        }
    }

    private class SpectrumVisualizer(
        @ColorInt barColor: Int, val maxBarsCount: Int, private val minBarWidth: Int
    ) {
        private val visibleRect = Rect()
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
            val bars = max(width / barWidth, 1)
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

        fun update(channels: Int, levels: ByteArray) {
            var band = 0
            val lim = min(channels, values.size)
            while (band < lim) {
                values[band] = max(values[band] - FALL_SPEED, levels[band].toInt())
                ++band
            }
        }

        fun update(): Boolean {
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
    }

    companion object {
        private const val MAX_LEVEL = 100
        private const val BAR_PADDING = 1
        private const val FALL_SPEED = 2
        private const val UPDATE_FPS = 30
    }
}

private class DispatchingVisualizer : Visualizer {

    private var delegate: Visualizer = VisualizerStub.instance()
    private var isUpdating: Boolean = true

    fun setDelegate(update: Visualizer?): Boolean {
        val value = update ?: VisualizerStub.instance()
        return if (delegate != value) {
            delegate = value
            true
        } else {
            false
        }
    }

    fun setIsUpdating(update: Boolean) = if (isUpdating != update) {
        isUpdating = update
        true
    } else {
        false
    }

    override fun getSpectrum(levels: ByteArray) = if (isUpdating) {
        delegate.getSpectrum(levels)
    } else {
        0
    }
}

private fun SurfaceHolder.onCanvas(block: (Canvas) -> Unit) = runCatching {
    val canvas = if (Build.VERSION.SDK_INT >= 26) lockHardwareCanvas() else lockCanvas()
    block(canvas)
    unlockCanvasAndPost(canvas)
}.onFailure {
    LOG.w(it) { "Failed to draw on canvas" }
}
