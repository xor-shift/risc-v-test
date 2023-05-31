#include <array>
#include <cstddef>
#include <cstdint>

extern int64_t test_outputs[10];

int64_t test_outputs[10]{
  0x0F0E'0E0B'0D0A'0E0Dl,
  0,  // cyc_cnt 0
  0,  // fib_iter_10
  0,  // cyc_cnt 1
  0,  // fib_recursive_10
  0,  // cyc_cnt 2
  0,  // fac_iterative_10
  0,  // cyc_cnt 3
  0,  // __cxa_atexit test
  0x0E0B'0A0B'0E0F'0A0Cu,
};

static struct static_global_test {
    static_global_test() {
        test_outputs[8] |= 0x10;
    }

    ~static_global_test() {
        test_outputs[8] |= 0x0E0D000C0D0A0E0Dll;
    }
} static_global {};

auto fibo_iterative(int64_t n) -> int64_t {
    static int fibo[2] = {1, 1};

    for (int i = 2; i < n; i++) {
        int t = fibo[0] + fibo[1];
        fibo[0] = fibo[1];
        fibo[1] = t;
    }

    return fibo[1];
}

auto fibo_recursive(int64_t n) -> int64_t {
    if (n <= 2) {
        return 1;
    }

    return fibo_recursive(n - 1) + fibo_recursive(n - 2);
}

auto factorial_iterative(int64_t n) -> int64_t {
    int64_t ret = 1;

    while (n != 1) {
        ret *= n;
        n -= 1;
    }

    return ret;
}

template<typename T>
void assert_eq(T val, T expected) {
    if (val != expected) {
        asm volatile("");  // TODO
    }
}

int64_t read_cyc() {
    /*for (;;) {
        int64_t upper, lower, upper_again;

        asm volatile(
          "rdcycleh %[upper]\n"
          "rdcycle %[lower]\n"
          "rdcycleh %[upper_again]\n"
          : [upper] "=r"(upper),  //
            [lower] "=r"(lower),  //
            [upper_again] "=r"(upper_again)
          :
          :
        );

        if (upper == upper_again) {
            return upper << 32l | upper_again;
        }
    }*/

    // int64_t cyc_cnt;
    // asm volatile("rdcycle %[cyc_cnt]" : [cyc_cnt] "=r"(cyc_cnt));
    // return cyc_cnt;
    return 0x0E'0F'0A'0C'0D'0A'0B'0All;
}

extern "C" int main() {
    test_outputs[1] = read_cyc();
    assert_eq<int64_t>(test_outputs[2] = fibo_iterative(10l), 55l);
    test_outputs[3] = read_cyc();
    assert_eq<int64_t>(test_outputs[4] = fibo_recursive(10l), 55l);
    test_outputs[5] = read_cyc();
    assert_eq<int64_t>(test_outputs[6] = factorial_iterative(7l), 5040l);
    test_outputs[7] = read_cyc();

    test_outputs[8] |= 0x20;

    return 0;
}
