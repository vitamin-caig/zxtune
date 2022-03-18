package app.zxtune.fs.api

import android.util.Base64
import androidx.annotation.VisibleForTesting
import app.zxtune.BuildConfig
import app.zxtune.auth.Auth
import app.zxtune.net.Http
import java.io.IOException
import java.net.HttpURLConnection

private const val ENDPOINT = BuildConfig.API_ROOT + "/"
private const val REPLY = ""

object Api {
    @VisibleForTesting
    var authorization: String? = null

    @JvmStatic
    fun initialize(auth: Auth.UserInfo) = setAuthorization(auth.identifier, "")

    @VisibleForTesting
    fun setAuthorization(name: String, password: String) {
        val credentials = "$name:$password"
        authorization = "Basic ${Base64.encodeToString(credentials.toByteArray(), Base64.NO_WRAP)}"
    }

    @Throws(IOException::class)
    @JvmStatic
    fun postEvent(url: String) {
        val reply = doRequest("${ENDPOINT}events/$url", "POST")
        if (REPLY != reply) {
            throw IOException("Wrong reply: $reply")
        }
    }

    private fun doRequest(fullUrl: String, method: String) = with(Http.createConnection(fullUrl)) {
        authorization?.let {
            setRequestProperty("Authorization", it)
        }
        requestMethod = method
        try {
            connect()
            if (responseCode != HttpURLConnection.HTTP_OK) {
                throw IOException(responseMessage)
            }
            inputStream.use { input ->
                val realUrl = url
                // Allow redirects to the same origin
                if (!realUrl.toString().startsWith(ENDPOINT)) {
                    throw IOException("Unexpected redirect: $fullUrl -> $realUrl")
                }
                input.bufferedReader().use { it.readText() }
            }
        } finally {
            disconnect()
        }
    }
}
