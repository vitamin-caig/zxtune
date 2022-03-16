package app.zxtune.fs.local

import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config
import org.robolectric.annotation.Implementation
import org.robolectric.annotation.Implements
import java.io.File
import java.io.StringReader

@RunWith(RobolectricTestRunner::class)
@Config(shadows = [ShadowUtils::class])
class LegacyStoragesSourceTest {

    @Test
    fun `emulator x86, sdk 16-18, sdcard`() = checkStorages(
        """
rootfs / rootfs ro,relatime 0 0
tmpfs /dev tmpfs rw,nosuid,relatime,mode=755 0 0
devpts /dev/pts devpts rw,relatime,mode=600 0 0
proc /proc proc rw,relatime 0 0
sysfs /sys sysfs rw,relatime 0 0
tmpfs /mnt tmpfs rw,relatime,mode=755,gid=1000 0 0
debugfs /sys/kernel/debug debugfs rw,relatime 0 0
tmpfs /mnt/asec tmpfs rw,relatime,mode=755,gid=1000 0 0
tmpfs /mnt/obb tmpfs rw,relatime,mode=755,gid=1000 0 0
/dev/block/vda /system ext4 ro,relatime,data=ordered 0 0
/dev/block/vdb /cache ext4 rw,nosuid,nodev,noatime,errors=panic,data=ordered 0 0
/dev/block/vdc /data ext4 rw,nosuid,nodev,noatime,errors=panic,data=ordered 0 0
/dev/block/vold/253:48 /mnt/sdcard vfat rw,dirsync,nosuid,nodev,noexec,relatime,uid=1000,gid=1015,fmask=0702,dmask=0702,allow_utime=0020,codepage=437,iocharset=iso8859-1,shortname=mixed,utf8,errors=remount-ro 0 0
/dev/block/vold/253:48 /mnt/secure/asec vfat rw,dirsync,nosuid,nodev,noexec,relatime,uid=1000,gid=1015,fmask=0702,dmask=0702,allow_utime=0020,codepage=437,iocharset=iso8859-1,shortname=mixed,utf8,errors=remount-ro 0 0
tmpfs /mnt/sdcard/.android_secure tmpfs ro,relatime,size=0k,mode=000 0 0        
""", arrayOf(
            "/mnt/sdcard",
        )
    )

    @Test
    fun `emulator x86, sdk 19-22, sdcard`() = checkStorages(
        """
rootfs / rootfs ro,relatime 0 0
tmpfs /dev tmpfs rw,nosuid,relatime,mode=755 0 0
devpts /dev/pts devpts rw,relatime,mode=600 0 0
proc /proc proc rw,relatime 0 0
sysfs /sys sysfs rw,relatime 0 0
debugfs /sys/kernel/debug debugfs rw,relatime 0 0
tmpfs /mnt/asec tmpfs rw,relatime,mode=755,gid=1000 0 0
tmpfs /mnt/obb tmpfs rw,relatime,mode=755,gid=1000 0 0
/dev/block/vda /system ext4 ro,relatime,data=ordered 0 0
/dev/block/vdb /cache ext4 rw,nosuid,nodev,noatime,errors=panic,data=ordered 0 0
/dev/block/vdc /data ext4 rw,nosuid,nodev,noatime,errors=panic,data=ordered 0 0
/dev/block/vold/253:48 /mnt/media_rw/sdcard vfat rw,dirsync,nosuid,nodev,noexec,relatime,uid=1023,gid=1023,fmask=0007,dmask=0007,allow_utime=0020,codepage=437,iocharset=iso8859-1,shortname=mixed,utf8,errors=remount-ro 0 0
/dev/block/vold/253:48 /mnt/secure/asec vfat rw,dirsync,nosuid,nodev,noexec,relatime,uid=1023,gid=1023,fmask=0007,dmask=0007,allow_utime=0020,codepage=437,iocharset=iso8859-1,shortname=mixed,utf8,errors=remount-ro 0 0
/dev/fuse /storage/sdcard fuse rw,nosuid,nodev,relatime,user_id=1023,group_id=1023,default_permissions,allow_other 0 0
""", arrayOf(
            "/mnt/media_rw/sdcard",
            "/storage/sdcard",
        )
    )

