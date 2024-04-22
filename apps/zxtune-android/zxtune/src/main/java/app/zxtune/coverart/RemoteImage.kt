package app.zxtune.coverart

import android.net.Uri
import android.widget.ImageView
import androidx.annotation.DrawableRes
import androidx.annotation.MainThread
import app.zxtune.Releaseable

class RemoteImage(private val loader: BitmapLoader) {
    private var view: ImageView? = null
    private var stub: ImageSource = EmptySource
    private var currentSource = stub
        private set(value) {
            field.release()
            field = value
        }
    private var currentUri: Uri? = null

    fun bindTo(img: ImageView?) {
        view = img
        if (img != null) {
            update()
        }
    }

    fun setStub(@DrawableRes default: Int) {
        val isStub = currentSource === stub
        stub = ResourceSource(default)
        if (isStub) {
            currentSource = stub
            view?.let {
                stub.applyTo(it)
            }
        }
    }

    fun setUri(uri: Uri?): Boolean {
        if (uri == currentUri) {
            return false
        }
        currentUri = uri
        update()
        return true
    }

    private fun update() {
        val uri = currentUri ?: return setSource(stub)
        loader.getCached(uri)?.let {
            setBitmap(it)
            return
        }
        // Transition to stub for any case
        setSource(stub)
        if (view == null) {
            // Do not load anything if no view available
            return
        }
        loader.get(uri) { image ->
            if (currentUri == uri) {
                view?.post {
                    setBitmap(image)
                }
            }
        }
    }

    @MainThread
    private fun setBitmap(image: BitmapReference) =
        setSource(image.bitmap?.let { BitmapSource(image) } ?: stub)

    private fun setSource(src: ImageSource) {
        if (currentSource === src) {
            return
        }
        view?.withFadeout {
            src.applyTo(it)
            currentSource = src
        } ?: run {
            currentSource = src
        }
    }

    private sealed interface ImageSource : Releaseable {
        fun applyTo(img: ImageView)
    }

    private data object EmptySource : ImageSource {
        override fun applyTo(img: ImageView) = Unit
        override fun release() = Unit
    }

    private class BitmapSource(val src: BitmapReference) : ImageSource {
        override fun applyTo(img: ImageView) = img.setImageBitmap(src.bitmap)
        override fun release() = src.release()
    }

    private class ResourceSource(@DrawableRes val src: Int) :
        ImageSource {
        override fun applyTo(img: ImageView) = img.setImageResource(src)
        override fun release() = Unit
    }
}
