package app.zxtune.core

/**
 * Poperties 'namespace'
 */
object Properties {
    /**
     * Prefix for all properties
     */
    const val PREFIX = "zxtune."

    object Core {
        private const val PREFIX = Properties.PREFIX + "core."

        /**
         * Channels muting mask (if supported, @see ModuleAttributes.CHANNELS_NAMES)
         */
        const val CHANNELS_MASK = PREFIX + "channels_mask"
    }

    /**
     * Sound properties 'namespace'
     */
    object Sound {
        private const val PREFIX = Properties.PREFIX + "sound."

        /**
         * Loop mode
         */
        const val LOOPED = PREFIX + "looped"
    }
}