    @Test
    fun `emulator x86, sdk 23, sdcard`() = checkStorages(
        """
rootfs / rootfs ro,seclabel,relatime 0 0
tmpfs /dev tmpfs rw,seclabel,nosuid,relatime,mode=755 0 0
devpts /dev/pts devpts rw,seclabel,relatime,mode=600 0 0
proc /proc proc rw,relatime 0 0
sysfs /sys sysfs rw,seclabel,relatime 0 0
selinuxfs /sys/fs/selinux selinuxfs rw,relatime 0 0
debugfs /sys/kernel/debug debugfs rw,seclabel,relatime,mode=755 0 0
none /acct cgroup rw,relatime,cpuacct 0 0
none /sys/fs/cgroup tmpfs rw,seclabel,relatime,mode=750,gid=1000 0 0
tmpfs /mnt tmpfs rw,seclabel,relatime,mode=755,gid=1000 0 0
none /dev/cpuctl cgroup rw,relatime,cpu 0 0
/dev/block/vda /system ext4 ro,seclabel,relatime,data=ordered 0 0
/dev/block/vdb /cache ext4 rw,seclabel,nosuid,nodev,noatime,errors=panic,data=ordered 0 0
/dev/block/vdc /data ext4 rw,seclabel,nosuid,nodev,noatime,errors=panic,data=ordered 0 0
tmpfs /storage tmpfs rw,seclabel,relatime,mode=755,gid=1000 0 0
/dev/fuse /mnt/runtime/default/emulated fuse rw,nosuid,nodev,noexec,noatime,user_id=1023,group_id=1023,default_permissions,allow_other 0 0
/dev/fuse /storage/emulated fuse rw,nosuid,nodev,noexec,noatime,user_id=1023,group_id=1023,default_permissions,allow_other 0 0
/dev/fuse /mnt/runtime/read/emulated fuse rw,nosuid,nodev,noexec,noatime,user_id=1023,group_id=1023,default_permissions,allow_other 0 0
/dev/fuse /mnt/runtime/write/emulated fuse rw,nosuid,nodev,noexec,noatime,user_id=1023,group_id=1023,default_permissions,allow_other 0 0
/dev/block/vold/public:253,48 /mnt/media_rw/13F6-100D vfat rw,dirsync,nosuid,nodev,noexec,relatime,uid=1023,gid=1023,fmask=0007,dmask=0007,allow_utime=0020,codepage=437,iocharset=iso8859-1,shortname=mixed,utf8,errors=remount-ro 0 0
/dev/fuse /mnt/runtime/default/13F6-100D fuse rw,nosuid,nodev,noexec,noatime,user_id=1023,group_id=1023,default_permissions,allow_other 0 0
/dev/fuse /storage/13F6-100D fuse rw,nosuid,nodev,noexec,noatime,user_id=1023,group_id=1023,default_permissions,allow_other 0 0
/dev/fuse /mnt/runtime/read/13F6-100D fuse rw,nosuid,nodev,noexec,noatime,user_id=1023,group_id=1023,default_permissions,allow_other 0 0
/dev/fuse /mnt/runtime/write/13F6-100D fuse rw,nosuid,nodev,noexec,noatime,user_id=1023,group_id=1023,default_permissions,allow_other 0 0
""", arrayOf(
            "/storage/emulated",
            "/mnt/media_rw/13F6-100D",
            "/storage/13F6-100D",
        )
    )

