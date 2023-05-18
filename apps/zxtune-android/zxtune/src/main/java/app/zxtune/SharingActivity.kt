package app.zxtune

import android.app.Activity
import android.content.Context
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.support.v4.media.MediaDescriptionCompat
import android.support.v4.media.MediaMetadataCompat
import androidx.activity.ComponentActivity
import androidx.activity.result.contract.ActivityResultContract
import app.zxtune.analytics.Analytics
import app.zxtune.fs.VfsExtensions
import app.zxtune.utils.ifNotNulls

/*
 Sharing-related logic
 Call chain:
 client -(via explicit intent)->
 SharingActivity -(via ACTION_PICK_ACTIVITY) ->
 ActivityPicker -(via activity result)->
 SharingActivity -(via startActivity)->
 Target
 */
class SharingActivity : ComponentActivity() {

    companion object {
        @JvmStatic
        fun maybeCreateShareIntent(ctx: Context, metadata: MediaMetadataCompat) =
            metadata.shareUrl?.let { shareUrl ->
                makeSendIntent("text/plain", metadata.description)
                    .apply {
                        putExtra(Intent.EXTRA_TEXT, ctx.getString(R.string.share_text, shareUrl))
                    }.let {
                        wrap(ctx, metadata.description, it)
                    }
            }

        @JvmStatic
        fun maybeCreateSendIntent(ctx: Context, metadata: MediaMetadataCompat) =
            metadata.contentUrl?.let { contentUrl ->
                makeSendIntent("application/octet", metadata.description).apply {
                    putExtra(
                        Intent.EXTRA_TEXT,
                        ctx.getString(R.string.send_text, metadata.shareUrl ?: "")
                    )
                    putExtra(Intent.EXTRA_STREAM, Uri.parse(contentUrl))
                    addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
                }.let {
                    wrap(ctx, metadata.description, it)
                }
            }

        private fun makeSendIntent(mimeType: String, description: MediaDescriptionCompat) =
            Intent(Intent.ACTION_SEND).apply {
                type = mimeType
                putExtra(Intent.EXTRA_SUBJECT, description.title)
            }

        private fun wrap(ctx: Context, description: MediaDescriptionCompat, intent: Intent) =
            Intent(ctx, SharingActivity::class.java).apply {
                data = description.mediaUri
                putExtra(Intent.EXTRA_INTENT, intent)
            }
    }

    private val request = registerForActivityResult(ActivityPickerContract()) { data ->
        if (data != null) {
            ifNotNulls(
                intent?.data,
                data.component?.packageName,
                data.extras?.let { guessSocialAction(it) },
                Analytics::sendSocialEvent
            )
            data.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            startActivity(data)
        }
        finish()
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        intent?.extras?.getParcelable<Intent>(Intent.EXTRA_INTENT)?.let {
            request.launch(it)
        }
    }
}

private class ActivityPickerContract : ActivityResultContract<Intent?, Intent?>() {

    override fun createIntent(context: Context, input: Intent?) =
        Intent(Intent.ACTION_PICK_ACTIVITY).apply {
            if (input != null) {
                input.getStringExtra(Intent.EXTRA_SUBJECT)?.let {
                    putExtra(Intent.EXTRA_TITLE, it)
                }
                putExtra(Intent.EXTRA_INTENT, input)
            }
        }

    override fun parseResult(resultCode: Int, intent: Intent?) =
        intent.takeIf { resultCode == Activity.RESULT_OK }
}

private val MediaMetadataCompat.shareUrl
    get() = getString(VfsExtensions.SHARE_URL)

private val MediaMetadataCompat.contentUrl
    get() = getString(VfsExtensions.FILE)

private fun guessSocialAction(extra: Bundle) = when {
    extra.getParcelable<Uri>(Intent.EXTRA_STREAM) != null -> Analytics.SOCIAL_ACTION_SEND
    extra.getString(Intent.EXTRA_TEXT) != null -> Analytics.SOCIAL_ACTION_SHARE
    else -> null
}
