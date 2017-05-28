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
import android.app.Notification;
import android.app.NotificationManager;
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
import android.support.annotation.Nullable;
import android.widget.Toast;

import java.io.File;
import java.util.Locale;
import java.util.concurrent.TimeUnit;

import app.zxtune.playback.FileIterator;
import app.zxtune.playback.PlayableItem;
import app.zxtune.sound.PlayerEventsListener;
import app.zxtune.sound.SamplesSource;
import app.zxtune.sound.StubPlayerEventsListener;
import app.zxtune.sound.SyncPlayer;
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
    handler.post(new Runnable() {
      @Override
      public void run() {
        Toast.makeText(RingtoneService.this, text, duration).show();
      }
    });
  }
  
  private void showNotification(String moduleTitle) {
    final Notification.Builder builder = new Notification.Builder(this);
    builder.setOngoing(false);
    builder.setSmallIcon(R.drawable.ic_stat_notify_ringtone);
    builder.setContentTitle(getString(R.string.ringtone_changed));
    builder.setContentText(moduleTitle);
    final NotificationManager manager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
    manager.notify(RingtoneService.class.hashCode(), builder.getNotification());
  }
  
  @Override
  protected void onHandleIntent(Intent intent) {
    if (ACTION_MAKERINGTONE.equals(intent.getAction())) {
      final Uri module = intent.getParcelableExtra(EXTRA_MODULE);
      final long seconds = intent.getLongExtra(EXTRA_DURATION_SECONDS, DEFAULT_DURATION_SECONDS);
      final TimeStamp duration = TimeStamp.createFrom(seconds, TimeUnit.SECONDS);
      createRingtone(module, duration);
    }
  }
  
  private void createRingtone(Uri source, TimeStamp howMuch) {
    try {
      final PlayableItem item = load(source);
      try {
        final File target = getTargetLocation(item, howMuch); 
        convert(item, howMuch, target);
        final String title = setAsRingtone(item, howMuch, target);
        showNotification(title);
        Analytics.sendSocialEvent("Ringtone", "app.zxtune", item);
      } finally {
        item.release();
      }
    } catch (Exception e) {
      Log.w(TAG, e, "Failed to create ringtone");
      makeToast(e);
    }
  }

  private PlayableItem load(Uri uri) throws Exception {
    final FileIterator iter = new FileIterator(this, new Uri[] {uri});
    return iter.getItem();
  }
    
  private File getTargetLocation(PlayableItem item, TimeStamp duration) throws Exception {
    final File dir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_RINGTONES);
    if (dir.mkdirs()) {
      Log.d(TAG, "Created ringtones directory");
    }
    final long moduleId = getModuleId(item);
    final String filename = String.format(Locale.US, "%d_%d.wav", moduleId, duration.convertTo(TimeUnit.SECONDS)); 
    Log.d(TAG, "Dir: %s filename: %s", dir, filename);
    return new File(dir, filename);
  }
  
  private static long getModuleId(PlayableItem item) throws Exception {
    return item.getModule().getProperty("CRC"/*ZXTune.Module.Attributes.CRC*/, item.getDataId().hashCode());
  }
  
  //TODO: rework errors processing scheme
  private class NotifyEventsListener extends StubPlayerEventsListener {
    @Override
    public void onError(Exception e) {
      makeToast(e);
    }
  }; 
  
  private void convert(PlayableItem item, TimeStamp limit, File location)  throws Exception {
    makeToast(getString(R.string.ringtone_create_started), Toast.LENGTH_SHORT);
    final TimeLimitedSamplesSource source = new TimeLimitedSamplesSource(item.getModule().createPlayer(), limit);
    final WaveWriteSamplesTarget target = new WaveWriteSamplesTarget(location.getAbsolutePath());
    final PlayerEventsListener events = new NotifyEventsListener(); 
    final SyncPlayer player = new SyncPlayer(source, target, events);
    try {
      player.startPlayback();
    } finally {
      player.release();
    }
  }
  
  private String setAsRingtone(PlayableItem item, TimeStamp limit, File path) throws Exception {
    final ContentValues values = createRingtoneData(item, limit, path);

    final Uri ringtoneUri = createOrUpdateRingtone(values);
    
    RingtoneManager.setActualDefaultRingtoneUri(this, RingtoneManager.TYPE_RINGTONE, ringtoneUri);

    return values.getAsString(MediaStore.MediaColumns.TITLE);
  }
  
  private static ContentValues createRingtoneData(PlayableItem item, TimeStamp limit, File path) throws Exception {
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

    private ZXTune.Player player;
    private final TimeStamp limit;
    private int restSamples;
    
    public TimeLimitedSamplesSource(ZXTune.Player player, TimeStamp limit) throws Exception {
      this.player = player;
      this.limit = limit;
      player.setPosition(0);
      player.setProperty(ZXTune.Properties.Sound.LOOPED, 1);
    }
    
    @Override
    public void initialize(int sampleRate) throws Exception {
      player.setProperty(ZXTune.Properties.Sound.FREQUENCY, sampleRate);
      restSamples = (int) (limit.convertTo(TimeUnit.SECONDS) * sampleRate);
    }

    @Override
    public boolean getSamples(short[] buf) throws Exception {
      if (restSamples > 0 && player.render(buf)) {
        restSamples -= buf.length / SamplesSource.Channels.COUNT;
        return true;
      } else {
        return false;
      }
    }

    @Override
    public void release() {
      player.release();
      player = null;
    }
  }
}
