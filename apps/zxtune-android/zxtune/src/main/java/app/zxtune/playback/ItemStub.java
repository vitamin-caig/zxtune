/**
 *
 * @file
 *
 * @brief Stub singleton implementation of Item
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.playback;

import android.net.Uri;

import app.zxtune.Identifier;
import app.zxtune.TimeStamp;

public class ItemStub implements Item {

  private static final String EMPTY_STRING = "";
  
  @Override
  public Uri getId() {
    return Uri.EMPTY;
  }

  @Override
  public Identifier getDataId() {
    return Identifier.EMPTY;
  }

  @Override
  public String getTitle() {
    return EMPTY_STRING;
  }

  @Override
  public String getAuthor() {
    return EMPTY_STRING;
  }
  
  @Override
  public String getProgram() {
    return EMPTY_STRING;
  }
  
  @Override
  public String getComment() {
    return EMPTY_STRING;
  }

  @Override
  public String getStrings() {
    return EMPTY_STRING;
  }
  
  @Override
  public TimeStamp getDuration() {
    return TimeStamp.EMPTY;
  }

  public static Item instance() {
    return Holder.INSTANCE;
  }

  //onDemand holder idiom
  private static class Holder {
    public static final Item INSTANCE = new ItemStub();
  }  
}
