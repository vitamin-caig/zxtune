/**
 *
 * @file
 *
 * @brief Scanner implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// common includes
#include <contract.h>
#include <iterator.h>
#include <make_ptr.h>
// library includes
#include <analysis/scanner.h>
#include <debug/log.h>
// std includes
#include <deque>
#include <list>

namespace Analysis
{
  const Debug::Stream Dbg("Analysis::Scanner");

  using namespace Formats;

  template<class DecoderPtrType>
  class DecodersQueue
  {
  public:
    void Add(std::size_t offset, DecoderPtrType decoder)
    {
      Storage.push_back(PositionAndDecoder(offset, decoder));
      Storage.sort();
    }

    bool IsEmpty() const
    {
      return Storage.empty();
    }

    std::size_t GetPosition() const
    {
      return Storage.front().Position;
    }

    DecoderPtrType FetchDecoder()
    {
      const DecoderPtrType result = Storage.front().Decoder;
      Storage.pop_front();
      return result;
    }

    void Reset()
    {
      StorageType other;
      for (const auto& entry : Storage)
      {
        other.push_back(PositionAndDecoder(0, entry.Decoder));
      }
      Storage.swap(other);
    }

  private:
    struct PositionAndDecoder
    {
      const std::size_t Position;
      const DecoderPtrType Decoder;

      PositionAndDecoder(std::size_t pos, DecoderPtrType decoder)
        : Position(pos)
        , Decoder(std::move(decoder))
      {}

      bool operator<(const PositionAndDecoder& rh) const
      {
        return Position < rh.Position;
      }
    };
    using StorageType = std::list<PositionAndDecoder>;
    StorageType Storage;
  };

  class ScanningContainer : public Binary::Container
  {
  public:
    explicit ScanningContainer(const Binary::Container& delegate)
      : Delegate(delegate)
      , Limit(Delegate.Size())
      , Content(static_cast<const uint8_t*>(Delegate.Start()))
      , Offset(0)
    {}

    std::size_t GetOffset() const
    {
      return Offset;
    }

    void SetOffset(std::size_t newOffset)
    {
      Require(newOffset >= Offset);
      Offset = newOffset;
    }

    void Advance(std::size_t delta)
    {
      Offset += delta;
    }

    bool IsEmpty() const
    {
      return Limit == Offset;
    }

    void SetEmpty()
    {
      Offset = Limit;
    }

    // Binary::Container
    const void* Start() const override
    {
      return Content + Offset;
    }

    std::size_t Size() const override
    {
      return Limit - Offset;
    }

    Binary::Container::Ptr GetSubcontainer(std::size_t offset, std::size_t size) const override
    {
      return Delegate.GetSubcontainer(Offset + offset, size);
    }

  private:
    const Binary::Container& Delegate;
    const std::size_t Limit;
    const uint8_t* const Content;
    std::size_t Offset;
  };

  template<class Traits>
  class TypedScanner
  {
  public:
    TypedScanner(const typename Traits::DecodersList& storage, std::size_t base, const Binary::Container& data)
      : Unprocessed(storage.begin(), storage.end())
      , Base(base)
      , Window(data)
      , Unrecognized(data)
    {}

    void Scan(Scanner::Target& target)
    {
      Scheduled.Reset();
      Window.SetOffset(0);
      Unrecognized.SetOffset(0);

      ScanFromStart(target);
      RescheduleUnprocessed();
      while (!IsFinished())
      {
        ScanFromLookaheads(target);
      }
      FlushUnrecognized(target);
    }

  private:
    bool IsFinished() const
    {
      return Window.IsEmpty();
    }

    void ScanFromStart(Scanner::Target& target)
    {
      while (!IsFinished() && Unprocessed)
      {
        const typename Traits::Decoder::Ptr decoder = *Unprocessed;
        ++Unprocessed;
        if (ProcessDecoder(decoder, target))
        {
          break;
        }
      }
    }

    void RescheduleUnprocessed()
    {
      while (Unprocessed)
      {
        const typename Traits::Decoder::Ptr decoder = *Unprocessed;
        Reschedule(decoder);
        ++Unprocessed;
      }
    }

    void ScanFromLookaheads(Scanner::Target& target)
    {
      while (!IsFinished())
      {
        if (Scheduled.IsEmpty())
        {
          Window.SetEmpty();
          break;
        }
        RescheduleLookaheads();
        Window.SetOffset(Scheduled.GetPosition());
        ProcessDecoder(Scheduled.FetchDecoder(), target);
      }
    }

    bool ProcessDecoder(typename Traits::Decoder::Ptr decoder, Scanner::Target& target)
    {
      if (const typename Traits::Container::Ptr result = decoder->Decode(Window))
      {
        const std::size_t used = Traits::GetUsedSize(*result);
        Dbg("Found {} at {}+{} in {} bytes", decoder->GetDescription(), Base, Window.GetOffset(), used);
        FlushUnrecognized(target);
        target.Apply(*decoder, Base + Window.GetOffset(), result);
        Schedule(decoder, used);
        Window.Advance(used);
        Unrecognized.SetOffset(Window.GetOffset());
        return true;
      }
      else
      {
        Schedule(decoder, decoder->GetFormat()->NextMatchOffset(Window));
        return false;
      }
    }

    void FlushUnrecognized(Scanner::Target& target) const
    {
      const std::size_t till = Window.GetOffset();
      const std::size_t unmatchedAt = Unrecognized.GetOffset();
      if (till > unmatchedAt)
      {
        Dbg("Unrecognized {}+({}..{})", Base, unmatchedAt, till);
        target.Apply(Base + unmatchedAt, Unrecognized.GetSubcontainer(0, till - unmatchedAt));
      }
    }

    void Schedule(typename Traits::Decoder::Ptr decoder, std::size_t delta)
    {
      const String id = decoder->GetDescription();
      if (delta != Window.Size())
      {
        const std::size_t nextPos = Window.GetOffset() + delta;
        Dbg("Skip {} for {} bytes (check at {}+{})", id, delta, Base, nextPos);
        Scheduled.Add(nextPos, decoder);
      }
      else
      {
        Dbg("Disable {} for future scans", id);
      }
    }

    void RescheduleLookaheads()
    {
      while (Scheduled.GetPosition() < Window.GetOffset())
      {
        Reschedule(Scheduled.FetchDecoder());
      }
    }

    void Reschedule(typename Traits::Decoder::Ptr decoder)
    {
      Dbg("Schedule to check {} at {}", decoder->GetDescription(), Window.GetOffset());
      Scheduled.Add(Window.GetOffset(), decoder);
    }

  private:
    RangeIterator<typename Traits::DecodersList::const_iterator> Unprocessed;
    DecodersQueue<typename Traits::Decoder::Ptr> Scheduled;
    const std::size_t Base;
    ScanningContainer Window;
    ScanningContainer Unrecognized;
  };

  struct ArchivedDataTraits
  {
    using Decoder = Archived::Decoder;
    using Container = Archived::Container;
    using DecodersList = std::deque<Decoder::Ptr>;

    static std::size_t GetUsedSize(const Container& container)
    {
      return container.Size();
    }
  };

  struct PackedDataTraits
  {
    using Decoder = Packed::Decoder;
    using Container = Packed::Container;
    using DecodersList = std::deque<Decoder::Ptr>;

    static std::size_t GetUsedSize(const Container& container)
    {
      return container.PackedSize();
    }
  };

  struct ImageDataTraits
  {
    using Decoder = Image::Decoder;
    using Container = Image::Container;
    using DecodersList = std::deque<Decoder::Ptr>;

    static std::size_t GetUsedSize(const Container& container)
    {
      return container.OriginalSize();
    }
  };

  struct ChiptuneDataTraits
  {
    using Decoder = Chiptune::Decoder;
    using Container = Chiptune::Container;
    using DecodersList = std::deque<Decoder::Ptr>;

    static std::size_t GetUsedSize(const Container& container)
    {
      return container.Size();
    }
  };

  struct DecodersSet
  {
    ArchivedDataTraits::DecodersList Archived;
    PackedDataTraits::DecodersList Packed;
    ImageDataTraits::DecodersList Image;
    ChiptuneDataTraits::DecodersList Chiptune;
  };

  template<class Traits>
  class DecodeUnrecognizedTarget : public Scanner::Target
  {
  public:
    DecodeUnrecognizedTarget(const typename Traits::DecodersList& decoders, Scanner::Target& recognized,
                             Scanner::Target& unrecognized)
      : Decoders(decoders)
      , Recognized(recognized)
      , Unrecognized(unrecognized)
    {}

    void Apply(const Archived::Decoder& decoder, std::size_t offset, Archived::Container::Ptr data) override
    {
      Recognized.Apply(decoder, offset, data);
    }

    void Apply(const Packed::Decoder& decoder, std::size_t offset, Packed::Container::Ptr data) override
    {
      Recognized.Apply(decoder, offset, data);
    }

    void Apply(const Image::Decoder& decoder, std::size_t offset, Image::Container::Ptr data) override
    {
      Recognized.Apply(decoder, offset, data);
    }

    void Apply(const Chiptune::Decoder& decoder, std::size_t offset, Chiptune::Container::Ptr data) override
    {
      Recognized.Apply(decoder, offset, data);
    }

    void Apply(std::size_t offset, Binary::Container::Ptr data) override
    {
      TypedScanner<Traits> scanner(Decoders, offset, *data);
      scanner.Scan(Unrecognized);
    }

    Scanner::Target& GetUnrecognizedTarget()
    {
      return Decoders.empty() ? Unrecognized : *this;
    }

  private:
    const typename Traits::DecodersList& Decoders;
    Scanner::Target& Recognized;
    Scanner::Target& Unrecognized;
  };

  class LinearScanner : public Scanner
  {
  public:
    void AddDecoder(Archived::Decoder::Ptr decoder) override
    {
      Decoders.Archived.push_back(decoder);
    }

    void AddDecoder(Packed::Decoder::Ptr decoder) override
    {
      Decoders.Packed.push_back(decoder);
    }

    void AddDecoder(Image::Decoder::Ptr decoder) override
    {
      Decoders.Image.push_back(decoder);
    }

    void AddDecoder(Chiptune::Decoder::Ptr decoder) override
    {
      Decoders.Chiptune.push_back(decoder);
    }

    void Scan(Binary::Container::Ptr data, Target& target) const override
    {
      // order: Archive -> Packed -> Image -> Chiptune -> unrecognized with possible skipping
      DecodeUnrecognizedTarget<ChiptuneDataTraits> chiptune(Decoders.Chiptune, target, target);
      DecodeUnrecognizedTarget<ImageDataTraits> image(Decoders.Image, target, chiptune.GetUnrecognizedTarget());
      DecodeUnrecognizedTarget<PackedDataTraits> packed(Decoders.Packed, target, image.GetUnrecognizedTarget());
      DecodeUnrecognizedTarget<ArchivedDataTraits> archived(Decoders.Archived, target, packed.GetUnrecognizedTarget());

      archived.Apply(0, data);
    }

  private:
    DecodersSet Decoders;
  };
}  // namespace Analysis

namespace Analysis
{
  Scanner::RWPtr CreateScanner()
  {
    return MakeRWPtr<LinearScanner>();
  }
}  // namespace Analysis
