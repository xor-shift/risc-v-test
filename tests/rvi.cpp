#include <gtest/gtest.h>

#include "./common.hpp"

#include <rv/rv.hpp>

TEST(RV_DECODE, RV64I) {
    // clang-format off
    static constexpr std::pair<u32, std::string_view> test_cases[]{
      {0x00000037u, "lui x0, 0"},        {0xffffffb7u, "lui x31, -1"},       {0xffffefb7u, "lui x31, -2"},
      {0x00000f97u, "auipc x31, 0"},     {0xffffff97u, "auipc x31, -1"},     {0xffffef97u, "auipc x31, -2"},

      {0x0000006fu, "jal x0, 0"},        {0x0020006fu, "jal x0, 2"},         {0xfffff06fu, "jal x0, -2"},
      {0x0010006fu, "jal x0, 2048"},     {0x7fe0006fu, "jal x0, 2046"},      {0x801ff06fu, "jal x0, -2048"},
      {0x8000006fu, "jal x0, -1048576"}, {0x7ffff06fu, "jal x0, 1048574"},  //

      {0x00000067u, "jalr x0, x0, 0"},   {0x00208067u, "jalr x0, x1, 2"},    {0xffe08067u, "jalr x0, x1, -2"},

      {0x00000063u, "beq x0, x0, 0"},    {0x00100163u, "beq x0, x1, 2"},     {0xfe100fe3, "beq x0, x1, -2"},
      {0x001000e3u, "beq x0, x1, 2048"}, {0x80100063u, "beq x0, x1, -4096"},

      {0x00000003u, "lb x0, 0(x0)"},     {0x00208003u, "lb x0, 2(x1)"},      {0xffe08003u, "lb x0, -2(x1)"},
      {0x00001003u, "lh x0, 0(x0)"},     {0x00209003u, "lh x0, 2(x1)"},      {0xffe09003u, "lh x0, -2(x1)"},
      {0x00002003u, "lw x0, 0(x0)"},     {0x0020a003u, "lw x0, 2(x1)"},      {0xffe0a003u, "lw x0, -2(x1)"},
      {0x00004003u, "lbu x0, 0(x0)"},    {0x0020c003u, "lbu x0, 2(x1)"},     {0xffe0c003u, "lbu x0, -2(x1)"},
      {0x00005003u, "lhu x0, 0(x0)"},    {0x0020d003u, "lhu x0, 2(x1)"},     {0xffe0d003u, "lhu x0, -2(x1)"},

      {0x00000023u, "sb x0, 0(x0)"},     {0x00008123u, "sb x0, 2(x1)"},      {0xfe008f23u, "sb x0, -2(x1)"},
      {0x00001023u, "sh x0, 0(x0)"},     {0x00009123u, "sh x0, 2(x1)"},      {0xfe009f23u, "sh x0, -2(x1)"},
      {0x00002023u, "sw x0, 0(x0)"},     {0x0000a123u, "sw x0, 2(x1)"},      {0xfe00af23u, "sw x0, -2(x1)"},

      {0x00000013u, "addi x0, x0, 0"},   {0x00208013u, "addi x0, x1, 2"},    {0xffe08013u, "addi x0, x1, -2"},
      {0x00001013u, "slli x0, x0, 0"},   {0x00209013u, "slli x0, x1, 2"},
      {0x00002013u, "slti x0, x0, 0"},   {0x0020a013u, "slti x0, x1, 2"},    {0xffe0a013, "slti x0, x1, -2"},
      {0x00003013u, "sltiu x0, x0, 0"},  {0x0020b013u, "sltiu x0, x1, 2"},   {0xffe0b013u, "sltiu x0, x1, -2"},
      {0x00004013u, "xori x0, x0, 0"},   {0x0020c013u, "xori x0, x1, 2"},    {0xffe0c013u, "xori x0, x1, -2"},
      {0x00005013u, "srli x0, x0, 0"},   {0x0020d013u, "srli x0, x1, 2"},
      {0x40005013u, "srai x0, x0, 0"},   {0x4020d013u, "srai x0, x1, 2"},
      {0x00006013u, "ori x0, x0, 0"},    {0x0020e013u, "ori x0, x1, 2"},     {0xffe0e013u, "ori x0, x1, -2"},
      {0x00007013u, "andi x0, x0, 0"},   {0x0020f013u, "andi x0, x1, 2"},    {0xffe0f013, "andi x0, x1, -2"},

      {0x00000033u, "add x0, x0, x0"},   {0x00208033u, "add x0, x1, x2"},
      {0x00001033u, "sll x0, x0, x0"},   {0x00209033u, "sll x0, x1, x2"},
      {0x00002033u, "slt x0, x0, x0"},   {0x0020a033u, "slt x0, x1, x2"},
      {0x00003033u, "sltu x0, x0, x0"},  {0x0020b033u, "sltu x0, x1, x2"},
      {0x00004033u, "xor x0, x0, x0"},   {0x0020c033u, "xor x0, x1, x2"},
      {0x00005033u, "srl x0, x0, x0"},   {0x0020d033u, "srl x0, x1, x2"},
      {0x40005033u, "sra x0, x0, x0"},   {0x4020d033u, "sra x0, x1, x2"},
      {0x00006033u, "or x0, x0, x0"},    {0x0020e033u, "or x0, x1, x2"},
      {0x00007033u, "and x0, x0, x0"},   {0x0020f033u, "and x0, x1, x2"},

      {0x0c30000fu, "fence io, rw"},     {0x0690000fu, "fence or, iw"},  // real_opcode::fence

      {0x00000073u, "ecall"},            {0x00100073u, "ebreak"},  // real_opcode::env_ctl
    };
    // clang-format on

    run_tests<rv::detail::is_rv64i>({test_cases}, false);
}

TEST(rv_decode, rv32zifencei) {
    static constexpr std::pair<u32, std::string_view> test_cases[]{
      {0x0000100fu, "fence.i"},
    };

    run_tests<rv::detail::is_rv32zifencei>({test_cases}, false);
}

TEST(rv_decode, rv32zicsr) {
    /*static constexpr std::pair<u32, std::string_view> test_cases[]{

    };

    run_tests<rv::detail::is_rv32zicsr>({test_cases}, false);*/
}
