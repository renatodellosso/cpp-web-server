#pragma once

#include <vector>

struct BenchmarkConfig
{
  unsigned int duration;
  unsigned int clients;
};

void runBenchmarks();