/**
 *
 * @file
 *
 * @brief  ZXZip compressor support
 *
 * @author vitamin.caig@gmail.com
 *
 * @note   Based on XLook sources by HalfElf
 *
 **/

#include "formats/packed/common/container.h"
#include "formats/packed/common/utils.h"

#include "binary/crc.h"
#include "binary/data_builder.h"
#include "binary/format_factories.h"
#include "binary/input_stream.h"
#include "formats/packed/decoder.h"
#include "math/numeric.h"

#include "byteorder.h"
#include "contract.h"
#include "make_ptr.h"
#include "pointers.h"

#include <array>
#include <cassert>
#include <cstring>
#include <iterator>

namespace Formats::Packed::ZXZip
{
  const std::size_t MIN_SIZE = 0x16 + 32;

  const auto DESCRIPTION = "ZXZip"sv;
  const auto HEADER_PATTERN =
      // Filename
      "20-7a 20-7a 20-7a 20-7a 20-7a 20-7a 20-7a 20-7a"
      // Type
      "20-7a ??"
      // SourceSize
      "??"
      // SourceSectors
      "01-ff"
      // PackedSize
      "??"
      // SourceCRC
      "????"
      // Method
      "00-03"
      // Flags
      "%0000000x"
      ""sv;

  struct RawHeader
  {
    //+0x0
    char Name[8];
    //+0x8
    char Type;
    //+0x9
    le_uint16_t StartOrSize;
    //+0xb
    le_uint16_t SourceSize;
    //+0xd
    uint8_t SourceSectors;
    //+0xe
    le_uint16_t PackedSize;
    //+0x10
    le_uint32_t SourceCRC;
    //+0x14
    uint8_t Method;
    //+0x15
    uint8_t Flags;
    //+0x16
  };

  static_assert(sizeof(RawHeader) * alignof(RawHeader) == 0x16, "Invalid layout");

  class Parser
  {
  public:
    explicit Parser(Binary::View data)
      : Data(data)
    {}

    bool FastCheck() const
    {
      if (Data.Size() < sizeof(RawHeader))
      {
        return false;
      }
      const RawHeader& header = GetHeader();
      const std::size_t packedSize = header.PackedSize;
      if (!packedSize)
      {
        return false;
      }
      if (GetSourceFileSize() < packedSize)
      {
        return false;
      }
      return GetUsedSize() <= Data.Size();
    }

    const RawHeader& GetHeader() const
    {
      return *Data.As<RawHeader>();
    }

    std::size_t GetUsedSize() const
    {
      const RawHeader& header = GetHeader();
      return sizeof(header) + header.PackedSize;
    }

    std::size_t GetSourceFileSize() const
    {
      const auto& header = GetHeader();
      const uint_t calcSize = header.Type == 'B' || header.Type == 'b' ? uint_t(4 + header.StartOrSize)
                                                                       : uint_t(header.SourceSize);
      constexpr uint_t SECTOR_SIZE = 256;
      const auto calcSectors = Math::Align(calcSize, SECTOR_SIZE) / SECTOR_SIZE;
      return calcSectors == header.SourceSectors ? calcSize : 256 * header.SourceSectors;
    }

    Binary::View GetCompressedData() const
    {
      const auto& header = GetHeader();
      return Data.SubView(sizeof(header), header.PackedSize);
    }

  private:
    const Binary::View Data;
  };

  enum PackMethods
  {
    STORE = 0,
    LZPRESS = 1,
    SHRINK = 2,
    IMPLODE = 3
  };

  // ZXZip data is LSB-first; reading past the input end is a contract violation.
  class Bitstream
  {
  public:
    explicit Bitstream(Binary::View data)
      : Delegate(data)
    {}

    bool GetBit()
    {
      if ((Mask <<= 1) > 255)
      {
        Bits = Delegate.ReadByte();
        Mask = 0x1;
      }
      return 0 != (Bits & Mask);
    }

    uint_t GetBits(uint_t count)
    {
      uint_t result = 0;
      for (uint_t idx = 0; idx < count; ++idx)
      {
        result = result | (GetBit() << idx);
      }
      return result;
    }

