#pragma once

#include <stuff/core.hpp>

namespace rv {

enum class instruction_standard {
    RV32I,
    RV64I,
    RV128I,

    RV32C,
    RV64C,
    RV128C,

    RV32M,
    RV64M,

    RV32A,
    RV64A,

    RV32F,
    RV64F,

    RV32D,
    RV64D,

    RV32Q,
    RV64Q,

    Zicsr,
    Zifencei,
};

enum class opcode_format {
    reg_reg,
    immediate,
    store,
    branch,
    upper_immediate,
    jump,

    c_reg_reg,
    c_immediate,
    c_wide_immediate,
    c_stack_rela_store,

    /*c_reg_reg,
    c_immediate,
    c_stack_rela_store,
    c_wide_immediate,
    c_load,
    c_store,
    c_arithmetic,
    c_branch,
    c_jump,*/
};

enum class reg : u32 {
    // clang-format off
    x0 = 0, zero = x0,
    x1 = 1, ra = x1,
    x2 = 2, sp = x2,
    x3 = 3, gp = x3,
    x4 = 4, tp = x4,
    x5 = 5, t0 = x5,
    x6 = 6, t1 = x6,
    x7 = 7, t2 = x7,
    x8 = 8, s0 = x8,
    x9 = 9, s1 = x9,
    x10 = 10, a0 = x10,
    x11 = 11, a1 = x11,
    x12 = 12, a2 = x12,
    x13 = 13, a3 = x13,
    x14 = 14, a4 = x14,
    x15 = 15, a5 = x15,
    x16 = 16, a6 = x16,
    x17 = 17, a7 = x17,
    x18 = 18, s2 = x18,
    x19 = 19, s3 = x19,
    x20 = 20, s4 = x20,
    x21 = 21, s5 = x21,
    x22 = 22, s6 = x22,
    x23 = 23, s7 = x23,
    x24 = 24, s8 = x24,
    x25 = 25, s9 = x25,
    x26 = 26, s10 = x26,
    x27 = 27, s11 = x27,
    x28 = 28, t3 = x28,
    x29 = 29, t4 = x29,
    x30 = 30, t5 = x30,
    x31 = 31, t6 = x31,
    // clang-format on
};

template<bool ABI = false>
constexpr auto register_name(reg reg) -> std::string_view {
    constexpr std::string_view names[][2]{
      {"x0", "Zero"}, {"x1", "ra"},  {"x2", "sp"},  {"x3", "gp"},  {"x4", "tp"},   {"x5", "t0"},   {"x6", "t1"},  {"x7", "t2"},  {"x8", "s0"},  {"x9", "s1"},  {"x10", "a0"},
      {"x11", "a1"},  {"x12", "a2"}, {"x13", "a3"}, {"x14", "a4"}, {"x15", "a5"},  {"x16", "a6"},  {"x17", "a7"}, {"x18", "s2"}, {"x19", "s3"}, {"x20", "s4"}, {"x21", "s5"},
      {"x22", "s6"},  {"x23", "s7"}, {"x24", "s8"}, {"x25", "s9"}, {"x26", "s10"}, {"x27", "s11"}, {"x28", "t3"}, {"x29", "t4"}, {"x30", "t5"}, {"x31", "t6"},
    };

    return names[static_cast<usize>(reg)][ABI];
}

enum class float_reg : u32 {
    // clang-format off
    f0 = 0, ft0 = f0,
    f1 = 1, ft1 = f1,
    f2 = 2, ft2 = f2,
    f3 = 3, ft3 = f3,
    f4 = 4, ft4 = f4,
    f5 = 5, ft5 = f5,
    f6 = 6, ft6 = f6,
    f7 = 7, ft7 = f7,
    f8 = 8, fs0 = f8,
    f9 = 9, fs1 = f9,
    f10 = 10, fa0 = f10,
    f11 = 11, fa1 = f11,
    f12 = 12, fa2 = f12,
    f13 = 13, fa3 = f13,
    f14 = 14, fa4 = f14,
    f15 = 15, fa5 = f15,
    f16 = 16, fa6 = f16,
    f17 = 17, fa7 = f17,
    f18 = 18, fs2 = f18,
    f19 = 19, fs3 = f19,
    f20 = 20, fs4 = f20,
    f21 = 21, fs5 = f21,
    f22 = 22, fs6 = f22,
    f23 = 23, fs7 = f23,
    f24 = 24, fs8 = f24,
    f25 = 25, fs9 = f25,
    f26 = 26, fs10 = f26,
    f27 = 27, fs11 = f27,
    f28 = 28, ft8 = f28,
    f29 = 29, ft9 = f29,
    f30 = 30, ft10 = f30,
    f31 = 31, ft11 = f31,
    // clang-format on
};

template<bool ABI = false>
constexpr auto enum_name(float_reg reg) -> std::string_view {
    constexpr std::string_view names[][2]{
      {"f0", "ft0"},  {"f1", "ft1"},  {"f2", "ft2"},   {"f3", "ft3"},   {"f4", "ft4"},  {"f5", "ft5"},  {"f6", "ft6"},   {"f7", "ft7"},
      {"f8", "fs0"},  {"f9", "fs1"},  {"f10", "fa0"},  {"f11", "fa1"},  {"f12", "fa2"}, {"f13", "fa3"}, {"f14", "fa4"},  {"f15", "fa5"},
      {"f16", "fa6"}, {"f17", "fa7"}, {"f18", "fs2"},  {"f19", "fs3"},  {"f20", "fs4"}, {"f21", "fs5"}, {"f22", "fs6"},  {"f23", "fs7"},
      {"f24", "fs8"}, {"f25", "fs9"}, {"f26", "fs10"}, {"f27", "fs11"}, {"f28", "ft8"}, {"f29", "ft9"}, {"f30", "ft10"}, {"f31", "ft11"},
    };

    return names[static_cast<usize>(reg)][ABI];
}

}  // namespace rv
