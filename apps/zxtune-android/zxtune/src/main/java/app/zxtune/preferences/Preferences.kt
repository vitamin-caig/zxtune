/**
 *
 * @file
 *
 * @brief
 *
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.preferences

import android.content.Context
import android.content.SharedPreferences

object Preferences {
    @JvmStatic
    fun getDefaultSharedPreferences(context: Context): SharedPreferences =
        context.getSharedPreferences("${context.packageName}_preferences", Context.MODE_PRIVATE)
}
