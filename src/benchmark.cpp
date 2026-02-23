#include <thread>
#include <future>
#include <iostream>
#include <chrono>

#include "benchmark.hpp"
#include "server.hpp"
#include "client.hpp"
#include "numformat.hpp"

bool benchmarkDone = false;

void createServer()
{
  startServer(
      "127.0.0.1", 3000, "./public");
}

void makeRequest(char *target)
{
  request("127.0.0.1", "3000", target);
}

unsigned int clientProcess()
{
  unsigned int count;

  char *targets[3] = {
      "/index.html",
      "/index.css",
      "/index.js"};

  while (!benchmarkDone)
  {
    makeRequest(targets[count % 3]);
    count++;
  }

  return count;
}

void runBenchmark()
{
  std::thread server(createServer);

  std::future<unsigned int> clients[BENCHMARK_CLIENT_THREADS];

  for (int i = 0; i < BENCHMARK_CLIENT_THREADS; i++)
    clients[i] = std::async(&clientProcess);

  std::cout << "Benchmark started. Waiting for " << BENCHMARK_DURATION << " seconds...\n";
  std::this_thread::sleep_for(std::chrono::seconds(BENCHMARK_DURATION));

  benchmarkDone = true;

  unsigned int totalResult = 0;
  for (int i = 0; i < BENCHMARK_CLIENT_THREADS; i++)
    totalResult += clients[i].get();
  unsigned int throughput = totalResult / BENCHMARK_DURATION;

  // Set locale for comma formatting
  std::locale locale(std::locale(), new NumberFormat());
  std::cout.imbue(locale);

  std::cout << "Benchmark Complete! Result: " << throughput << " reqs/sec\n";

  std::terminate(); // Kill all threads, including server thread
}