  private:
    Binary::DataInputStream Delegate;
    uint_t Bits = 0;
    uint_t Mask = 128;
  };

  namespace Store
  {
    Binary::Container::Ptr Decode(const Parser& parser)
    {
      assert(STORE == parser.GetHeader().Method);
      return Binary::CreateContainer(parser.GetCompressedData());
    }
  }  // namespace Store

  namespace Implode
  {
    const uint8_t SFT_64_1[] = {
        0x07,                                     // size
        0x01, 0x13, 0x34, 0xE5, 0xF6, 0x96, 0xF7  // packed data llllbbbb
    };

    const uint8_t SFT_64_2[] = {0x10, 0x00, 0x12, 0x03, 0x24, 0x15, 0x36, 0x27, 0x38,
                                0x39, 0x6A, 0x7B, 0x4C, 0x9D, 0x6E, 0x1F, 0x09};

    const uint8_t SFT_64_3[] = {0x07, 0x12, 0x23, 0x14, 0xE5, 0xF6, 0x96, 0xF7};

    const uint8_t SFT_64_4[] = {0x0D, 0x01, 0x22, 0x23, 0x14, 0x15, 0x36, 0x37, 0x68, 0x89, 0x9A, 0xDB, 0x3C, 0x05};

    const uint8_t SFT_64_5[] = {0x07, 0x12, 0x13, 0x44, 0xC5, 0xF6, 0x96, 0xF7};

    const uint8_t SFT_64_6[] = {0x0E, 0x02, 0x01, 0x12, 0x23, 0x14, 0x15, 0x36,
                                0x37, 0x68, 0x89, 0x9A, 0xDB, 0x3C, 0x05};

    const uint8_t SFT_100[] = {0x62, 0x0A, 0x7B, 0x07, 0x06, 0x1B, 0x06, 0xBB, 0x0C, 0x4B, 0x03, 0x09, 0x07, 0x0B, 0x09,
                               0x0B, 0x09, 0x07, 0x16, 0x07, 0x08, 0x06, 0x05, 0x06, 0x07, 0x06, 0x05, 0x36, 0x07, 0x16,
                               0x17, 0x0B, 0x0A, 0x06, 0x08, 0x0A, 0x0B, 0x05, 0x06, 0x15, 0x04, 0x06, 0x17, 0x05, 0x0A,
                               0x08, 0x05, 0x06, 0x15, 0x06, 0x0A, 0x25, 0x06, 0x08, 0x07, 0x18, 0x0A, 0x07, 0x0A, 0x08,
                               0x0B, 0x07, 0x0B, 0x04, 0x25, 0x04, 0x25, 0x04, 0x0A, 0x06, 0x04, 0x05, 0x14, 0x05, 0x09,
                               0x34, 0x07, 0x06, 0x17, 0x09, 0x1A, 0x2B, 0xFC, 0xFC, 0xFC, 0xFB, 0xFB, 0xFB, 0x0C, 0x0B,
                               0x2C, 0x0B, 0x2C, 0x0B, 0x3C, 0x0B, 0x2C, 0x2B, 0xAC};

    struct SFTEntry
    {
      uint_t Bits;
      uint_t Value;
      uint_t Code;

      bool operator<(const SFTEntry& rh) const
      {
        return Bits == rh.Bits ? Value < rh.Value : Bits < rh.Bits;
      }
    };

    constexpr uint_t InvertBits(uint_t val)
    {
      uint_t res = 0;
      for (uint_t count = 16, mask = val; count; --count, mask >>= 1)
      {
        res = 2 * res + (mask & 1);
      }
      return res;
    }

