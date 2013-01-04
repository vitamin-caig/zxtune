/*
Abstract:
  ZXTune benchmark application

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "core/benchmark.h"
#include <iostream>

namespace
{
  class ExecuteTestsVisitor : public Benchmark::TestsVisitor
  {
  public:
    virtual void OnPerformanceTest(const Benchmark::PerformanceTest& test)
    {
      const std::string cat = test.Category();
      if (cat != LastCategory)
      {
        std::cout << "Test for " << cat << std::endl;
        LastCategory = cat;
      }
      std::cout << " " << test.Name() << ": " << std::flush << 'x' << test.Execute() << std::endl;
    }
  private:
    std::string LastCategory;
  };
}

int main()
{
  ExecuteTestsVisitor visitor;
  Benchmark::ForAllTests(visitor);
}
