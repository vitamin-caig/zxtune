package app.zxtune.net

import android.os.Build
import androidx.annotation.RawRes
import app.zxtune.BuildConfig
import app.zxtune.Log
import app.zxtune.MainApplication
import app.zxtune.R
import java.io.IOException
import java.net.HttpURLConnection
import java.net.URL
import java.security.KeyStore
import java.security.cert.CertificateFactory
import java.security.cert.X509Certificate
import java.util.Locale
import javax.net.ssl.HttpsURLConnection
import javax.net.ssl.SSLContext
import javax.net.ssl.SSLSocketFactory
import javax.net.ssl.TrustManagerFactory

object Http {
    private val USER_AGENT = String.format(
        Locale.US,
        "%s/%d (%s; %s; %s; %s)",
        BuildConfig.APPLICATION_ID,
        BuildConfig.VERSION_CODE,
        BuildConfig.BUILD_TYPE,
        Build.CPU_ABI,
        BuildConfig.FLAVOR_packaging,
        BuildConfig.FLAVOR_api
    )

    init {
        if (Build.VERSION.SDK_INT < 25) {
            HttpsURLConnection.setDefaultSSLSocketFactory(CompatSSLContext.socketFactory)
        }
    }

    @Throws(IOException::class)
    fun createConnection(uri: String?) = (URL(uri).openConnection() as HttpURLConnection).apply {
        setRequestProperty("User-Agent", USER_AGENT)
    }
}

private object CompatSSLContext {
    private val context: SSLContext by lazy {
        val tmf = TrustManagerFactory.getInstance(TrustManagerFactory.getDefaultAlgorithm()).apply {
            init(createCustomKeyStore())
        }
        SSLContext.getInstance("TLS").apply {
            init(
                null, tmf.trustManagers, null
            )
        }
    }

    val socketFactory: SSLSocketFactory
        get() = context.socketFactory
}

private fun createCustomKeyStore() = KeyStore.getInstance(KeyStore.getDefaultType()).apply {
    load(null)
    if (addCustomCA("ISRG Root X1", R.raw.isrgrootx1)) {
        addSupportedCAs()
    }
}

private fun KeyStore.addCustomCA(alias: String, @RawRes id: Int): Boolean {
    if (!containsAlias(alias)) {
        loadCustomCert(id).apply {
            Log.d("SSL", "addCustomCA(%s) = %s", alias, subjectX500Principal.name)
            setCertificateEntry(alias, this)
        }
        return true
    }
    Log.d("SSL", "Already exists %s", alias)
    return false
}

private fun loadCustomCert(@RawRes id: Int) =
    MainApplication.getGlobalContext().resources.openRawResource(id).use {
        CertificateFactory.getInstance("X.509").generateCertificate(it) as X509Certificate
    }

private fun KeyStore.addSupportedCAs() = KeyStore.getInstance("AndroidCAStore").let { system ->
    system.load(null)
    for (alias in system.aliases()) {
        val cert = system.getCertificate(alias)
        Log.d(
            "SSL",
            "addSystemCA(%s)",
            (cert as? X509Certificate)?.subjectX500Principal?.name ?: cert.toString()
        )
        setCertificateEntry(alias, cert)
    }
}
