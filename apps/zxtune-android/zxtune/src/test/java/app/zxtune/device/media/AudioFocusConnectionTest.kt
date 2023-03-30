package app.zxtune.device.media

import android.media.AudioManager
import app.zxtune.Releaseable
import app.zxtune.TimeStamp
import app.zxtune.device.sound.SoundOutputSamplesTarget
import app.zxtune.playback.Callback
import app.zxtune.playback.PlaybackControl
import app.zxtune.playback.PlaybackService
import org.junit.After
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.*
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class AudioFocusConnectionTest {

    private val manager = mock<AudioManager>()
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
            verify(service).subscribe(callback!!)
            // part1
            verify(ctrl).play()
            verify(manager).requestAudioFocus(
                focusListener,
                SoundOutputSamplesTarget.STREAM,
                AudioManager.AUDIOFOCUS_GAIN
            )
            verify(ctrl).stop() // loss
            verify(ctrl).play() // gain
            verify(ctrl).stop()
            verify(manager).abandonAudioFocus(focusListener)
            // part2
            verify(ctrl).play()
            verify(manager).requestAudioFocus(
                focusListener,
                SoundOutputSamplesTarget.STREAM,
                AudioManager.AUDIOFOCUS_GAIN
            )
            verify(ctrl).stop() // loss
            verify(ctrl).play()
            verify(manager).abandonAudioFocus(focusListener)
            verify(manager).requestAudioFocus(
                focusListener,
                SoundOutputSamplesTarget.STREAM,
                AudioManager.AUDIOFOCUS_GAIN
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
            verify(service).subscribe(callback!!)
            // part1
            verify(ctrl).play()
            verify(manager).requestAudioFocus(
                focusListener,
                SoundOutputSamplesTarget.STREAM,
                AudioManager.AUDIOFOCUS_GAIN
            )
            verify(ctrl).stop() // failed to gain
            // final
            verify(callbackSubscription).release()
        }
    }
}
