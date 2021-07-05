/**
 * @file
 * @brief Stub implementation of PlayerEventsListener
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.sound

open class StubPlayerEventsListener  // permit inheritance
protected constructor() : PlayerEventsListener {
  override fun onStart() {}
  override fun onSeeking() {}
  override fun onFinish() {}
  override fun onStop() {}
  override fun onError(e: Exception) {}
}
