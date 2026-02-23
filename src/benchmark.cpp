#include <thread>
#include <future>
#include <iostream>
#include <chrono>

#include "benchmark.hpp"
#include "server.hpp"
#include "client.hpp"
#include "numformat.hpp"

constexpr unsigned int DEFAULT_DURATION = 20;

std::vector<BenchmarkConfig> benchmarkConfigs = {
    {DEFAULT_DURATION, 1},
    {DEFAULT_DURATION, 3},
    {DEFAULT_DURATION, 5},
    {DEFAULT_DURATION, 10},
    {DEFAULT_DURATION, 25}};

bool benchmarkDone = false;

struct BenchmarkClientResults
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

BenchmarkClientResults clientProcess()
{
  BenchmarkClientResults results;

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

void logBenchmark(BenchmarkConfig config, unsigned int successes, unsigned int failures)
{
  unsigned int throughput = successes / config.duration;

  std::cout << "\nBenchmark: " << config.clients << " clients, " << config.duration << "s duration\n";
  std::cout << "Throughput: " << throughput << " reqs/sec\n";
  std::cout << "Success Rate: " << (100 * successes / (successes + failures)) << "%\n";
}

void runBenchmark(BenchmarkConfig benchmark)
{
  benchmarkDone = false;

  std::future<BenchmarkClientResults> clients[benchmark.clients];

  for (int i = 0; i < benchmark.clients; i++)
    clients[i] = std::async(&clientProcess);

  std::this_thread::sleep_for(std::chrono::seconds(benchmark.duration));

  benchmarkDone = true;

  unsigned int totalSuccesses = 0, totalFailures = 0;
  for (int i = 0; i < benchmark.clients; i++)
  {
    BenchmarkClientResults results = clients[i].get();
    totalSuccesses += results.successes;
    totalFailures += results.failures;
  }

  logBenchmark(benchmark, totalSuccesses, totalFailures);
}

void runBenchmarks()
{
  // Set locale for comma formatting
  std::locale locale(std::locale(), new NumberFormat());
  std::cout.imbue(locale);

  std::thread server(createServer);
  std::cout << "Started server\n";

  for (auto benchmark : benchmarkConfigs)
    runBenchmark(benchmark);

  std::terminate(); // Kill all threads, including server thread
}