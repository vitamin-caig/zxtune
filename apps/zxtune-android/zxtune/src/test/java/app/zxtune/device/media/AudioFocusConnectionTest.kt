package app.zxtune.device.media

import android.content.Context
import android.media.AudioManager
import android.os.Build
import app.zxtune.Releaseable
import app.zxtune.TimeStamp
import app.zxtune.playback.Callback
import app.zxtune.playback.PlaybackControl
import app.zxtune.playback.PlaybackService
import org.junit.After
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Assert.assertSame
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.*
import org.robolectric.RobolectricTestRunner
import org.robolectric.RuntimeEnvironment
import org.robolectric.Shadows.shadowOf
import org.robolectric.annotation.Config

private const val FOCUS_REQUEST_SDK = Build.VERSION_CODES.O

@RunWith(RobolectricTestRunner::class)
class AudioFocusConnectionTest {

    private var callback: Callback? = null
    private val ctrl = mock<PlaybackControl> {
        var state = PlaybackControl.State.STOPPED
        on { play() } doAnswer {
            if (state != PlaybackControl.State.PLAYING) {
                state = PlaybackControl.State.PLAYING
                callback!!.onStateChanged(state, TimeStamp.EMPTY)
            }
        }
        on { stop() } doAnswer {
            if (state != PlaybackControl.State.STOPPED) {
                state = PlaybackControl.State.STOPPED
                callback!!.onStateChanged(state, TimeStamp.EMPTY)
            }
        }
    }
    private val callbackSubscription = mock<Releaseable>()
    private val service = mock<PlaybackService> {
        on { subscribe(any()) } doAnswer {
            callback = it.getArgument(0)
            callbackSubscription
        }
        on { playbackControl } doReturn ctrl
    }

    @Before
    fun setUp() {
        callback = null
        reset(callbackSubscription)
    }

    @After
    fun tearDown() {
        verifyNoMoreInteractions(ctrl, callbackSubscription, service)
    }

    private fun createManager(response: Int): AudioManager = (RuntimeEnvironment.getApplication().getSystemService(Context.AUDIO_SERVICE) as AudioManager)
        .also { shadowOf(it).setNextFocusRequestResponse(response) }

    @Test
    @Config(sdk = [FOCUS_REQUEST_SDK - 1, FOCUS_REQUEST_SDK])
    fun `regular workflow`() {
        val manager = createManager(AudioManager.AUDIOFOCUS_REQUEST_GRANTED)
        with(AudioFocusConnection(manager, service)) {
            // part1
            ctrl.play()
            val focusListener = shadowOf(manager).lastAudioFocusRequest.listener
            focusListener.onAudioFocusChange(AudioManager.AUDIOFOCUS_LOSS_TRANSIENT)
            focusListener.onAudioFocusChange(AudioManager.AUDIOFOCUS_GAIN)
            ctrl.stop()
            // part2
            ctrl.play()
            focusListener.onAudioFocusChange(AudioManager.AUDIOFOCUS_LOSS)
            ctrl.play()
            ctrl.stop()
            // final
            release()
        }

        inOrder(ctrl, callbackSubscription, service) {
            // init
            verify(service).playbackControl
            verify(service).subscribe(callback!!)
            // part1
            verify(ctrl).play()
            verify(ctrl).stop() // loss
            verify(ctrl).play() // gain
            verify(ctrl).stop()
            // part2
            verify(ctrl).play()
            verify(ctrl).stop() // loss
            verify(ctrl).play()
            verify(ctrl).stop()
            // final
            verify(callbackSubscription).release()
        }
        val lastRequest = shadowOf(manager).lastAudioFocusRequest
        assertNotNull(lastRequest.listener)
        if (Build.VERSION.SDK_INT >= FOCUS_REQUEST_SDK) {
            val frameworkRequest = lastRequest.audioFocusRequest
            assertNotNull(frameworkRequest)
            assertSame(frameworkRequest, shadowOf(manager).lastAbandonedAudioFocusRequest)
        } else {
            assertNull(lastRequest.audioFocusRequest)
            assertSame(lastRequest.listener, shadowOf(manager).lastAbandonedAudioFocusListener)
        }
    }

    @Test
    @Config(sdk = [FOCUS_REQUEST_SDK - 1, FOCUS_REQUEST_SDK])
    fun `no focus gain`() {
        val manager = createManager(AudioManager.AUDIOFOCUS_REQUEST_FAILED)
        with(AudioFocusConnection(manager, service)) {
            // part1
            ctrl.play()
            // final
            release()
        }
        inOrder(ctrl, callbackSubscription, service) {
            // init
            verify(service).playbackControl
            verify(service).subscribe(callback!!)
            // part1
            verify(ctrl).play()
            verify(ctrl).stop() // failed to gain
            // final
            verify(callbackSubscription).release()
        }
        // focus request was still attempted regardless of the result
        assertNotNull(shadowOf(manager).lastAudioFocusRequest)
    }
}