    @Test
    fun `Samsung S5, from user`() = checkStorages(
        """
rootfs / rootfs ro,seclabel,relatime 0 0
tmpfs /dev tmpfs rw,seclabel,nosuid,relatime,size=865732k,nr_inodes=128483,mode=755 0 0
devpts /dev/pts devpts rw,seclabel,relatime,mode=600 0 0
none /dev/cpuctl cgroup rw,relatime,cpu 0 0
proc /proc proc rw,relatime 0 0
sysfs /sys sysfs rw,seclabel,relatime 0 0
selinuxfs /sys/fs/selinux selinuxfs rw,relatime 0 0
debugfs /sys/kernel/debug debugfs rw,seclabel,relatime 0 0
none /sys/fs/cgroup tmpfs rw,seclabel,relatime,size=865732k,nr_inodes=128483,mode=750,gid=1000 0 0
none /acct cgroup rw,relatime,cpuacct 0 0
tmpfs /mnt tmpfs rw,seclabel,relatime,size=865732k,nr_inodes=128483,mode=755,gid=1000 0 0
tmpfs /mnt/secure tmpfs rw,seclabel,relatime,size=865732k,nr_inodes=128483,mode=700 0 0
tmpfs /mnt/secure/asec tmpfs rw,seclabel,relatime,size=865732k,nr_inodes=128483,mode=700 0 0
/dev/block/vold/public:179,65 /mnt/secure/asec exfat rw,dirsync,nosuid,nodev,noexec,noatime,nodiratime,uid=1023,gid=1023,fmask=0007,dmask=0007,allow_utime=0020,codepage=cp437,iocharset=utf8,namecase=0,errors=remount-ro 0 0
/data/knox/tmp_sdcard /mnt/knox sdcardfs rw,seclabel,nosuid,nodev,relatime,mask=0077 0 0
/data/knox/sdcard /mnt/knox/default/knox-emulated sdcardfs rw,seclabel,nosuid,nodev,relatime,low_uid=1000,low_gid=1000,gid=9997,multi_user,mask=0006 0 0
/data/knox/sdcard /mnt/knox/read/knox-emulated sdcardfs rw,seclabel,nosuid,nodev,relatime,low_uid=1000,low_gid=1000,gid=9997,multi_user,mask=0027 0 0
/data/knox/sdcard /mnt/knox/write/knox-emulated sdcardfs rw,seclabel,nosuid,nodev,relatime,low_uid=1000,low_gid=1000,gid=9997,multi_user,mask=0007 0 0
/data/knox/secure_fs/enc_media /mnt/shell/enc_media sdcardfs rw,seclabel,nosuid,nodev,relatime,low_uid=1000,low_gid=1000,gid=9997,multi_user,reserved=20MB 0 0
/data/media /mnt/runtime/default/emulated sdcardfs rw,seclabel,nosuid,nodev,noexec,relatime,low_uid=1023,low_gid=1023,gid=1015,multi_user,mask=0006,reserved=20MB 0 0
/data/media /mnt/runtime/read/emulated sdcardfs rw,seclabel,nosuid,nodev,noexec,relatime,low_uid=1023,low_gid=1023,gid=9997,multi_user,mask=0027,reserved=20MB 0 0
/data/media /mnt/runtime/write/emulated sdcardfs rw,seclabel,nosuid,nodev,noexec,relatime,low_uid=1023,low_gid=1023,gid=9997,multi_user,mask=0007,reserved=20MB 0 0
/dev/block/vold/public:179,65 /mnt/media_rw/XXXX-XXXX exfat rw,dirsync,nosuid,nodev,noexec,noatime,nodiratime,uid=1023,gid=1023,fmask=0007,dmask=0007,allow_utime=0020,codepage=cp437,iocharset=utf8,namecase=0,errors=remount-ro 0 0
/mnt/media_rw/XXXX-XXXX /mnt/runtime/default/XXXX-XXXX sdcardfs rw,seclabel,nosuid,nodev,noexec,relatime,low_uid=1023,low_gid=1023,gid=1015,mask=0006 0 0
/mnt/media_rw/XXXX-XXXX /mnt/runtime/read/XXXX-XXXX sdcardfs rw,seclabel,nosuid,nodev,noexec,relatime,low_uid=1023,low_gid=1023,gid=9997,mask=0022 0 0
/mnt/media_rw/XXXX-XXXX /mnt/runtime/write/XXXX-XXXX sdcardfs rw,seclabel,nosuid,nodev,noexec,relatime,low_uid=1023,low_gid=1023,gid=9997,mask=0022 0 0
/dev/block/platform/msm_sdcc.1/by-name/apnhlos /firmware vfat ro,context=u:object_r:firmware_file:s0,relatime,uid=1000,gid=1000,fmask=0337,dmask=0227,codepage=cp437,iocharset=iso8859-1,shortname=lower,errors=remount-ro 0 0
/dev/block/platform/msm_sdcc.1/by-name/modem /firmware-modem vfat ro,context=u:object_r:firmware_file:s0,relatime,uid=1000,gid=1000,fmask=0337,dmask=0227,codepage=cp437,iocharset=iso8859-1,shortname=lower,errors=remount-ro 0 0
/dev/block/platform/msm_sdcc.1/by-name/system /system ext4 ro,seclabel,noatime,data=ordered 0 0
/dev/block/platform/msm_sdcc.1/by-name/userdata /data ext4 rw,seclabel,nosuid,nodev,noatime,discard,journal_checksum,journal_async_commit,noauto_da_alloc,data=ordered 0 0
/dev/block/platform/msm_sdcc.1/by-name/cache /cache ext4 rw,seclabel,nosuid,nodev,noatime,discard,journal_checksum,journal_async_commit,noauto_da_alloc,errors=panic,data=ordered 0 0
/dev/block/platform/msm_sdcc.1/by-name/persist /persist ext4 rw,seclabel,nosuid,nodev,noatime,discard,journal_checksum,journal_async_commit,noauto_da_alloc,data=ordered 0 0
""".trimIndent(), arrayOf(
            "/mnt/media_rw/XXXX-XXXX",
        )
    )