    void ExtractTree(const uint8_t* data, std::vector<SFTEntry>& result)
    {
      std::vector<SFTEntry> tree;
      for (uint_t count = *data++, idx = 0; count; --count)
      {
        const uint_t bits = (*data & 0x0f) + 1;
        for (uint_t groupLen = (*data++ >> 4) + 1; groupLen; --groupLen, ++idx)
        {
          const SFTEntry entry = {bits, idx, 0};
          tree.push_back(entry);
        }
      }
      std::sort(tree.begin(), tree.end());
      static_assert(InvertBits(1) == 0x8000, "Wrong InvertBits");
      static_assert(InvertBits(0x180) == 0x180, "Wrong InvertBits");
      static_assert(InvertBits(0x8000) == 1, "Wrong InvertBits");
      uint_t code = 0;
      uint_t codeIncrement = 0;
      uint_t lastBits = 0;
      for (auto it = tree.rbegin(), lim = tree.rend(); it != lim; ++it)
      {
        code += codeIncrement;
        it->Code = InvertBits(code);
        if (it->Bits != lastBits)
        {
          lastBits = it->Bits;
          codeIncrement = 1 << (16 - lastBits);
        }
      }
      result.swap(tree);
    }

    uint_t ReadByTree(Bitstream& stream, const std::vector<SFTEntry>& tree)
    {
      auto it = tree.begin();
      for (uint_t bits = 0, result = 0;;)
      {
        result |= stream.GetBit() << bits++;
        for (; it->Bits <= bits; ++it)
        {
          Require(it != tree.end());
          if (it->Bits == bits && it->Code == result)
          {
            return it->Value;
          }
        }
      }
      return 0;
    }

    Binary::Container::Ptr Decode(const Parser& parser)
    {
      const auto& header = parser.GetHeader();
      assert(IMPLODE == header.Method);
      const std::size_t dataSize = parser.GetSourceFileSize();
      const bool isBigFile = dataSize >= 0x1600;
      const bool isTextFile = 0 != (header.Flags & 1);
      const bool isBigTextFile = isBigFile && isTextFile;

      std::vector<SFTEntry> SFT1;
      std::vector<SFTEntry> SFT2;
      std::vector<SFTEntry> SFT3;

      if (isBigTextFile)
      {
        ExtractTree(SFT_100, SFT1);
        assert(SFT1.size() == 256);
      }
      if (isTextFile)
      {
        ExtractTree(isBigFile ? SFT_64_4 : SFT_64_6, SFT2);
        ExtractTree(isBigFile ? SFT_64_3 : SFT_64_5, SFT3);
      }
      else
      {
        ExtractTree(SFT_64_2, SFT2);
        ExtractTree(SFT_64_1, SFT3);
      }
      assert(SFT2.size() == 64);
      assert(SFT3.size() == 64);

      const uint_t distBits = isBigTextFile ? 7 : 6;
      const uint_t minMatchLen = isBigTextFile ? 3 : 2;

      Bitstream stream(parser.GetCompressedData());
      Binary::DataBuilder result(dataSize);
      while (result.Size() < dataSize)
      {
        if (stream.GetBit())
        {
          const uint_t data = isBigTextFile ? ReadByTree(stream, SFT1) : stream.GetBits(8);
          result.AddByte(static_cast<uint8_t>(data));
        }
        else
        {
          const uint_t loDist = stream.GetBits(distBits);
          const uint_t hiDist = ReadByTree(stream, SFT3);
          const std::size_t dist = (loDist | (hiDist << distBits)) + 1;

          std::size_t len = ReadByTree(stream, SFT2);
          if (63 == len)
          {
            len += stream.GetBits(8);
          }
          len += minMatchLen;
          if (dist > result.Size())
          {
            const std::size_t zeroes = std::min<std::size_t>(dist - result.Size(), len);
            Fill(result, zeroes, 0);
            len -= zeroes;
          }
          if (len)
          {
            CopyFromBack(dist, result, len);
          }
        }
      }
      return result.CaptureResult();
    }
  }  // namespace Implode

  namespace Shrink
  {
    struct LZWEntry
    {
      static const uint_t LIMITER = 256;

      uint_t Parent = 0;
      uint8_t Value = '\0';
      bool IsFree = false;

      LZWEntry() = default;

      explicit LZWEntry(uint_t value)
        : Parent(LIMITER)
        , Value(static_cast<uint8_t>(value < LIMITER ? value : 0))
        , IsFree(value >= LIMITER)
      {}
    };

