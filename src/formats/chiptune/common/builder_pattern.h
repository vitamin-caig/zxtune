/**
 *
 * @file
 *
 * @brief  Pattern builder interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "types.h"

namespace Formats::Chiptune
{
  class ChannelBuilder
  {
  public:
    virtual ~ChannelBuilder() = default;

    virtual void SetRest() = 0;
    virtual void SetNote(uint_t note) = 0;
    virtual void SetSample(uint_t sample) = 0;
    virtual void SetOrnament(uint_t ornament) = 0;
    virtual void SetVolume(uint_t volume) = 0;
    virtual void SetTempo(uint_t tempo) = 0;
  };

  template<class ChannelType>
  struct Tracking
  {
    using ChannelBuilder = ChannelType;

    class LineBuilder
    {
    public:
      virtual ~LineBuilder() = default;

      virtual ChannelBuilder& StartChannel(uint_t index) = 0;
      virtual void SetTempo(uint_t tempo) = 0;
    };

    class PatternBuilder
    {
    public:
      virtual ~PatternBuilder() = default;

      virtual LineBuilder& StartLine(uint_t index) = 0;
      virtual void Finish(uint_t size) = 0;
    };
  };

  template<class ChannelInterface>
  class PatternBuilderProxy
    : public Tracking<ChannelInterface>::PatternBuilder
    , public Tracking<ChannelInterface>::LineBuilder
    , public ChannelInterface
  {
  public:
    typename Tracking<ChannelInterface>::LineBuilder& StartLine(uint_t index) override
    {
      CurLine = &CurPattern->StartLine(index);
      CurChannel = nullptr;
      return *this;
    }

    void Finish(uint_t size) override
    {
      CurPattern->Finish(size);
      CurPattern = nullptr;
      CurLine = nullptr;
      CurChannel = nullptr;
    }

    ChannelInterface& StartChannel(uint_t index) override
    {
      CurChannel = &CurLine->StartChannel(index);
      return *this;
    }

    void SetTempo(uint_t tempo) override
    {
      if (CurChannel)
      {
        CurChannel->SetTempo(tempo);
      }
      else
      {
        CurLine->SetTempo(tempo);
      }
    }

  protected:
    void BindPattern(typename Tracking<ChannelInterface>::PatternBuilder& pattern)
    {
      CurPattern = &pattern;
    }

    ChannelInterface& GetChannel() const
    {
      return *CurChannel;
    }

  private:
    typename Tracking<ChannelInterface>::PatternBuilder* CurPattern = nullptr;
    typename Tracking<ChannelInterface>::LineBuilder* CurLine = nullptr;
    ChannelInterface* CurChannel = nullptr;
  };

  template<class ChannelInterface>
  class StubChannelBuilderImpl : public ChannelInterface
  {
  public:
    using Interface = ChannelInterface;

    void SetRest() override {}

    void SetNote(uint_t /*note*/) override {}

    void SetSample(uint_t /*sample*/) override {}

    void SetOrnament(uint_t /*ornament*/) override {}

    void SetVolume(uint_t /*volume*/) override {}

    void SetTempo(uint_t /*tempo*/) override {}
  };

  template<class ChannelType>
  class StubLineBuilderImpl : public Tracking<typename ChannelType::Interface>::LineBuilder
  {
  public:
    ChannelType& StartChannel(uint_t /*index*/) override
    {
      return Channel;
    }

    void SetTempo(uint_t /*tempo*/) override {}

  private:
    ChannelType Channel;
  };

  template<class ChannelType>
  class StubPatternBuilderImpl : public Tracking<typename ChannelType::Interface>::PatternBuilder
  {
    using LineType = StubLineBuilderImpl<ChannelType>;

  public:
    LineType& StartLine(uint_t /*index*/) override
    {
      return Line;
    }

    void Finish(uint_t /*size*/) override {}

  private:
    LineType Line;
  };

  template<class ChannelType>
  typename Tracking<typename ChannelType::Interface>::PatternBuilder& GetStubPatternBuilder()
  {
    static StubPatternBuilderImpl<ChannelType> Stub;
    return Stub;
  }
}  // namespace Formats::Chiptune
