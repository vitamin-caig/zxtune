/**
 * 
 * @file
 *
 * @brief
 *
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune;

import android.app.IntentService;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.media.RingtoneManager;
import android.net.Uri;
import android.os.Environment;
import android.os.Handler;
import android.provider.MediaStore;
import android.widget.Toast;

import androidx.annotation.Nullable;

import java.io.File;
import java.util.Locale;
import java.util.concurrent.TimeUnit;

import app.zxtune.analytics.Analytics;
import app.zxtune.core.Module;
import app.zxtune.core.Player;
import app.zxtune.core.Properties;
import app.zxtune.device.ui.Notifications;
import app.zxtune.playback.FileIterator;
import app.zxtune.playback.PlayableItem;
import app.zxtune.sound.AsyncPlayer;
import app.zxtune.sound.PlayerEventsListener;
import app.zxtune.sound.SamplesSource;
import app.zxtune.sound.StubPlayerEventsListener;
import app.zxtune.sound.WaveWriteSamplesTarget;

public class RingtoneService extends IntentService {

  private static final String TAG = RingtoneService.class.getName();
  
  private static final String ACTION_MAKERINGTONE = TAG + ".makeringtone";
  private static final String EXTRA_MODULE = "module";
  private static final String EXTRA_DURATION_SECONDS = "duration";
  private static final long DEFAULT_DURATION_SECONDS = 30;
  
  private final Handler handler;
  
  public RingtoneService() {
    super(RingtoneService.class.getName());
    this.handler = new Handler();
    setIntentRedelivery(false);
  }
  
  public static void execute(Context context, Uri module, TimeStamp duration) {
    final Intent intent = new Intent(context, RingtoneService.class);
    intent.setAction(ACTION_MAKERINGTONE);
    intent.putExtra(EXTRA_MODULE, module);
    intent.putExtra(EXTRA_DURATION_SECONDS, duration.convertTo(TimeUnit.SECONDS));
    context.startService(intent);
  }
  
  private void makeToast(Exception e) {
    Log.w(TAG, e, "Failed to create ringtone");
    final Throwable cause = e.getCause();
    final String msg = cause != null ? cause.getMessage() : e.getMessage();
    final String txt = getString(R.string.ringtone_creating_failed, msg);
    makeToast(txt, Toast.LENGTH_LONG);
  }

  private void makeToast(final String text, final int duration) {
    handler.post(() -> Toast.makeText(RingtoneService.this, text, duration).show());
  }

  @Override
  protected void onHandleIntent(@Nullable Intent intent) {
    if (intent != null && ACTION_MAKERINGTONE.equals(intent.getAction())) {
      final Uri module = intent.getParcelableExtra(EXTRA_MODULE);
      final long seconds = intent.getLongExtra(EXTRA_DURATION_SECONDS, DEFAULT_DURATION_SECONDS);
      final TimeStamp duration = TimeStamp.createFrom(seconds, TimeUnit.SECONDS);
      createRingtone(module, duration);
    }
  }
  
  private void createRingtone(Uri source, TimeStamp howMuch) {
    try {
      final PlayableItem item = load(source);
      final File target = getTargetLocation(item, howMuch);
      convert(item, howMuch, target);
      final String title = setAsRingtone(item, howMuch, target);
      Notifications.sendEvent(this, R.drawable.ic_stat_notify_ringtone, R.string.ringtone_changed, title);
      Analytics.sendSocialEvent(source,"app.zxtune", Analytics.SOCIAL_ACTION_RINGTONE);
    } catch (Exception e) {
      Log.w(TAG, e, "Failed to create ringtone");
      makeToast(e);
    }
  }

  private PlayableItem load(Uri uri) throws Exception {
    final FileIterator iter = FileIterator.create(this, uri);
    return iter.getItem();
  }
    
  private File getTargetLocation(PlayableItem item, TimeStamp duration) {
    final File dir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_RINGTONES);
    if (dir.mkdirs()) {
      Log.d(TAG, "Created ringtones directory");
    }
    final long moduleId = getModuleId(item);
    final String filename = String.format(Locale.US, "%d_%d.wav", moduleId, duration.convertTo(TimeUnit.SECONDS)); 
    Log.d(TAG, "Dir: %s filename: %s", dir, filename);
    return new File(dir, filename);
  }
  
  private static long getModuleId(PlayableItem item) {
    return item.getModule().getProperty("CRC"/*ZXTune.Module.Attributes.CRC*/, item.getDataId().hashCode());
  }
  
  //TODO: rework errors processing scheme
  private class NotifyEventsListener extends StubPlayerEventsListener {
    @Override
    public void onError(Exception e) {
      makeToast(e);
    }

    @Override
    public void onFinish() {
      synchronized (this) {
        notifyAll();
      }
    }
  }
  
  private void convert(PlayableItem item, TimeStamp limit, File location)  throws Exception {
    makeToast(getString(R.string.ringtone_create_started), Toast.LENGTH_SHORT);
    final WaveWriteSamplesTarget target = new WaveWriteSamplesTarget(location.getAbsolutePath());
    final TimeLimitedSamplesSource source = new TimeLimitedSamplesSource(item.getModule(), target.getSampleRate(), limit);
    final PlayerEventsListener events = new NotifyEventsListener();
    final app.zxtune.sound.Player player = AsyncPlayer.create(target, events);
    try {
      player.setSource(source);
      synchronized (events) {
        player.startPlayback();
        events.wait();
        player.stopPlayback();
      }
    } finally {
      player.release();
    }
  }
  
  private String setAsRingtone(PlayableItem item, TimeStamp limit, File path) {
    final ContentValues values = createRingtoneData(item, limit, path);

    final Uri ringtoneUri = createOrUpdateRingtone(values);
    
    RingtoneManager.setActualDefaultRingtoneUri(this, RingtoneManager.TYPE_RINGTONE, ringtoneUri);

    return values.getAsString(MediaStore.MediaColumns.TITLE);
  }
  
  private static ContentValues createRingtoneData(PlayableItem item, TimeStamp limit, File path) {
    final ContentValues values = new ContentValues();
    values.put(MediaStore.MediaColumns.DATA, path.getAbsolutePath());
    values.put(MediaStore.MediaColumns.MIME_TYPE, "audio/wav");
    final String filename = item.getDataId().getDisplayFilename();
    values.put(MediaStore.MediaColumns.DISPLAY_NAME, filename);
    values.put(MediaStore.Audio.Media.DURATION, limit.convertTo(TimeUnit.MILLISECONDS));
    final String title = Util.formatTrackTitle(item.getAuthor(), item.getTitle(), filename);
    final String name = String.format(Locale.US, "%s (%s)", title, limit);
    values.put(MediaStore.MediaColumns.TITLE, name);

    values.put(MediaStore.Audio.Media.IS_RINGTONE, true);
    values.put(MediaStore.Audio.Media.IS_NOTIFICATION, false);
    values.put(MediaStore.Audio.Media.IS_ALARM, false);
    values.put(MediaStore.Audio.Media.IS_MUSIC, false);
    return values;
  }

  @Nullable
  private Uri createOrUpdateRingtone(ContentValues values) {
    final ContentResolver resolver = getContentResolver();
    final String path = values.getAsString(MediaStore.MediaColumns.DATA);
    final Uri tableUri = MediaStore.Audio.Media.getContentUriForPath(path);
    final Cursor query = resolver.query(tableUri,
        new String[] {MediaStore.MediaColumns._ID},
        MediaStore.MediaColumns.DATA + " = '" + path + "'", null,
        null);
    Uri ringtoneUri;
    try {
      if (query != null && query.moveToFirst()) {
        final long id = query.getLong(0);
        ringtoneUri = ContentUris.withAppendedId(tableUri, id);
        if (0 != resolver.update(ringtoneUri, values,
            MediaStore.Audio.Media.IS_RINGTONE + " = 1",
            null)) {
          Log.d(TAG, "Updated ringtone at %s", ringtoneUri);
        } else {
          Log.d(TAG, "Failed to update ringtone %s", ringtoneUri);
        }
      } else {
        ringtoneUri = resolver.insert(tableUri, values);
        Log.d(TAG, "Registered new ringtone at %s", ringtoneUri);
      }
    } finally {
      if (query != null) {
        query.close();
      }
    }
    return ringtoneUri;
  }
  
  private static class TimeLimitedSamplesSource implements SamplesSource {

    private final Player player;
    private final TimeStamp limit;
    private int restSamples;
    
    TimeLimitedSamplesSource(Module module, int sampleRate, TimeStamp limit) {
      this.player = module.createPlayer(sampleRate);
      this.limit = limit;
      restSamples = (int) (limit.convertTo(TimeUnit.SECONDS) * sampleRate);
      player.setPosition(TimeStamp.EMPTY);
      player.setProperty(Properties.Sound.LOOPED, 1);
    }
    
    @Override
    public boolean getSamples(short[] buf) {
      if (restSamples > 0 && player.render(buf)) {
        restSamples -= buf.length / SamplesSource.Channels.COUNT;
        return true;
      } else {
        player.setPosition(TimeStamp.EMPTY);
        return false;
      }
    }

    @Override
    public void setPosition(TimeStamp pos) {}

    @Override
    public TimeStamp getPosition() {
      return TimeStamp.EMPTY;
    }
  }
}
