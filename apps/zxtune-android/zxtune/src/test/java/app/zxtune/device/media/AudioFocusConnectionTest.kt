package app.zxtune.device.media

import android.media.AudioManager
import app.zxtune.Releaseable
import app.zxtune.TimeStamp
import app.zxtune.device.sound.SoundOutputSamplesTarget
import app.zxtune.playback.PlaybackControl
import app.zxtune.playback.PlaybackService
import kotlinx.coroutines.channels.BufferOverflow
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.asSharedFlow
import org.junit.After
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.any
import org.mockito.kotlin.doAnswer
import org.mockito.kotlin.doReturn
import org.mockito.kotlin.inOrder
import org.mockito.kotlin.mock
import org.mockito.kotlin.reset
import org.mockito.kotlin.stub
import org.mockito.kotlin.verifyNoMoreInteractions
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class AudioFocusConnectionTest {

    private val manager = mock<AudioManager>()
    private val stateFlow = MutableSharedFlow<Pair<PlaybackControl.State, TimeStamp>>(
        replay = 1, extraBufferCapacity = 1, onBufferOverflow = BufferOverflow.DROP_LATEST
    )
    private val ctrl = mock<PlaybackControl> {
        var state = PlaybackControl.State.STOPPED
        on { play() } doAnswer {
            if (state != PlaybackControl.State.PLAYING) {
                state = PlaybackControl.State.PLAYING
                stateFlow.tryEmit(state to TimeStamp.EMPTY)
            }
        }
        on { stop() } doAnswer {
            if (state != PlaybackControl.State.STOPPED) {
                state = PlaybackControl.State.STOPPED
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
        reset(manager, callbackSubscription)
    }

    @After
    fun tearDown() {
        verifyNoMoreInteractions(manager, ctrl, callbackSubscription, service)
    }

    @Test
    fun `regular workflow`() {
        lateinit var focusListener: AudioManager.OnAudioFocusChangeListener
        manager.stub {
            on { requestAudioFocus(any(), any(), any()) } doAnswer {
                focusListener = it.getArgument(0)
                AudioManager.AUDIOFOCUS_REQUEST_GRANTED
            }
        }
        with(AudioFocusConnection(manager, service)) {
            // part1
            ctrl.play()
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

        inOrder(manager, ctrl, callbackSubscription, service) {
            // init
            verify(service).playbackControl
            verify(service).state
            // part1
            verify(ctrl).play()
            verify(manager).requestAudioFocus(
                focusListener, SoundOutputSamplesTarget.STREAM, AudioManager.AUDIOFOCUS_GAIN
            )
            verify(ctrl).stop() // loss
            verify(ctrl).play() // gain
            verify(ctrl).stop()
            verify(manager).abandonAudioFocus(focusListener)
            // part2
            verify(ctrl).play()
            verify(manager).requestAudioFocus(
                focusListener, SoundOutputSamplesTarget.STREAM, AudioManager.AUDIOFOCUS_GAIN
            )
            verify(ctrl).stop() // loss
            verify(ctrl).play()
            verify(manager).abandonAudioFocus(focusListener)
            verify(manager).requestAudioFocus(
                focusListener, SoundOutputSamplesTarget.STREAM, AudioManager.AUDIOFOCUS_GAIN
            )
            verify(ctrl).stop()
            verify(manager).abandonAudioFocus(focusListener)
            // final
            verify(callbackSubscription).release()
        }
    }

    @Test
    fun `no focus gain`() {
        var focusListener: AudioManager.OnAudioFocusChangeListener? = null
        manager.stub {
            on { requestAudioFocus(any(), any(), any()) } doAnswer {
                focusListener = it.getArgument(0)
                AudioManager.AUDIOFOCUS_REQUEST_FAILED
            }
        }
        with(AudioFocusConnection(manager, service)) {
            // part1
            ctrl.play()
            // final
            release()
        }
        inOrder(manager, ctrl, callbackSubscription, service) {
            // init
            verify(service).playbackControl
            verify(service).state
            // part1
            verify(ctrl).play()
            verify(manager).requestAudioFocus(
                focusListener, SoundOutputSamplesTarget.STREAM, AudioManager.AUDIOFOCUS_GAIN
            )
            verify(ctrl).stop() // failed to gain
            // final
            verify(callbackSubscription).release()
        }
    }
}
