/**
 *
 * @file
 *
 * @brief  String sanitizing implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// library includes
#include <strings/encoding.h>
#include <strings/sanitize.h>
#include <strings/split.h>
#include <strings/trim.h>
// std includes
#include <vector>

namespace Strings
{
  namespace
  {
    constexpr char LINE_BREAK = '\n';
    constexpr char SPACE = ' ';

    bool IsPrintable(uint8_t c)
    {
      return c >= ' ' && c != 0xff;
    }

    // Makes utf8
    // - '\t' -> ' '
    // - '\r', '\n', "\r\n" -> endlReplacement
    // - all controls -> ctrlReplacement
    String Prepare(StringView str, char endlReplacement, char ctrlReplacement)
    {
      auto utf8 = ToAutoUtf8(str);
      bool wasCr = false;
      std::size_t out = 0;
      for (const auto in : utf8)
      {
        const auto isCr = in == '\r';
        const auto isLn = in == '\n';
        const auto isTab = in == '\t';
        const auto isSym = IsPrintable(in);
        if (isCr || isLn)
        {
          if (isLn && wasCr)
          {
            wasCr = false;
            continue;
          }
          utf8[out++] = endlReplacement;
          wasCr = isCr;
          continue;
        }
        wasCr = false;
        if (isTab)
        {
          utf8[out++] = SPACE;
        }
        else if (isSym)
        {
          utf8[out++] = in;
        }
        else if (ctrlReplacement)
        {
          utf8[out++] = ctrlReplacement;
        }
      }
      utf8.resize(out);
      return utf8;
    }

    class StringBuilder
    {
    public:
      explicit StringBuilder(String& out)
        : Result(out)
      {}

      void Add(StringView str)
      {
        auto* target = &Result[End];
        if (str.size() && target != str.data())
        {
          std::copy(str.begin(), str.end(), target);
        }
        End += str.size();
      }

      void AddEndl()
      {
        if (End)
        {
          Result[End++] = LINE_BREAK;
        }
      }

      String Finish()
      {
        while (End && Result[End - 1] == LINE_BREAK)
        {
          --End;
        }
        Result.resize(End);
        return std::move(Result);
      }

    private:
      String& Result;
      std::size_t End = 0;
    };
  }  // namespace

  String Sanitize(StringView str)
  {
    auto out = Prepare(str, SPACE, 0);
    if (!out.empty() && (out.front() == SPACE || out.back() == SPACE))
    {
      const auto trimmed = TrimSpaces(out);
      std::copy(trimmed.begin(), trimmed.end(), out.begin());
      out.resize(trimmed.size());
    }
    return out;
  }

  String SanitizeMultiline(StringView str)
  {
    if (StringView::npos == str.find_first_of("\r\n"))
    {
      return Sanitize(str);
    }
    auto out = Prepare(str, LINE_BREAK, 0);
    std::vector<StringViewCompat> substrings;
    Split(out, LINE_BREAK, substrings);
    StringBuilder builder(out);
    for (const auto& sub : substrings)
    {
      builder.Add(TrimSpaces(sub));
      builder.AddEndl();
    }
    return builder.Finish();
  }

  String SanitizeKeepPadding(StringView str)
  {
    auto out = Prepare(str, SPACE, SPACE);
    std::size_t end = out.size();
    while (end && out[end - 1] == SPACE)
    {
      --end;
    }
    out.resize(end);
    return out;
  }
}  // namespace Strings
