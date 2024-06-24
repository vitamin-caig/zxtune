/**
 *
 * @file
 *
 * @brief Test for Analysis::Path
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include <analysis/path.h>
#include <iostream>

namespace
{
  void Test(bool res, const String& text)
  {
    std::cout << (res ? "Passed" : "Failed") << " test '" << text << "' " << std::endl;
    if (!res)
    {
      throw 1;
    }
  }

  void TestPath(const String& context, const Analysis::Path& path, const String& val, const String& first = String(),
                const String& second = String(), const String& third = String())
  {
    Test(path.Empty() == val.empty(), context + " Empty");
    const String asStr = path.AsString();
    Test(asStr == val, context + " AsString");
    const auto elems = path.Elements();
    Test(!elems.empty() == !val.empty(), context + " Elements");
    Test(elems.size() == std::size_t(!first.empty() + !second.empty() + !third.empty()), "  size()");
    Test(first.empty() || (elems[0] == first), "  [0]");
    Test(second.empty() || (elems[1] == second), "  [1]");
    Test(third.empty() || (elems[2] == third), "  [2]");
  }

  const String EMPTY_PATH = String();
  const String FIRST_ELEMENT("first");
  const String SECOND_ELEMENT("second");
  const String THIRD_ELEMENT("third");
  const String SINGLE_PATH(FIRST_ELEMENT);
  const String DOUBLE_PATH(FIRST_ELEMENT + '/' + SECOND_ELEMENT);
  const String TREBLE_PATH(FIRST_ELEMENT + '/' + SECOND_ELEMENT + '/' + THIRD_ELEMENT);
}  // namespace

int main()
{
  try
  {
    {
      const Analysis::Path::Ptr empty = Analysis::ParsePath(EMPTY_PATH, '/');
      TestPath("empty path", *empty, EMPTY_PATH);
      Test(!empty->GetParent(), "Empty path parent");
      const Analysis::Path::Ptr emptyPlusNotEmpty = empty->Append(FIRST_ELEMENT);
      TestPath("empty path plus not empty", *emptyPlusNotEmpty, SINGLE_PATH, FIRST_ELEMENT);
      const Analysis::Path::Ptr emptyPlusEmpty = empty->Append(EMPTY_PATH);
      TestPath("empty path plus empty", *emptyPlusEmpty, EMPTY_PATH);
      const Analysis::Path::Ptr emptyExtractEmpty = empty->Extract(EMPTY_PATH);
      TestPath("empty path extracts empty", *emptyExtractEmpty, EMPTY_PATH);
      const Analysis::Path::Ptr emptyExtractNotEmpty = empty->Extract(FIRST_ELEMENT);
      Test(!emptyExtractNotEmpty, "empty path extracts not empty");
    }
    {
      const Analysis::Path::Ptr single = Analysis::ParsePath(SINGLE_PATH, '/');
      TestPath("single element path", *single, SINGLE_PATH, FIRST_ELEMENT);
      TestPath("single element path parent", *single->GetParent(), EMPTY_PATH);
      const Analysis::Path::Ptr singlePlusNotEmpty = single->Append(SECOND_ELEMENT);
      TestPath("single path plus not empty", *singlePlusNotEmpty, DOUBLE_PATH, FIRST_ELEMENT, SECOND_ELEMENT);
      const Analysis::Path::Ptr singlePlusEmpty = single->Append(EMPTY_PATH);
      TestPath("single path plus empty", *singlePlusEmpty, SINGLE_PATH, FIRST_ELEMENT);
      const Analysis::Path::Ptr singleExtractEmpty = single->Extract(EMPTY_PATH);
      TestPath("single path extracts empty", *singleExtractEmpty, SINGLE_PATH, FIRST_ELEMENT);
      const Analysis::Path::Ptr singleExtractFirst = single->Extract(FIRST_ELEMENT);
      TestPath("single path extracts first element", *singleExtractFirst, EMPTY_PATH);
      const Analysis::Path::Ptr singleExtractsInvalid = single->Extract(SECOND_ELEMENT);
      Test(!singleExtractsInvalid, "single path extracts invalid");
    }
    {
      const Analysis::Path::Ptr dbl = Analysis::ParsePath(DOUBLE_PATH, '/');
      TestPath("double element path", *dbl, DOUBLE_PATH, FIRST_ELEMENT, SECOND_ELEMENT);
      TestPath("double element path parent", *dbl->GetParent(), SINGLE_PATH, FIRST_ELEMENT);
      const Analysis::Path::Ptr dblPlusNotEmpty = dbl->Append(THIRD_ELEMENT);
      TestPath("double path plus not empty", *dblPlusNotEmpty, TREBLE_PATH, FIRST_ELEMENT, SECOND_ELEMENT,
               THIRD_ELEMENT);
      const Analysis::Path::Ptr dblPlusEmpty = dbl->Append(EMPTY_PATH);
      TestPath("double path plus empty", *dblPlusEmpty, DOUBLE_PATH, FIRST_ELEMENT, SECOND_ELEMENT);
      const Analysis::Path::Ptr dblExtractEmpty = dbl->Extract(EMPTY_PATH);
      TestPath("double path extracts empty", *dblExtractEmpty, DOUBLE_PATH, FIRST_ELEMENT, SECOND_ELEMENT);
      const Analysis::Path::Ptr dblExtractFirst = dbl->Extract(FIRST_ELEMENT);
      TestPath("double path extracts first element", *dblExtractFirst, SECOND_ELEMENT, SECOND_ELEMENT);
      const Analysis::Path::Ptr dblExtractsInvalid = dbl->Extract(SECOND_ELEMENT);
      Test(!dblExtractsInvalid, "double path extracts invalid");
    }
  }
  catch (int val)
  {
    return val;
  }
}
