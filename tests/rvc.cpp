#include <gtest/gtest.h>

#include "./common.hpp"

#include <rv/rv.hpp>

TEST(RV_DECODE, RV32C) {
    // clang-format off
    static std::pair<u32, std::string_view> rv32c_test_cases[] {
        // quadrant 0
        {0b000'000'000'00'000'00, "c.invalid"}, // c.addi4spn x8, 0
        {0b000'000'000'00'001'00, "reserved(c.addi4spn)"}, // c.addi4spn x9, 0
        {0b000'000'000'00'111'00, "reserved(c.addi4spn)"}, // c.addi4spn x15, 0
        {0b000'000'000'01'000'00, "c.addi4spn"}, // c.addi4spn x8, 4
        {0b000'000'000'01'001'00, "c.addi4spn"}, // c.addi4spn x9, 4
        {0b000'111'111'11'111'00, "c.addi4spn"}, // c.addi4spn x15, 1020
        {0b001'000'000'00'000'00, "c.fld"}, // overridden by c.lq (RV128C)
        {0b001'111'111'11'111'00, "c.fld"}, // overridden by c.lq (RV128C)
        {0b010'000'000'00'000'00, "c.lw"},
        {0b010'111'111'11'111'00, "c.lw"},
        {0b011'000'000'00'000'00, "c.flw"}, // overridden by c.ld (RV64C, RV128C)
        {0b011'111'111'11'111'00, "c.flw"}, // overridden by c.ld (RV64C, RV128C)
        {0b100'000'000'00'000'00, "unknown"},
        {0b100'111'111'11'111'00, "unknown"},
        {0b101'000'000'00'000'00, "c.fsd"}, // overriden by c.sq (RV128)
        {0b101'111'111'11'111'00, "c.fsd"}, // overriden by c.sq (RV128)
        {0b110'000'000'00'000'00, "c.sw"},
        {0b110'111'111'11'111'00, "c.sw"},
        {0b111'000'000'00'000'00, "c.fsw"}, // overridden by c.sd (RV64C, RV128C)
        {0b111'111'111'11'111'00, "c.fsw"}, // overridden by c.sd (RV64C, RV128C)

        // quadrant 1
        // [ ] | [    ] [    ] []
        {0b000'0'00'000'00'000'01, "c.nop"},
        {0b000'1'00'000'00'000'01, "hint(c.nop)"},
        {0b000'0'00'000'11'111'01, "hint(c.nop)"},
        {0b000'0'00'001'00'000'01, "hint(c.addi)"},
        {0b000'0'11'111'00'000'01, "hint(c.addi)"},
        {0b000'1'00'001'00'000'01, "c.addi"},
        {0b000'0'00'001'00'001'01, "c.addi"},
        {0b000'1'00'001'11'111'01, "c.addi"},
        {0b001'0'00'000'00'000'01, "c.jal"}, // overridden by c.addiw (RV64C, RV128C)
        {0b001'1'11'111'11'111'01, "c.jal"}, // overridden by c.addiw (RV64C, RV128C)
        {0b010'0'00'000'00'000'01, "hint(c.li)"},
        {0b010'1'00'000'11'111'01, "hint(c.li)"},
        {0b010'0'00'001'00'000'01, "c.li"},
        {0b010'1'00'001'11'111'01, "c.li"},
        {0b010'0'11'111'00'000'01, "c.li"},
        {0b010'1'11'111'11'111'01, "c.li"},
        {0b011'0'00'010'00'000'01, "reserved(c.addi16sp)"},
        {0b011'0'00'010'00'001'01, "c.addi16sp"},
        {0b011'1'00'010'11'111'01, "c.addi16sp"},

        {0b011'0'00'001'00'000'01, "reserved(c.lui)"}, // res, nzimm=0; hint, rd=0; rd=2 means res(c.addi16sp)
        {0b011'0'00'000'00'000'01, "reserved(c.lui)"}, // both RES and HINT at the same time, RES comes out on top
        {0b011'1'00'000'00'000'01, "hint(c.lui)"},
        {0b011'1'00'000'11'111'01, "hint(c.lui)"},
        {0b011'0'00'001'00'001'01, "c.lui"},
        {0b011'1'11'111'11'111'01, "c.lui"},

        {0b100'0'00'000'00'000'01, "hint(c.srli)"}, // overridden by c.srli64 (RV128C)
        {0b100'0'00'001'00'000'01, "hint(c.srli)"}, // overridden by c.srli64 (RV128C)
        {0b100'0'00'111'00'000'01, "hint(c.srli)"}, // overridden by c.srli64 (RV128C)
        {0b100'1'00'000'00'000'01, "nse(c.srli)"}, // overridden by c.srli (RV64C, RV128C)
        {0b100'1'00'000'00'001'01, "nse(c.srli)"}, // overridden by c.srli (RV64C, RV128C)
        {0b100'1'00'001'00'000'01, "nse(c.srli)"}, // overridden by c.srli (RV64C, RV128C)
        {0b100'1'00'111'11'111'01, "nse(c.srli)"}, // overridden by c.srli (RV64C, RV128C)
        {0b100'0'00'000'00'001'01, "c.srli"},
        {0b100'0'00'001'00'001'01, "c.srli"},
        {0b100'0'00'111'11'111'01, "c.srli"},

        {0b100'0'01'000'00'000'01, "hint(c.srai)"}, // overridden by c.srai64 (RV128C)
        {0b100'0'01'001'00'000'01, "hint(c.srai)"}, // overridden by c.srai64 (RV128C)
        {0b100'0'01'111'00'000'01, "hint(c.srai)"}, // overridden by c.srai64 (RV128C)
        {0b100'1'01'000'00'000'01, "nse(c.srai)"}, // overridden by c.srai (RV64C, RV128C)
        {0b100'1'01'000'00'001'01, "nse(c.srai)"}, // overridden by c.srai (RV64C, RV128C)
        {0b100'1'01'001'00'000'01, "nse(c.srai)"}, // overridden by c.srai (RV64C, RV128C)
        {0b100'1'01'111'11'111'01, "nse(c.srai)"}, // overridden by c.srai (RV64C, RV128C)
        {0b100'0'01'000'00'001'01, "c.srai"},
        {0b100'0'01'001'00'001'01, "c.srai"},
        {0b100'0'01'111'11'111'01, "c.srai"},

        {0b100'0'10'000'00'000'01, "c.andi"},
        {0b100'1'10'000'00'000'01, "c.andi"},
        {0b100'1'10'111'00'000'01, "c.andi"},
        {0b100'1'10'111'11'111'01, "c.andi"},

        {0b100'0'11'000'00'000'01, "c.sub"},
        {0b100'0'11'111'00'111'01, "c.sub"},
        {0b100'0'11'000'01'000'01, "c.xor"},
        {0b100'0'11'111'01'111'01, "c.xor"},
        {0b100'0'11'000'10'000'01, "c.or"},
        {0b100'0'11'111'10'111'01, "c.or"},
        {0b100'0'11'000'11'000'01, "c.and"},
        {0b100'0'11'111'11'111'01, "c.and"},

        {0b100'1'11'000'00'000'01, "reserved(c.subw)"}, // overriden by c.subw (RV64C, RV128C)
        {0b100'1'11'111'00'111'01, "reserved(c.subw)"}, // overriden by c.subw (RV64C, RV128C)
        {0b100'1'11'000'01'000'01, "reserved(c.addw)"}, // overriden by c.addw (RV64C, RV128C)
        {0b100'1'11'111'01'111'01, "reserved(c.addw)"}, // overriden by c.addw (RV64C, RV128C)
        {0b100'1'11'000'10'000'01, "reserved(c.aluw10)"},
        {0b100'1'11'111'10'111'01, "reserved(c.aluw10)"},
        {0b100'1'11'000'11'000'01, "reserved(c.aluw11)"},
        {0b100'1'11'111'11'111'01, "reserved(c.aluw11)"},

        {0b101'0'00'000'00'000'01, "c.j"},
        {0b101'1'11'111'11'111'01, "c.j"},
        {0b110'0'00'000'00'000'01, "c.beqz"},
        {0b110'1'11'111'11'111'01, "c.beqz"},
        {0b111'0'00'000'00'000'01, "c.bnez"},
        {0b111'1'11'111'11'111'01, "c.bnez"},

        // quadrant 2
        // [ ] | [   ] [    ] []
        {0b000'1'00000'00000'10, "nse(c.slli)"}, // overridden by c.slli (RV64C, RV128C)
        {0b000'1'00000'11111'10, "nse(c.slli)"}, // overridden by c.slli (RV64C, RV128C)
        {0b000'0'00000'00000'10, "hint(c.slli64)"}, // overridden by c.slli64 (RV128C)
        {0b000'0'11111'00000'10, "hint(c.slli64)"}, // overridden by c.slli64 (RV128C)
        {0b000'0'00000'00001'10, "hint(c.slli)"},
        {0b000'0'00000'11111'10, "hint(c.slli)"},
        {0b000'0'00001'00001'10, "c.slli"},
        {0b000'0'00001'11111'10, "c.slli"},

        {0b001'0'00000'00000'10, "c.fldsp"}, // overddien by c.lqsp (RV128C)
        {0b001'1'00000'00000'10, "c.fldsp"}, // overddien by c.lqsp (RV128C)
        {0b001'0'00000'00001'10, "c.fldsp"}, // overddien by c.lqsp (RV128C)
        {0b001'0'00001'00000'10, "c.fldsp"}, // overddien by c.lqsp (RV128C)
        {0b001'1'11111'11111'10, "c.fldsp"}, // overddien by c.lqsp (RV128C)

        {0b010'0'00000'00000'10, "reserved(c.lwsp)"},
        {0b010'0'00000'00001'10, "reserved(c.lwsp)"},
        {0b010'1'00000'00000'10, "reserved(c.lwsp)"},
        {0b010'1'00000'11111'10, "reserved(c.lwsp)"},
        {0b010'0'00001'00000'10, "c.lwsp"},
        {0b010'0'00001'00001'10, "c.lwsp"},
        {0b010'1'00001'00000'10, "c.lwsp"},
        {0b010'1'00001'11111'10, "c.lwsp"},
        {0b010'1'11111'11111'10, "c.lwsp"},

        {0b011'0'00000'00000'10, "c.flwsp"}, // overridden by c.ldsp (RV64C, RV128C)
        {0b011'0'00000'00001'10, "c.flwsp"}, // overridden by c.ldsp (RV64C, RV128C)
        {0b011'1'00000'00000'10, "c.flwsp"}, // overridden by c.ldsp (RV64C, RV128C)
        {0b011'0'00001'00000'10, "c.flwsp"}, // overridden by c.ldsp (RV64C, RV128C)
        {0b011'1'11111'11111'10, "c.flwsp"}, // overridden by c.ldsp (RV64C, RV128C)

        {0b100'0'00000'00000'10, "reserved(c.jr)"},
        {0b100'0'00001'00000'10, "c.jr"},
        {0b100'0'11111'00000'10, "c.jr"},
        {0b100'0'00000'00001'10, "hint(c.mv)"},
        {0b100'0'00000'11111'10, "hint(c.mv)"},
        {0b100'0'00001'00001'10, "c.mv"},
        {0b100'0'11111'00001'10, "c.mv"},
        {0b100'1'00000'00000'10, "c.ebreak"},
        {0b100'1'00001'00000'10, "c.jalr"},
        {0b100'1'11111'00000'10, "c.jalr"},
        {0b100'1'00000'00001'10, "hint(c.add)"},
        {0b100'1'00001'00001'10, "c.add"},
        {0b100'1'11111'00001'10, "c.add"},

        {0b101'0'00000'00000'10, "c.fsdsp"}, // overridden by c.sqsp (RV128C)
        {0b101'0'00000'00001'10, "c.fsdsp"}, // overridden by c.sqsp (RV128C)
        {0b101'0'00001'00000'10, "c.fsdsp"}, // overridden by c.sqsp (RV128C)
        {0b101'1'00000'00000'10, "c.fsdsp"}, // overridden by c.sqsp (RV128C)
        {0b101'1'11111'11111'10, "c.fsdsp"}, // overridden by c.sqsp (RV128C)

        {0b110'0'00000'00000'10, "c.swsp"},
        {0b110'0'00000'00001'10, "c.swsp"},
        {0b110'0'00001'00000'10, "c.swsp"},
        {0b110'1'00000'00000'10, "c.swsp"},
        {0b110'1'11111'11111'10, "c.swsp"},

        {0b111'0'00000'00000'10, "c.fswsp"}, // overridden by c.sdsp (RV64C, RV128C)
        {0b111'0'00000'00001'10, "c.fswsp"}, // overridden by c.sdsp (RV64C, RV128C)
        {0b111'0'00001'00000'10, "c.fswsp"}, // overridden by c.sdsp (RV64C, RV128C)
        {0b111'1'00000'00000'10, "c.fswsp"}, // overridden by c.sdsp (RV64C, RV128C)
        {0b111'1'11111'11111'10, "c.fswsp"}, // overridden by c.sdsp (RV64C, RV128C)
    };
    // clang-format on

    run_tests<rv::detail::is_rv32c>({rv32c_test_cases}, false);
}

TEST(RV_DECODE, RV64C) {
    // clang-format off
    static std::pair<u32, std::string_view> rv64c_test_cases[] {
      {0b011'000'000'00'000'00, "c.ld"}, // overrides c.flw (RV32C)
      {0b011'111'111'11'111'00, "c.ld"}, // overrides c.flw (RV32C)
      {0b111'000'000'00'000'00, "c.sd"}, // overrides c.fsw (RV32C)
      {0b111'111'111'11'111'00, "c.sd"}, // overrides c.fsw (RV32C)

      // quadrant 1
      // [ ] | [    ] [    ] []
      {0b001'0'00'000'00'000'01, "reserved(c.addiw)"}, // overrides c.jal (RV32C)
      {0b001'1'00'000'11'111'01, "reserved(c.addiw)"}, // overrides c.jal (RV32C)
      {0b001'0'00'001'00'000'01, "c.addiw"}, // overrides c.jal (RV32C)
      {0b001'0'11'111'00'000'01, "c.addiw"}, // overrides c.jal (RV32C)
      {0b001'1'11'111'11'111'01, "c.addiw"}, // overrides c.jal (RV32C)
      {0b001'1'11'111'11'111'01, "c.addiw"}, // overrides c.jal (RV32C)
      {0b100'1'00'000'00'000'01, "c.srli"}, // overrides nse(c.srli) (RV32C)
      {0b100'1'00'000'00'001'01, "c.srli"}, // overrides nse(c.srli) (RV32C)
      {0b100'1'00'001'00'000'01, "c.srli"}, // overrides nse(c.srli) (RV32C)
      {0b100'1'00'111'11'111'01, "c.srli"}, // overrides nse(c.srli) (RV32C)
      {0b100'1'01'000'00'000'01, "c.srai"}, // overrides nse(c.srai) (RV32C)
      {0b100'1'01'000'00'001'01, "c.srai"}, // overrides nse(c.srai) (RV32C)
      {0b100'1'01'001'00'000'01, "c.srai"}, // overrides nse(c.srai) (RV32C)
      {0b100'1'01'111'11'111'01, "c.srai"}, // overrides nse(c.srai) (RV32C)

      {0b100'1'11'000'00'000'01, "c.subw"}, // overrides reserved(c.subw) (RV32C)
      {0b100'1'11'111'00'111'01, "c.subw"}, // overrides reserved(c.subw) (RV32C)
      {0b100'1'11'000'01'000'01, "c.addw"}, // overrides reserved(c.addw) (RV32C)
      {0b100'1'11'111'01'111'01, "c.addw"}, // overrides reserved(c.addw) (RV32C)

      // quadrant 2
      // [ ] | [   ] [    ] []
      {0b000'1'00000'00000'10, "c.slli"}, // overrides nse(c.slli) (RV32C)
      {0b000'1'00000'11111'10, "c.slli"}, // overrides nse(c.slli) (RV32C)

      {0b011'0'00000'00000'10, "reserved(c.ldsp)"}, // overrides c.flwsp (RV32C)
      {0b011'0'00000'00001'10, "reserved(c.ldsp)"}, // overrides c.flwsp (RV32C)
      {0b011'1'00000'00000'10, "reserved(c.ldsp)"}, // overrides c.flwsp (RV32C)
      {0b011'0'00001'00000'10, "c.ldsp"}, // overrides c.flwsp (RV32C)
      {0b011'1'11111'11111'10, "c.ldsp"}, // overrides c.flwsp (RV32C)

      {0b111'0'00000'00000'10, "c.sdsp"}, // overrides c.fswsp (RV32C)
      {0b111'0'00000'00001'10, "c.sdsp"}, // overrides c.fswsp (RV32C)
      {0b111'0'00001'00000'10, "c.sdsp"}, // overrides c.fswsp (RV32C)
      {0b111'1'00000'00000'10, "c.sdsp"}, // overrides c.fswsp (RV32C)
      {0b111'1'11111'11111'10, "c.sdsp"}, // overrides c.fswsp (RV32C)
    };
    // clang-format on

    run_tests<rv::detail::is_rv64c>({rv64c_test_cases}, false);
}

TEST(RV_DECODE, RV128C) {
    // clang-format off
    static std::pair<u32, std::string_view> rv128c_test_cases[] {
        {0b001'000'000'00'000'00, "c.lq"}, // overrides c.fld (RV32C, RV64C)
        {0b001'111'111'11'111'00, "c.lq"}, // overrides c.fld (RV32C, RV64C)
        {0b101'000'000'00'000'00, "c.sq"}, // overrides c.fsd (RV32C, RV64C)
        {0b101'111'111'11'111'00, "c.sq"}, // overrides c.fsd (RV32C, RV64C)

        //quadrant 1
        {0b100'0'00'000'00'000'01, "c.srli64"}, // overrides hint(c.srli) (RV32C, RV64C)
        {0b100'0'00'001'00'000'01, "c.srli64"}, // overrides hint(c.srli) (RV32C, RV64C)
        {0b100'0'00'111'00'000'01, "c.srli64"}, // overrides hint(c.srli) (RV32C, RV64C)
        {0b100'0'01'000'00'000'01, "c.srai64"}, // overrides hint(c.srai) (RV32C, RV64C)
        {0b100'0'01'001'00'000'01, "c.srai64"}, // overrides hint(c.srai) (RV32C, RV64C)
        {0b100'0'01'111'00'000'01, "c.srai64"}, // overrides hint(c.srai) (RV32C, RV64C)

        // quadrant 2
        // [ ] | [   ] [    ] []
        {0b000'0'00000'00000'10, "c.slli64"}, // overridden by hint(c.slli64) (RV32C, RV64C)
        {0b000'0'11111'00000'10, "c.slli64"}, // overridden by hint(c.slli64) (RV32C, RV64C)

        {0b001'0'00000'00000'10, "reserved(c.lqsp)"}, // overrides c.fldsp (RV32C, RV64C)
        {0b001'1'00000'00000'10, "reserved(c.lqsp)"}, // overrides c.fldsp (RV32C, RV64C)
        {0b001'0'00000'00001'10, "reserved(c.lqsp)"}, // overrides c.fldsp (RV32C, RV64C)
        {0b001'0'00001'00000'10, "c.lqsp"}, // overrides c.fldsp (RV32C, RV64C)
        {0b001'1'11111'11111'10, "c.lqsp"}, // overrides c.fldsp (RV32C, RV64C)

        {0b101'0'00000'00000'10, "c.sqsp"}, // overrides c.fsdsp (RV32C RV64C)
        {0b101'0'00000'00001'10, "c.sqsp"}, // overrides c.fsdsp (RV32C RV64C)
        {0b101'0'00001'00000'10, "c.sqsp"}, // overrides c.fsdsp (RV32C RV64C)
        {0b101'1'00000'00000'10, "c.sqsp"}, // overrides c.fsdsp (RV32C RV64C)
        {0b101'1'11111'11111'10, "c.sqsp"}, // overrides c.fsdsp (RV32C RV64C)
    };
    // clang-format on

    run_tests<rv::detail::is_rv128c>({rv128c_test_cases}, false);
}
