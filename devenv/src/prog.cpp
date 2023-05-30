#include <array>
#include <cstddef>
#include <cstdint>

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
  0,  // asdasd
  0x0E0B'0A0B'0E0F'0A0Cu,
};

static struct foo {
    foo() {
        //test_outputs[8] = 0x0E0D000C0D0A0E0Dll;
    }

    ~foo() {
        test_outputs[8] = 0x0E0D000C0D0A0E0Dll;
    }
} awsdasdasdfasdfv {};

extern uint8_t game_of_life[16][8];

alignas(8) uint8_t game_of_life[16][8]{}; /*{
  0, 0, 0, 0, 0, 0, 0, 0,  //
  0, 0, 1, 0, 0, 0, 0, 0,  //
  0, 0, 0, 1, 0, 0, 0, 0,  //
  0, 1, 1, 1, 0, 0, 0, 0,  //
  0, 0, 0, 0, 0, 0, 0, 0,  //
  0, 0, 0, 0, 0, 0, 0, 0,  //
  0, 0, 0, 0, 0, 0, 0, 0,  //
  0, 0, 0, 0, 0, 0, 0, 0,  //
  0, 0, 0, 0, 0, 0, 0, 0,  //
  0, 0, 0, 0, 0, 0, 0, 0,  //
  0, 0, 0, 0, 0, 0, 0, 0,  //
  0, 0, 0, 0, 0, 0, 0, 0,  //
  0, 0, 0, 0, 0, 0, 0, 0,  //
  0, 0, 0, 0, 0, 0, 0, 0,  //
  0, 0, 0, 0, 0, 0, 0, 0,  //
  0, 0, 0, 0, 0, 0, 0, 0,  //
};*/

static size_t asdasd = 0;

uint8_t foo = 0xFFu;
uint8_t bar = 0x00u;

extern auto neighbours_inner(ptrdiff_t y, ptrdiff_t x, ptrdiff_t o_y, ptrdiff_t o_x) -> bool;
extern auto neighbours(ptrdiff_t y, ptrdiff_t x) -> size_t;
extern void step_gol();

void step_gol() {
    for (ptrdiff_t y = 0; y < std::size(game_of_life); y++) {
        for (ptrdiff_t x = 0; x < std::size(game_of_life[0]); x++) {
            const auto nbrs = neighbours(y, x);

            volatile auto& cell = game_of_life[y][x];
            auto alive = (cell & 1) == 1;

            if (alive) {
                if (nbrs > 3uz || nbrs <= 1uz) {
                    alive = false;
                }
            } else if (nbrs == 3uz) {
                alive = true;
            }

            cell = cell | (2 * alive);
        }
    }

    for (ptrdiff_t y = 0; y < std::size(game_of_life); y++) {
        for (ptrdiff_t x = 0; x < std::size(game_of_life[0]); x++) {
            auto& cell = game_of_life[y][x];
            cell >>= 1;
        }
    }
}

auto neighbours_inner(ptrdiff_t y, ptrdiff_t x, ptrdiff_t o_y, ptrdiff_t o_x) -> bool {
    if (o_y == o_x && o_x == 0) {
        return false;
    }

    ptrdiff_t c_y = y + o_y;
    ptrdiff_t c_x = x + o_x;

    if (c_y < 0 || c_x < 0 || c_x >= std::size(game_of_life[0]) || c_y >= std::size(game_of_life)) {
        return false;
    }

    if ((game_of_life[c_y][c_x] & 1) == 1) {
        return true;
    }

    return false;
}

auto neighbours(ptrdiff_t y, ptrdiff_t x) -> size_t {
    size_t ret = 0;
    for (ptrdiff_t o_y = -1; o_y <= 1; o_y++) {
        for (ptrdiff_t o_x = -1; o_x <= 1; o_x++) {
            if (neighbours_inner(y, x, o_y, o_x)) {
                ++ret;
            }
        }
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

    test_outputs[8] = 1;

    game_of_life[1][2] = 1;
    game_of_life[2][3] = 1;
    game_of_life[3][1] = 1;
    game_of_life[3][2] = 1;
    game_of_life[3][3] = 1;

    /*for (ptrdiff_t y = 0; y < std::size(game_of_life); y++) {
        for (ptrdiff_t x = 0; x < std::size(game_of_life[0]); x++) {
            auto& cell = game_of_life[y][x];
            cell = neighbours(y, x) << 1;
        }
    }*/

    for (int i = 0; i < 16; i++) {
        step_gol();
    }

    return 0;
}
