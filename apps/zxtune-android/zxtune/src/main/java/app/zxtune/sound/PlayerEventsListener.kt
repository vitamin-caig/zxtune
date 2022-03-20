/**
 * @file
 * @brief Player events listener interface
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.sound

/**
 * All calls are synchronous and may be performed from background thread
 */
interface PlayerEventsListener {
    /**
     * Called when playback is started or resumed
     */
    fun onStart()

    /**
     * Called when seek is started
     */
    fun onSeeking()

    /**
     * Called when played stream come to an end
     */
    fun onFinish()

    /**
     * Called when playback stopped (also called after onFinish)
     */
    fun onStop()

    /**
     * Called on unexpected error occurred
     * @param e Exception happened
     */
    fun onError(e: Exception)
}
