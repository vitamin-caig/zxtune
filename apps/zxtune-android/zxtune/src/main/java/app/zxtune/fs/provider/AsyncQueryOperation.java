package app.zxtune.fs.provider;

import android.database.Cursor;

import androidx.annotation.Nullable;

import java.util.concurrent.Callable;

interface AsyncQueryOperation extends Callable<Cursor> {

  @Nullable
  Cursor call() throws Exception;

  @Nullable
  Cursor status();
}
