/*
 * @file
 * @brief Parcelable implementation of Playback.Status
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.rpc;

import java.util.concurrent.TimeUnit;

import android.os.Parcel;
import android.os.Parcelable;
import app.zxtune.Playback;
import app.zxtune.TimeStamp;

public final class ParcelablePlaybackStatus implements Playback.Status, Parcelable {
  public static final Parcelable.Creator<ParcelablePlaybackStatus> CREATOR =
      new Parcelable.Creator<ParcelablePlaybackStatus>() {
        @Override
        public ParcelablePlaybackStatus createFromParcel(Parcel in) {
          return in.dataAvail() != 0 ? new ParcelablePlaybackStatus(in) : null;
        }

        @Override
        public ParcelablePlaybackStatus[] newArray(int size) {
          return new ParcelablePlaybackStatus[size];
        }
      };

  private final TimeStamp position;
  private final boolean paused;

  public ParcelablePlaybackStatus(Playback.Status status) {
    this.position = status.getPosition();
    this.paused = status.isPaused();
  }

  private ParcelablePlaybackStatus(Parcel in) {
    position = new TimeStamp(in.readLong(), TimeUnit.MILLISECONDS);
    paused = in.readInt() != 0;
  }

  @Override
  public TimeStamp getPosition() {
    return position;
  }

  @Override
  public boolean isPaused() {
    return paused;
  }

  @Override
  public int describeContents() {
    return 0;
  }

  @Override
  public void writeToParcel(Parcel dest, int flags) {
    dest.writeLong(position.convertTo(TimeUnit.MILLISECONDS));
    dest.writeInt(paused ? 1 : 0);
  }
}
