/*
Abstract:
  Public interface of benchmark libray.

  This non-production library and application is used to research, so it looks much complex than required.

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef BENCHMARK_H_DEFINED
#define BENCHMARK_H_DEFINED

//std includes
#include <string>

namespace Benchmark
{
  class PerformanceTest
  {
  public:
    virtual ~PerformanceTest() {}

    virtual std::string Category() const = 0;
    virtual std::string Name() const = 0;
    //! @return Performance index
    virtual double Execute() const = 0;
  };

  class TestsVisitor
  {
  public:
    virtual ~TestsVisitor() {}

    virtual void OnPerformanceTest(const PerformanceTest& test) = 0;
  };

  void ForAllTests(TestsVisitor& visitor);
}

#endif //BENCHMARK_H_DEFINED
