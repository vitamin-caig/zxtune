package app.zxtune.ui.utils

import android.view.View
import androidx.core.graphics.Insets
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import org.junit.Assert.assertEquals
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner
import org.robolectric.RuntimeEnvironment
import org.robolectric.annotation.Config

// Type.ime() is only reported from API 30 on, so the padding math can only be checked there
@RunWith(RobolectricTestRunner::class)
@Config(sdk = [30])
class EdgeToEdgeUtilsTest {

    private fun insetsOf(
        statusBars: Insets = Insets.NONE,
        navigationBars: Insets = Insets.NONE,
        displayCutout: Insets = Insets.NONE,
        ime: Insets = Insets.NONE,
        // a transient bar swipe dispatches hidden bars, but the platform keeps reporting how
        // much room they will take once they slide back in
        navigationBarsHidden: Boolean = false,
    ) = WindowInsetsCompat.Builder()
        .apply {
            // getInsets() only reports the types flagged visible, and the builder flags just
            // the first one set - drive it explicitly rather than depend on the call order
            setVisible(WindowInsetsCompat.Type.statusBars(), true)
            setVisible(WindowInsetsCompat.Type.displayCutout(), true)
            setVisible(WindowInsetsCompat.Type.navigationBars(), !navigationBarsHidden)
            setVisible(WindowInsetsCompat.Type.ime(), true)
            setInsets(WindowInsetsCompat.Type.statusBars(), statusBars)
            setInsets(WindowInsetsCompat.Type.navigationBars(), navigationBars)
            setInsets(WindowInsetsCompat.Type.displayCutout(), displayCutout)
            setInsets(WindowInsetsCompat.Type.ime(), ime)
            setInsetsIgnoringVisibility(WindowInsetsCompat.Type.navigationBars(), navigationBars)
        }
        .build()

    private fun newView(padding: Insets = Insets.NONE) = View(RuntimeEnvironment.getApplication())
        .apply { setPadding(padding.left, padding.top, padding.right, padding.bottom) }

    private fun View.padding() = Insets.of(paddingLeft, paddingTop, paddingRight, paddingBottom)

    private fun View.applyInsets(insets: WindowInsetsCompat) = ViewCompat.dispatchApplyWindowInsets(
        this,
        insets
    )

    @Test
    fun padsWithStatusBarAndNavigationBar() {
        assertEquals(
            Insets.of(0, 63, 0, 126),
            systemBarPadding(
                insetsOf(
                    statusBars = Insets.of(0, 63, 0, 0),
                    navigationBars = Insets.of(0, 0, 0, 126),
                )
            )
        )
    }

    @Test
    fun padsWithDisplayCutout() {
        assertEquals(Insets.of(15, 0, 0, 0), systemBarPadding(insetsOf(displayCutout = Insets.of(15, 0, 0, 0))))
    }

    @Test
    fun padsWithLargestOfNavigationBarAndKeyboard() {
        assertEquals(
            Insets.of(0, 0, 0, 800),
            systemBarPadding(
                insetsOf(
                    navigationBars = Insets.of(0, 0, 0, 126),
                    ime = Insets.of(0, 300, 0, 800),
                )
            )
        )
    }

    @Test
    fun padsWithNavigationBarWhenTheKeyboardIsSmaller() {
        assertEquals(
            Insets.of(0, 0, 0, 126),
            systemBarPadding(
                insetsOf(
                    navigationBars = Insets.of(0, 0, 0, 126),
                    ime = Insets.of(0, 0, 0, 48),
                )
            )
        )
    }

    @Test
    fun padsWithNavigationBarWhenItIsHidden() {
        assertEquals(
            Insets.of(0, 0, 0, 126),
            systemBarPadding(
                insetsOf(
                    navigationBars = Insets.of(0, 0, 0, 126),
                    navigationBarsHidden = true,
                )
            )
        )
    }

    @Test
    fun padsWithNothingWhenThereAreNoInsets() {
        assertEquals(Insets.NONE, systemBarPadding(insetsOf()))
    }

    @Test
    fun addsPaddingToTheOriginalOne() {
        newView(Insets.of(4, 5, 6, 7)).run {
            padWithSystemBars()
            applyInsets(insetsOf(statusBars = Insets.of(0, 63, 0, 0)))
            assertEquals(Insets.of(4, 68, 6, 7), padding())
        }
    }

    @Test
    fun doesNotAccumulatePaddingOnRepeatedDispatch() {
        newView(Insets.of(4, 5, 6, 7)).run {
            padWithSystemBars()
            val insets = insetsOf(statusBars = Insets.of(0, 63, 0, 0))
            applyInsets(insets)
            applyInsets(insets)
            applyInsets(insets)
            assertEquals(Insets.of(4, 68, 6, 7), padding())
        }
    }

    @Test
    fun doesNotConsumeInsets() {
        newView().run {
            padWithSystemBars()
            val applied = applyInsets(insetsOf(statusBars = Insets.of(0, 63, 0, 0)))
            assertEquals(Insets.of(0, 63, 0, 0), applied.getInsets(WindowInsetsCompat.Type.statusBars()))
        }
    }
}
