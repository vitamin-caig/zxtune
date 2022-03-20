package app.zxtune.core

/**
 * Poperties 'namespace'
 */
object Properties {
    /**
     * Prefix for all properties
     */
    const val PREFIX = "zxtune."

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
