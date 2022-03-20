package app.zxtune.core

import app.zxtune.Releaseable
import app.zxtune.TimeStamp

/**
 * Module interface
 */
interface Module : PropertiesAccessor, AdditionalFiles, Releaseable {
  /**
   * @return Module's duration in milliseconds
   */
  val duration: TimeStamp

  /**
   * Creates new player object
   *
   * @throws RuntimeException in case of error
   */
  fun createPlayer(samplerate: Int): Player
}
