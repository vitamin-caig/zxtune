package app.zxtune.core.jni

import org.junit.Assert.assertEquals
import org.junit.Test

class JniPluginsTest {
    @Test
    fun testPlugins() {
        val flags = intArrayOf(0, 0)
        val counts = intArrayOf(0, 0)
        val players = HashSet<String>()
        val containers = HashSet<String>()
        val api = JniApi()
        api.enumeratePlugins(object : Plugins.Visitor {
            override fun onPlayerPlugin(devices: Int, id: String, description: String) {
                flags[0] = flags[0] or devices
                ++counts[0]
                players.add(id)
            }

            override fun onContainerPlugin(type: Int, id: String, description: String) {
                flags[1] = flags[1] or (1 shl type)
                ++counts[1]
                containers.add(id)
            }
        })
        assertEquals(16375, flags[0])
        assertEquals(51, flags[1])
        assertEquals(118, counts[0])
        assertEquals(24, counts[1])
    }
}