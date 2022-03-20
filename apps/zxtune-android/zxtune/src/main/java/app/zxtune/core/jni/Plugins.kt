package app.zxtune.core.jni

class Plugins {
    object DeviceType {
        //ZXTune::Capabilities::Module::Device::Type
        const val AY38910 = 1
        const val TURBOSOUND = 2
        const val BEEPER = 4
        const val YM2203 = 8
        const val TURBOFM = 16
        const val DAC = 32
        const val SAA1099 = 64
        const val MOS6581 = 128
        const val SPC700 = 256
        const val MULTIDEVICE = 512
        const val RP2A0X = 1024
        const val LR35902 = 2048
        const val CO12294 = 4096
        const val HUC6270 = 8192
    }

    object ContainerType {
        //ZXTune::Capabilities::Container::Type
        const val ARCHIVE = 0
        const val COMPRESSOR = 1
        const val SNAPSHOT = 2
        const val DISKIMAGE = 3
        const val DECOMPILER = 4
        const val MULTITRACK = 5
        const val SCANER = 6
    }

    interface Visitor {
        fun onPlayerPlugin(devices: Int, id: String, description: String)
        fun onContainerPlugin(type: Int, id: String, description: String)
    }
}
