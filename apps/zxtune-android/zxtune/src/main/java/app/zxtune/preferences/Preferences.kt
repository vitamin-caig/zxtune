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

    private lateinit var providerClient: ProviderClient
    private lateinit var dataStore: DataStore

    @JvmStatic
    fun getDefaultSharedPreferences(context: Context): SharedPreferences =
        context.getSharedPreferences("${context.packageName}_preferences", Context.MODE_PRIVATE)

    fun getProviderClient(context: Context): ProviderClient {
        if (!this::providerClient.isInitialized) {
            providerClient = ProviderClient(context.applicationContext)
        }
        return providerClient
    }

    @JvmStatic
    fun getDataStore(context: Context): DataStore {
        if (!this::dataStore.isInitialized) {
            dataStore = DataStore(getProviderClient(context))
        }
        return dataStore
    }
}
