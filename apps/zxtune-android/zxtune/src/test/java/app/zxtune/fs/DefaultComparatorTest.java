package app.zxtune.fs;

import org.junit.Test;

import static org.junit.Assert.assertEquals;

public class DefaultComparatorTest {

  @Test
  public void testAlpha() {
    final String[] DATA = {
            "",
            "ABCD",
            "abcd",
            "ABCDE",
            "abcde",
            "CDEF",
            "cdef",
            "CDEFG",
            "cdefg"
    };

    assertSorted(DATA);
  }

  @Test
  public void testNumeric() {
    final String[] DATA = {
            "00",
            "0",
            "001",
            "01",
            "1",
            "05",
            "5",
            "0010",
            "010",
            "10",
            "0100",
            "100"
    };

    assertSorted(DATA);
  }

  @Test
  public void testNumericWithPrefix() {
    final String[] DATA = {
            "prefix00",
            "prefix0",
            "prefix001",
            "prefix01",
            "prefix1",
            "prefix05",
            "prefix5",
            "prefix0010",
            "prefix010",
            "prefix10",
            "prefix0100",
            "prefix100"
    };

    assertSorted(DATA);
  }

  @Test
  public void testNumericWithSuffix() {
    final String[] DATA = {
            "00suffix",
            "0suffix",
            "001suffix",
            "01suffix",
            "1suffix",
            "05suffix",
            "5suffix",
            "0010suffix",
            "010suffix",
            "10suffix",
            "0100suffix",
            "100suffix"
    };

    assertSorted(DATA);
  }

  @Test
  public void testNumericWithPrefixSuffix() {
    final String[] DATA = {
            "prefix00suffix",
            "prefix0suffix",
            "prefix001suffix",
            "prefix01suffix",
            "prefix1suffix",
            "prefix05suffix",
            "prefix5suffix",
            "prefix0010suffix",
            "prefix010suffix",
            "prefix10suffix",
            "prefix0100suffix",
            "prefix100suffix"
    };

    assertSorted(DATA);
  }

  @Test
  public void testBorderCases() {
    final String[] DATA = {
            "a",
            "ab",
            "ab00",
            "ab0",
            "ab00cd",
            "ab0cd",
            "ab01",
            "cd"
    };

    assertSorted(DATA);
  }

  @Test
  public void testBigIntegers() {
    final String[] DATA = {
            "359032080783004",
            "15747738266091545163",
            "23472943879812349871293841237687263750123984102341237461235123"
    };

    assertSorted(DATA);
  }

  @Test
  public void testCase() {
    final String[] DATA = {
        "cap",
        "Capital.2016",
        "capital.2015",
        "Datum"
    };

    assertSorted(DATA);
  }

  private static void assertSorted(String[] data) {
    final int len = data.length;
    for (int i = 0; i < len; ++i) {
      for (int j = i + 1; j < len; ++j) {
        assertOrdered(data[i], data[j]);
      }
    }
  }

  private static void assertOrdered(String lh, String rh) {
    assertEquals(lh + " < " + rh, -1,  DefaultComparator.compare(lh, rh));
    assertEquals(rh + " > " + lh, 1, DefaultComparator.compare(rh, lh));
    assertEquals(lh + " == " + lh, 0, DefaultComparator.compare(lh, lh));
    assertEquals(rh + " == " + rh, 0, DefaultComparator.compare(rh, rh));
  }
}