    using LZWTree = std::array<LZWEntry, 8192>;

    void ResetTree(LZWTree& tree)
    {
      for (uint_t idx = 0; idx < tree.size(); ++idx)
      {
        tree[idx] = LZWEntry(idx);
      }
    }

    void PartialResetTree(LZWTree& tree)
    {
      const std::size_t parentsCount = tree.size() - LZWEntry::LIMITER;
      std::vector<bool> hasChilds(parentsCount);
      for (std::size_t idx = 0; idx < parentsCount; ++idx)
      {
        if (!tree[idx + LZWEntry::LIMITER].IsFree && tree[idx + LZWEntry::LIMITER].Parent > LZWEntry::LIMITER)
        {
          const std::size_t parent = tree[idx + LZWEntry::LIMITER].Parent - LZWEntry::LIMITER;
          hasChilds[parent] = true;
        }
      }
      for (std::size_t idx = 0; idx < parentsCount; ++idx)
      {
        if (!hasChilds[idx])
        {
          tree[idx + LZWEntry::LIMITER].IsFree = true;
          tree[idx + LZWEntry::LIMITER].Parent = LZWEntry::LIMITER;
        }
      }
    }

    Binary::Container::Ptr Decode(const Parser& parser)
    {
      assert(SHRINK == parser.GetHeader().Method);

      Bitstream stream(parser.GetCompressedData());
      const std::size_t sourceSize = parser.GetSourceFileSize();
      Binary::DataBuilder result(sourceSize);

      LZWTree tree;
      ResetTree(tree);

      auto* lastFree = tree.begin() + LZWEntry::LIMITER;

      uint_t codeSize = 9;
      uint_t oldCode = stream.GetBits(codeSize);
      result.AddByte(static_cast<uint8_t>(oldCode));
      while (result.Size() < sourceSize)
      {
        const uint_t code = stream.GetBits(codeSize);
        if (LZWEntry::LIMITER == code)
        {
          const uint_t subCode = stream.GetBits(codeSize);
          if (1 == subCode)
          {
            ++codeSize;
          }
          else if (2 == subCode)
          {
            PartialResetTree(tree);
            lastFree = tree.begin() + LZWEntry::LIMITER;
          }
        }
        else
        {
          const bool isFree = tree.at(code).IsFree;
          Binary::Dump substring(isFree ? 1 : 0);
          for (uint_t curCode = isFree ? oldCode : code; curCode != LZWEntry::LIMITER;)
          {
            const LZWEntry& curEntry = tree.at(curCode);
            Require(curCode != curEntry.Parent);
            Require(substring.size() <= tree.size());
            substring.push_back(curEntry.Value);
            curCode = curEntry.Parent;
          }
          Require(!substring.empty());
          if (isFree)
          {
            substring.front() = substring.back();
          }
          std::reverse(substring.begin(), substring.end());
          result.Add(substring);
          for (++lastFree; lastFree != tree.end() && !lastFree->IsFree; ++lastFree)
          {}
          Require(lastFree != tree.end());
          lastFree->Value = substring.front();
          lastFree->Parent = oldCode;
          lastFree->IsFree = false;
          oldCode = code;
        }
      }
      return result.CaptureResult();
    }
  }  // namespace Shrink

  namespace LzPress
  {
    // One prefix bit selects a 7-bit (prefix 1) or 12-bit (prefix 0) code.
    uint_t ReadToken(Bitstream& stream)
    {
      const bool prefix = stream.GetBit();
      return stream.GetBits(prefix ? 7 : 12);
    }

    // A decoded dictionary entry: its length and its leading symbol, the latter being
    // appended to the previous string when the next entry is inserted.
    struct Chain
    {
      uint8_t Head = 0;
      uint_t Size = 0;
    };

    // LZPRESS dictionary: a bounded adaptive LZW code table whose entries are nodes of a
    // ternary search tree. A code is a node index; once the free-node counter reaches the
    // table limit, nodes are recycled, so the same code may map to different strings over
    // time. Two intrusive lists (A and B) drive that reuse. Original entry point labels are
    // kept in comments to simplify cross-checking with the reference disassembly.
    class Dictionary
    {
      static const uint_t NODES = 5040;

