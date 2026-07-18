// Frameworkless JSON-shape smoke check for the benchmark harness.
//
// The smoke test only needs proof that Google Benchmark emitted *parseable*
// JSON, never that any field or timing value is correct -- so this
// deliberately isn't a JSON library: a string/escape-aware brace-balance scan
// is enough to catch "crashed mid-write" or "not JSON at all" without adding
// a JSON dependency to a harness whose whole point is having none.

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

bool StructurallyBalanced(const std::string& text) {
  int  depth      = 0;
  bool in_string  = false;
  bool escaped    = false;
  for (char c : text) {
    if (in_string) {
      if (escaped) {
        escaped = false;
      } else if (c == '\\') {
        escaped = true;
      } else if (c == '"') {
        in_string = false;
      }
      continue;
    }
    if (c == '"') {
      in_string = true;
    } else if (c == '{' || c == '[') {
      ++depth;
    } else if (c == '}' || c == ']') {
      --depth;
      if (depth < 0) return false;
    }
  }
  return !in_string && depth == 0;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: al_benchmark_json_check <path-to-json>\n";
    return 1;
  }

  std::ifstream file(argv[1], std::ios::binary);
  if (!file) {
    std::cerr << "al_benchmark_json_check: cannot open " << argv[1] << "\n";
    return 1;
  }
  std::ostringstream buffer;
  buffer << file.rdbuf();
  const std::string content = buffer.str();

  if (content.empty()) {
    std::cerr << "al_benchmark_json_check: " << argv[1] << " is empty\n";
    return 1;
  }
  if (!StructurallyBalanced(content)) {
    std::cerr << "al_benchmark_json_check: " << argv[1]
               << " is not structurally balanced JSON\n";
    return 1;
  }
  if (content.find("\"benchmarks\"") == std::string::npos) {
    std::cerr << "al_benchmark_json_check: " << argv[1]
               << " has no top-level \"benchmarks\" key\n";
    return 1;
  }

  return 0;
}
