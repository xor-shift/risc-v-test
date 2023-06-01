#include <cstdint>
#include <cstdlib>

extern int64_t test_outputs[10];

int64_t test_outputs[10]{
  0x0F0E'0E0B'0D0A'0E0Dl,
  0,
  0x0E0B'0A0B'0E0F'0A0Cu,
};

struct foo {
    foo() { test_outputs[1] |= 1; }

    ~foo() { test_outputs[1] |= 2; }
};

auto test_static_construction() -> int64_t {
    static struct foo foo {};
    return test_outputs[1];
}

int main() {
    std::atexit([] { test_outputs[1] |= 4; });

    return test_static_construction();
}