      // A node's ternary-search-tree children.
      enum Link : uint_t
      {
        Lesser = 0,
        Mid = 1,
        Greater = 2
      };

      // A dictionary trie node. The original packs six 12-bit link fields into nibbles
      // of a 13-byte record; they are logically independent, so they are kept apart.
      //
      // Ch is the stored symbol and Children the ternary-search-tree children, selected by
      // comparing Ch with the symbol being inserted; Parent is followed by Resolve when
      // emitting a string. Prev/Next thread the node onto one of the two reuse lists.
      struct Node
      {
        uint8_t Ch = 0;
        uint_t Parent = 0;
        uint_t Children[3] = {0, 0, 0};
        uint_t Prev = 0;
        uint_t Next = 0;
        // Usage counter compared by Touch to pick an eviction victim.
        uint_t Count = 0;
        // Length of the string this node stands for (one character per Mid link). Insert
        // refuses to grow it past 0x3f. The original kept it in the low 7 bits of a byte.
        uint_t ChainSize = 0;
        // Set while this multi-symbol entry has no Mid child, i.e. it is a reusable leaf
        // and a member of the second reuse list. The original kept it in bit 0x80.
        bool ReusableLeaf = false;

        uint_t& Child(Link type)
        {
          return Children[type];
        }

        const uint_t& Child(Link type) const
        {
          return Children[type];
        }
      };

    public:
      // Was call7205: build the 256-symbol base alphabet. Node indices are the LZW codes, so
      // this insertion order (a permutation of all byte values) is part of the format.
      Dictionary()
      {
        uint8_t symbol = 0xd9;
        for (uint_t i = 0; i != 256; ++i)
        {
          symbol = uint8_t(symbol + 0x47);
          Insert(0, symbol);
        }
      }

      // Was call7529: insert symbol under the node hl, returning its code, 0 on failure.
      uint_t Insert(uint_t hl, uint8_t symbol)
      {
        if (Nodes[hl].ChainSize >= 0x3f)
        {
          return 0;
        }
        uint_t parent = hl;
        Link slot = Mid;
        for (;;)
        {
          const uint_t child = Nodes[parent].Child(slot);
          if (child == 0)
          {
            break;
          }
          const uint8_t ch = Nodes[child].Ch;
          if (ch == symbol)
          {
            return child;
          }
          parent = child;
          slot = (symbol > ch) ? Greater : Lesser;
        }
        uint_t newNode = NextFree;
        if ((newNode + 2) & 0x1000)
        {
          newNode = Recycle(hl);
        }
        else
        {
          newNode = ++NextFree;
        }
        if (newNode == 0)
        {
          return 0;
        }
        CreateNode(newNode, symbol, parent, slot);
        Nodes[newNode].Count = 0;
        if (newNode < 0x80)
        {
          ListPushFront(HeadA, TailA, newNode);
        }
        else if (Nodes[newNode].ReusableLeaf)
        {
          ListPushFront(HeadB, TailB, newNode);
        }
        return newNode;
      }

      // Was call75aa: record a use of hl and apply the eviction policy, returning the code the
      // entry can still be reached by. A rotation exchanges the touched entry with the coldest
      // low code, so when the result differs from hl those two indices were swapped.
      uint_t Touch(uint_t hl)
      {
        if (hl == 0)
        {
          return 0;
        }
        const uint_t count = Nodes[hl].Count + 1;
        Nodes[hl].Count = count;
        if (hl < 0x80)
        {
          ListUnlink(HeadA, TailA, hl);
          ListPushFront(HeadA, TailA, hl);
          return hl;
        }
        const bool less = (Nodes[TailA].Count < count);
        if (!less)
        {
          if (!Nodes[hl].ReusableLeaf)
          {
            return hl;
          }
          ListUnlink(HeadB, TailB, hl);
          ListPushFront(HeadB, TailB, hl);
          return hl;
        }
        if (Nodes[hl].ReusableLeaf)
        {
          ListUnlink(HeadB, TailB, hl);
        }
        const uint_t oldest = TailA;
        ListUnlink(HeadA, TailA, oldest);
        Rotate(oldest, hl);
        ListPushFront(HeadA, TailA, oldest);
        if (Nodes[hl].ReusableLeaf)
        {
          ListPushFront(HeadB, TailB, hl);
        }
        return oldest;
      }

