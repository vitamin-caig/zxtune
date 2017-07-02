/**
 *
 * @file
 *
 * @brief Playlist item state abstraction
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.playlist;

import android.content.ContentValues;
import android.database.Cursor;

public class ItemState {

    public static final Database.Tables.Playlist.Fields COLUMN = Database.Tables.Playlist.Fields.state;

    private final boolean isPlaying;

    public ItemState(boolean isPlaying) {
        this.isPlaying = isPlaying;
    }

    public ItemState(Cursor cursor) {
        this.isPlaying = 0 != cursor.getInt(COLUMN.ordinal());
    }

    public final boolean isPlaying() {
        return isPlaying;
    }

    public ContentValues toContentValues() {
        final ContentValues values = new ContentValues();
        values.put(Database.Tables.Playlist.Fields.state.name(), isPlaying ? 1 : 0);
        return values;
    }
}
