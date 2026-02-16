#include "server.hpp"

int main() {
  startServer(
    "127.0.0.1", 3000, "/public"
  );

  return 0;
}