      // Was call77f5: append the string for hl to out and return the entry it stands for.
      Chain Extract(uint_t hl, Binary::DataBuilder& out) const
      {
        const uint_t len = Nodes[hl].ChainSize;
        uint8_t* const dst = static_cast<uint8_t*>(out.Allocate(len));
        uint_t idx = len;
        for (uint_t cur = hl; cur != 0; cur = Resolve(cur))
        {
          dst[--idx] = Nodes[cur].Ch;
        }
        return {dst[0], len};
      }

      // Highest assigned code; a token above it is invalid.
      uint_t GetNextFree() const
      {
        return NextFree;
      }

    private:
      // Was call7248: make nodes[bc].Parent point to nodes[hl].Parent and replace the
      // parent's child link to hl with bc.
      void ReplaceChildLink(uint_t hl, uint_t bc)
      {
        if (bc != 0)
        {
          Nodes[bc].Parent = Nodes[hl].Parent;
        }
        Node& parent = Nodes[Nodes[hl].Parent];
        for (uint_t& child : parent.Children)
        {
          if (child == hl)
          {
            child = bc;
            break;
          }
        }
      }

      // Was call7290: descend the Greater chain, then fix the last node's child link.
      uint_t DescendGreater(uint_t hl)
      {
        while (Nodes[hl].Child(Greater) != 0)
        {
          hl = Nodes[hl].Child(Greater);
        }
        // The original decrements a zero selector, leaving 0xff (Lesser), not Mid.
        ReplaceChildLink(hl, Nodes[hl].Child(Lesser));
        return hl;
      }

      // Was call72ae: walk up while the parent's Mid child is not us.
      uint_t Resolve(uint_t hl) const
      {
        for (;;)
        {
          const uint_t parent = Nodes[hl].Parent;
          if (Nodes[parent].Child(Mid) == hl)
          {
            return parent;
          }
          hl = parent;
        }
      }

      // Was call72d8: detach a node from its parent, putting a splice of its subtrees in its
      // place. With a single child that child takes over; with two, the Greater subtree is
      // rehung under the tail of the Lesser subtree.
      void FixNode(uint_t node)
      {
        uint_t replacement = 0;
        if (Nodes[node].Child(Lesser) == 0)
        {
          replacement = Nodes[node].Child(Greater);
        }
        else if (Nodes[node].Child(Greater) == 0)
        {
          replacement = Nodes[node].Child(Lesser);
        }
        else
        {
          const uint_t tail = DescendGreater(Nodes[node].Child(Lesser));
          // DescendGreater can rewire the Lesser child (when the Lesser subtree is a single chain),
          // so the surviving children are re-read here, as the original does.
          const uint_t lesser = Nodes[node].Child(Lesser);
          const uint_t greater = Nodes[node].Child(Greater);
          Nodes[tail].Child(Lesser) = lesser;
          if (lesser != 0)
          {
            Nodes[lesser].Parent = tail;
          }
          Nodes[tail].Child(Greater) = greater;
          if (greater != 0)
          {
            Nodes[greater].Parent = tail;
          }
          replacement = tail;
        }
        ReplaceChildLink(node, replacement);
      }

      // Was call7404
      void CreateNode(uint_t node, uint8_t symbol, uint_t parent, Link slot)
      {
        Node& n = Nodes[node];
        n = Node{};
        n.Ch = symbol;
        n.Parent = parent;
        Nodes[parent].Child(slot) = node;
        // A Mid link appends a character to the parent's string; Lesser/Greater stay at its
        // depth.
        const bool asMid = (slot == Mid);
        const uint_t chainSize = Nodes[parent].ChainSize + (asMid ? 1 : 0);
        n.ChainSize = chainSize;
        n.ReusableLeaf = (chainSize != 1);
        if (Nodes[parent].ReusableLeaf && asMid)
        {
          Nodes[parent].ReusableLeaf = false;
          if (parent >= 0x80)
          {
            ListUnlink(HeadB, TailB, parent);
          }
        }
      }

