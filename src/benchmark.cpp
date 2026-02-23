#include <thread>
#include <future>
#include <iostream>
#include <chrono>

#include "benchmark.hpp"
#include "server.hpp"
#include "client.hpp"
#include "numformat.hpp"

bool benchmarkDone = false;

struct BenchmarkResults
{
  unsigned int successes;
  unsigned int failures;
};

void createServer()
{
  startServer(
      "127.0.0.1", 3000, "./public");
}

bool makeRequest(char *target)
{
  return request("127.0.0.1", "3000", target);
}

BenchmarkResults clientProcess()
{
  BenchmarkResults results;

  char *targets[3] = {
      "/index.html",
      "/index.css",
      "/index.js"};

  for (unsigned int i = 0; !benchmarkDone; i++)
  {
    if (makeRequest(targets[i % 3]))
      results.successes++;
    else
      results.failures++;
  }

  return results;
}

void runBenchmark()
{
  std::thread server(createServer);

  std::future<BenchmarkResults> clients[BENCHMARK_CLIENT_THREADS];

  for (int i = 0; i < BENCHMARK_CLIENT_THREADS; i++)
    clients[i] = std::async(&clientProcess);

  std::cout << "Benchmark started with " << BENCHMARK_CLIENT_THREADS << " clients. Waiting for " << BENCHMARK_DURATION << " seconds...\n";
  std::this_thread::sleep_for(std::chrono::seconds(BENCHMARK_DURATION));

  benchmarkDone = true;

  unsigned int totalSuccesses = 0, totalFailures = 0;
  for (int i = 0; i < BENCHMARK_CLIENT_THREADS; i++)
  {
    BenchmarkResults results = clients[i].get();
    totalSuccesses += results.successes;
    totalFailures += results.failures;
  }
  unsigned int throughput = totalSuccesses / BENCHMARK_DURATION;

  // Set locale for comma formatting
  std::locale locale(std::locale(), new NumberFormat());
  std::cout.imbue(locale);

  std::cout << "Benchmark Complete! Result: " << throughput << " reqs/sec\n";
  std::cout << "Success Rate: " << (100 * totalSuccesses / (totalSuccesses + totalFailures)) << "%\n";

  std::terminate(); // Kill all threads, including server thread
}