    @Test
    fun `Samsung SM-G110B, from user`() = checkStorages(
        """
rootfs / rootfs ro,relatime 0 0
tmpfs /dev tmpfs rw,seclabel,nosuid,relatime,mode=755 0 0
devpts /dev/pts devpts rw,seclabel,relatime,mode=600 0 0
proc /proc proc rw,relatime 0 0
sysfs /sys sysfs rw,seclabel,relatime 0 0
selinuxfs /sys/fs/selinux selinuxfs rw,relatime 0 0
debugfs /sys/kernel/debug debugfs rw,relatime 0 0
none /sys/fs/cgroup tmpfs rw,seclabel,relatime,mode=750,gid=1000 0 0
none /acct cgroup rw,relatime,cpuacct 0 0
tmpfs /mnt/secure tmpfs rw,seclabel,relatime,mode=700 0 0
tmpfs /mnt/secure/asec tmpfs rw,seclabel,relatime,mode=700 0 0
/dev/block/vold/179:129 /mnt/secure/asec vfat rw,dirsync,nosuid,nodev,noexec,noatime,nodiratime,uid=1023,gid=1023,fmask=0007,dmask=0007,allow_utime=0020,codepage=437,iocharset=iso8859-1,shortname=mixed,utf8,errors=remount-ro 0 0
tmpfs /mnt/asec tmpfs rw,seclabel,relatime,mode=755,gid=1000 0 0
/dev/block/dm-0 /mnt/asec/com.duolingo-7 ext4 ro,dirsync,seclabel,nosuid,nodev,noatime,errors=continue 0 0
tmpfs /mnt/obb tmpfs rw,seclabel,relatime,mode=755,gid=1000 0 0
/dev/block/platform/sprd-sdhci.3/by-name/system /system ext4 ro,seclabel,relatime,data=ordered 0 0
/dev/block/platform/sprd-sdhci.3/by-name/userdata /data ext4 rw,seclabel,nosuid,nodev,noatime,discard,journal_checksum,journal_async_commit,noauto_da_alloc,data=ordered 0 0
/dev/block/platform/sprd-sdhci.3/by-name/CSC /cache ext4 rw,seclabel,nosuid,nodev,noatime,discard,journal_checksum,journal_async_commit,noauto_da_alloc,errors=panic,data=ordered 0 0
/dev/block/platform/sprd-sdhci.3/by-name/efs /efs ext4 rw,seclabel,nosuid,nodev,noatime,discard,journal_checksum,journal_async_commit,noauto_da_alloc,data=ordered 0 0
/dev/block/platform/sprd-sdhci.3/by-name/PRODNV /productinfo ext4 rw,seclabel,nosuid,nodev,noatime,discard,journal_checksum,journal_async_commit,noauto_da_alloc,data=ordered 0 0
/data/media /mnt/shell/emulated sdcardfs rw,nosuid,nodev,relatime,uid=1023,gid=1023,derive=legacy,reserved=20MB 0 0
tmpfs /storage/emulated tmpfs rw,seclabel,nosuid,nodev,relatime,mode=050,gid=1028 0 0
/dev/block/vold/179:129 /mnt/media_rw/extSdCard vfat rw,dirsync,nosuid,nodev,noexec,noatime,nodiratime,uid=1023,gid=1023,fmask=0007,dmask=0007,allow_utime=0020,codepage=437,iocharset=iso8859-1,shortname=mixed,utf8,errors=remount-ro 0 0
/mnt/media_rw/extSdCard /storage/extSdCard sdcardfs rw,nosuid,nodev,relatime,uid=1023,gid=1023,derive=unified 0 0
/data/media /storage/emulated/0 sdcardfs rw,nosuid,nodev,relatime,uid=1023,gid=1023,derive=legacy,reserved=20MB 0 0
/data/media /storage/emulated/0/Android/obb sdcardfs rw,nosuid,nodev,relatime,uid=1023,gid=1023,derive=legacy,reserved=20MB 0 0
/data/media /storage/emulated/legacy sdcardfs rw,nosuid,nodev,relatime,uid=1023,gid=1023,derive=legacy,reserved=20MB 0 0
/data/media /storage/emulated/legacy/Android/obb sdcardfs rw,nosuid,nodev,relatime,uid=1023,gid=1023,derive=legacy,reserved=20MB 0 0
""", arrayOf(
            "/mnt/media_rw/extSdCard",
        )
    )

    @Test
    fun `ignore nested mounts`() = checkStorages(
        """
/dev/fuse /some/dir fs options 0 0
/dev/fuse /some/dir/nested fs options 0 0
/dev/fuse /some fs options 0 0
""", arrayOf(
            "/some"
        )
    )

    @Test
    fun `no storages`() = checkStorages(
        """
rootfs / rootfs ro,relatime 0 0
""", arrayOf(
            "/"
        )
    )

    private fun checkStorages(mounts: String, mountpoints: Array<String>) {
        val points = ArrayList<String>()
        makeSource(mounts).getStorages { obj, description ->
            assertEquals("", description)
            points.add(obj.absolutePath)
        }
        assertArrayEquals(mountpoints, points.toArray())
    }

    private fun makeSource(mounts: String) = LegacyStoragesSource {
        StringReader(mounts)
    }
}

@Implements(Utils::class)
class ShadowUtils {
    companion object {
        @JvmStatic
        @Implementation
        fun isAcessibleStorage(obj: File) = true
    }
}