      // Was call7479: move hl to the front of a doubly linked list.
      void ListPushFront(uint_t& head, uint_t& tail, uint_t hl)
      {
        if (head == 0)
        {
          tail = hl;
        }
        else
        {
          Nodes[head].Prev = hl;
        }
        Nodes[hl].Next = head;
        Nodes[hl].Prev = 0;
        head = hl;
      }

      // Was call74b8: remove hl from a doubly linked list.
      void ListUnlink(uint_t& head, uint_t& tail, uint_t hl)
      {
        const Node& n = Nodes[hl];
        if (head == hl)
        {
          head = n.Next;
        }
        if (tail == hl)
        {
          tail = n.Prev;
        }
        if (n.Next != 0)
        {
          Nodes[n.Next].Prev = n.Prev;
        }
        if (n.Prev != 0)
        {
          Nodes[n.Prev].Next = n.Next;
        }
      }

      // Was call7659: exchange the entries of hl and bc, fixing every reference to either. The
      // reference does it through its single scratch buffer; the net effect is the swap of the
      // table entries plus the snapshot/fixup steps of call769d, call76d2 and call7732.
      uint_t Rotate(uint_t hl, uint_t bc)
      {
        // Was call769d: copy both entries and the parent link that refers to each.
        const Node n0 = Nodes[hl];
        const Node n1 = Nodes[bc];
        const Link s0 = Nodes[n0.Parent].Child(Lesser) == hl ? Lesser
                        : Nodes[n0.Parent].Child(Mid) == hl  ? Mid
                                                             : Greater;
        const Link s1 = Nodes[n1.Parent].Child(Lesser) == bc ? Lesser
                        : Nodes[n1.Parent].Child(Mid) == bc  ? Mid
                                                             : Greater;
        // Was call76d2: within each entry, replace references to the other with its own index.
        if (Nodes[bc].Parent == hl)
        {
          Nodes[bc].Parent = bc;
        }
        for (uint_t& child : Nodes[bc].Children)
        {
          if (child == hl)
          {
            child = bc;
          }
        }
        if (Nodes[hl].Parent == bc)
        {
          Nodes[hl].Parent = hl;
        }
        for (uint_t& child : Nodes[hl].Children)
        {
          if (child == bc)
          {
            child = hl;
          }
        }
        std::swap(Nodes[hl], Nodes[bc]);
        // Was call7732: repoint each copy's parent link and children to its new index.
        if (n1.Parent != hl)
        {
          Nodes[n1.Parent].Child(s1) = hl;
        }
        for (const uint_t child : n1.Children)
        {
          if (child != 0 && child != hl)
          {
            Nodes[child].Parent = hl;
          }
        }
        if (n0.Parent != bc)
        {
          Nodes[n0.Parent].Child(s0) = bc;
        }
        for (const uint_t child : n0.Children)
        {
          if (child != 0 && child != bc)
          {
            Nodes[child].Parent = bc;
          }
        }
        return hl;
      }

      // Was call7363: walk the B list for a node that is neither the insertion root nor in
      // its Mid subtree, detach it (FixNode) and return it for reuse. 0 means none was found.
      uint_t Recycle(uint_t root)
      {
        for (uint_t node = TailB; node != 0; node = Nodes[node].Prev)
        {
          if (node == root || Resolve(node) == root)
          {
            continue;
          }
          FixNode(node);
          const uint_t parent = Nodes[node].Parent;
          if (Nodes[parent].Child(Mid) == 0 && !Nodes[parent].ReusableLeaf && Nodes[parent].ChainSize != 1)
          {
            Nodes[parent].ReusableLeaf = true;
            if (parent >= 0x80)
            {
              ListPushFront(HeadB, TailB, parent);
            }
          }
          ListUnlink(HeadB, TailB, node);
          return node;
        }
        return 0;
      }

