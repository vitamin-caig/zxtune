package app.zxtune.rpc;

import android.os.IBinder;
import android.os.Parcel;
import android.os.Parcelable;

// TODO: remove when minSdk >= 18, use Bundle.putBinder/getBinder
public class ParcelableBinder implements Parcelable {

  private final IBinder binder;

  private ParcelableBinder(IBinder binder) {
    this.binder = binder;
  }

  private ParcelableBinder(Parcel in) {
    this(in.readStrongBinder());
  }

  public static Parcelable serialize(IBinder binder) {
    return new ParcelableBinder(binder);
  }

  public static IBinder deserialize(Parcelable parcelable) {
    final ParcelableBinder result = (ParcelableBinder) parcelable;
    return result != null ? result.binder : null;
  }

  @Override
  public int describeContents() {
    return 0;
  }

  @Override
  public void writeToParcel(Parcel dest, int flags) {
    dest.writeStrongBinder(binder);
  }

  public static final Parcelable.Creator<ParcelableBinder> CREATOR = new Parcelable.Creator<ParcelableBinder>() {
    @Override
    public ParcelableBinder createFromParcel(Parcel in) {
      return new ParcelableBinder(in);
    }

    @Override
    public ParcelableBinder[] newArray(int size) {
      return new ParcelableBinder[size];
    }
  };
}
