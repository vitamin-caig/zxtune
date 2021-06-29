/**
 *
 * @file
 *
 * @brief Playback item extension describing also module
 *
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.playback

import app.zxtune.core.Module

interface PlayableItem : Item {
    /**
     * @return Associated module
     */
    val module: Module
}
