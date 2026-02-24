package app.zxtune.device.media

import android.content.Context
import android.media.AudioManager
import android.os.Build
import app.zxtune.Releaseable
import app.zxtune.TimeStamp
import app.zxtune.playback.PlaybackControl
import app.zxtune.playback.PlaybackService
import kotlinx.coroutines.channels.BufferOverflow
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.asSharedFlow
import org.junit.After
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Assert.assertSame
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.doAnswer
import org.mockito.kotlin.doReturn
import org.mockito.kotlin.inOrder
import org.mockito.kotlin.mock
import org.mockito.kotlin.reset
import org.mockito.kotlin.verify
import org.mockito.kotlin.verifyNoMoreInteractions
import org.robolectric.RobolectricTestRunner
import org.robolectric.RuntimeEnvironment
import org.robolectric.Shadows.shadowOf
import org.robolectric.annotation.Config

private const val FOCUS_REQUEST_SDK = Build.VERSION_CODES.O

@RunWith(RobolectricTestRunner::class)
class AudioFocusConnectionTest {

    private val stateFlow = MutableSharedFlow<Pair<PlaybackService.State, TimeStamp>>(
        replay = 1, extraBufferCapacity = 1, onBufferOverflow = BufferOverflow.DROP_LATEST
    )
    private val ctrl = mock<PlaybackControl> {
        var state = PlaybackService.State.STOPPED
        on { play() } doAnswer {
            if (state != PlaybackService.State.PLAYING) {
                state = PlaybackService.State.PLAYING
                stateFlow.tryEmit(state to TimeStamp.EMPTY)
            }
        }
        on { stop() } doAnswer {
            if (state != PlaybackService.State.STOPPED) {
                state = PlaybackService.State.STOPPED
                stateFlow.tryEmit(state to TimeStamp.EMPTY)
            }
        }
    }
    private val callbackSubscription = mock<Releaseable>()
    private val service = mock<PlaybackService> {
        on { state } doReturn stateFlow.asSharedFlow()
        on { playbackControl } doReturn ctrl
    }

    @Before
    fun setUp() {
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
            verify(service).state
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
            verify(service).state
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
