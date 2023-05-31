#include <array>
#include <cstddef>
#include <cstdint>

extern uint8_t game_of_life[16][8];
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

extern "C" int main() {
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