      // Node-table allocation cursor. Once it passes the 12-bit range, Insert stops
      // appending and takes free nodes from the reuse lists instead.
      uint_t NextFree = 0;
      // Two intrusive reuse lists threaded through Node::Prev/Next. List A is ordered by
      // use, so TailA is the next eviction candidate; list B holds the reusable-leaf nodes
      // that have to be unlinked when they are recycled. Only indices >= 0x80 (codes reached
      // through the 12-bit token form) ever join B; the low 7-bit codes stay on A alone.
      uint_t HeadA = 0;
      uint_t TailA = 0;
      uint_t HeadB = 0;
      uint_t TailB = 0;

      std::array<Node, NODES> Nodes{};
    };

    // Was unlz: LZPRESS (ZXZip method 1) token stream decoded over a Dictionary.
    Binary::Container::Ptr Decode(const Parser& parser)
    {
      assert(LZPRESS == parser.GetHeader().Method);
      const std::size_t dataSize = parser.GetSourceFileSize();
      Binary::DataBuilder result(dataSize);
      Bitstream input(parser.GetCompressedData());
      Dictionary dict;

      // The entry emitted last, kept to extend it in the KwKwK case; prevCode is its code
      // (0 before the first entry).
      Chain chain;
      uint_t prevCode = 0;
      while (result.Size() < dataSize)
      {
        const uint_t code = ReadToken(input);
        if (code > dict.GetNextFree())
        {
          break;
        }
        if (code != 0)
        {
          chain = dict.Extract(code, result);
          const uint_t touched = dict.Touch(code);
          if (prevCode != 0)
          {
            // A rotation may have exchanged the touched entry with the previous one.
            const uint_t parent = prevCode == code ? touched : prevCode == touched ? code : prevCode;
            dict.Insert(parent, chain.Head);
          }
          prevCode = touched;
        }
        else
        {
          // Was call782c/KwKwK case: the next entry is the previous one plus its own first
          // byte, so it is a copy from just behind the output cursor.
          if (chain.Size == 0 || chain.Size >= 0x80)
          {
            break;
          }
          CopyFromBack(chain.Size, result, chain.Size + 1);
          ++chain.Size;
          prevCode = dict.Touch(dict.Insert(prevCode, chain.Head));
        }
      }
      // The reference stops exactly at the requested size; the last entry may overshoot.
      if (result.Size() > dataSize)
      {
        result.Resize(dataSize);
      }
      return result.CaptureResult();
    }

  }  // namespace LzPress

  Binary::Container::Ptr DecodeImpl(const Parser& parser)
  {
    switch (parser.GetHeader().Method)
    {
    case STORE:
      return Store::Decode(parser);
    case IMPLODE:
      return Implode::Decode(parser);
    case SHRINK:
      return Shrink::Decode(parser);
    case LZPRESS:
      return LzPress::Decode(parser);
    default:
      return {};
    }
  }

  class Decoder : public Packed::Decoder
  {
  public:
    StringView GetDescription() const override
    {
      return DESCRIPTION;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Format;
    }

    Container::Ptr Decode(const Binary::Container& rawData) const override
    {
      if (!Format->Match(rawData))
      {
        return {};
      }
      const Parser parser(rawData);
      if (!parser.FastCheck())
      {
        return {};
      }
      try
      {
        if (const auto decoded = DecodeImpl(parser))
        {
          Require(parser.GetSourceFileSize() == decoded->Size());
          Require(~parser.GetHeader().SourceCRC == Binary::Crc32(*decoded));
          return CreateContainer(std::move(decoded), parser.GetUsedSize());
        }
      }
      catch (const std::exception&)
      {}
      return {};
    }

  private:
    const Binary::Format::Ptr Format = Binary::CreateFormat(HEADER_PATTERN, MIN_SIZE);
  };
}  // namespace Formats::Packed::ZXZip

namespace Formats::Packed
{
  Decoder::Ptr CreateZXZipDecoder()
  {
    return MakePtr<ZXZip::Decoder>();
  }
}  // namespace Formats::Packed
