#pragma once

#include <stuff/bit.hpp>

#include <charconv>

namespace rv::detail {

template<typename RiscV>
using functor_hint = functor_nop<RiscV>;

template<typename RiscV>
using functor_reserved = functor_nyi<RiscV>;

template<typename RiscV>
using functor_nse = functor_nyi<RiscV>;


namespace assembler {

constexpr auto asm_immediate(u32 imm_11_0, reg rs_1, u32 funct_3, reg rd, u32 opcode) {
    return (imm_11_0 << 20) | (static_cast<u32>(rs_1) << 15) | (funct_3 << 12) | (static_cast<u32>(rd) << 7) | opcode;
}

template<alu_action Action, bool Word = false>
constexpr auto alu(reg rd, reg rs_1, reg rs_2) -> u32 {
    constexpr auto base_mask = ([] consteval {
        u32 mask = 0;
        switch (Action) {
            case alu_action::add: mask = 0b000'00000'01100'11; break;
            case alu_action::sub: mask = 0b000'00000'01100'11 | 0x4000'0000; break;
            case alu_action::sll: mask = 0b001'00000'01100'11; break;
            case alu_action::slt: mask = 0b010'00000'01100'11; break;
            case alu_action::sltu: mask = 0b011'00000'01100'11; break;
            case alu_action::bxor: mask = 0b100'00000'01100'11; break;
            case alu_action::srl: mask = 0b101'00000'01100'11; break;
            case alu_action::sra: mask = 0b101'00000'01100'11 | 0x4000'0000; break;
            case alu_action::bor: mask = 0b110'00000'01100'11; break;
            case alu_action::band: mask = 0b111'00000'01100'11; break;
            default: throw 1;
        }
        return mask;
    })();

    constexpr auto mask = base_mask | (Word * 0x0008u);

    return mask                            //
         | (static_cast<u32>(rd) << 7)     //
         | (static_cast<u32>(rs_1) << 15)  //
         | (static_cast<u32>(rs_2) << 20);
}

template<alu_action Action, bool Word = false>
constexpr auto alu_i(reg rd, reg rs_1, i32 imm) -> u32 {
    constexpr auto base_mask = ([] consteval {
        u32 mask = 0;
        switch (Action) {
            case alu_action::add: mask = 0b000'00000'00100'11; break;
            case alu_action::sll: mask = 0b001'00000'00100'11; break;
            case alu_action::slt: mask = 0b010'00000'00100'11; break;
            case alu_action::sltu: mask = 0b011'00000'00100'11; break;
            case alu_action::bxor: mask = 0b100'00000'00100'11; break;
            case alu_action::srl: mask = 0b101'00000'00100'11; break;
            case alu_action::sra: mask = 0b101'00000'00100'11 | 0x4000'0000; break;
            case alu_action::bor: mask = 0b110'00000'00100'11; break;
            case alu_action::band: mask = 0b111'00000'00100'11; break;
            default: throw 1;
        }
        return mask;
    })();

    constexpr auto mask = base_mask | (Word * 0x0008u);  // addi x?, x?, ?

    return mask                            //
         | (static_cast<u32>(rd) << 7)     //
         | (static_cast<u32>(rs_1) << 15)  //
         | ((static_cast<u32>(imm) & 0xFFF) << 20);
}

constexpr auto lui(reg rd, i32 imm) -> u32 {
    constexpr auto addi_base = 0x0000'0037u;  // lui x?, ?

    return addi_base                     //
         | (static_cast<u32>(rd) << 7u)  //
         | ((static_cast<u32>(imm) & 0xF'FFFFu) << 12u);
}

enum class ld_st_type : u32 {
    byte = 0b000,
    half = 0b001,
    word = 0b010,
    dword = 0b011,
    ubyte = 0b100,
    uhalf = 0b101,
    uword = 0b110,
    // udword = 0b111,
};

template<ld_st_type Type>
constexpr auto store(reg rs_2, i32 offset, reg rs_1) -> u32 {
    constexpr auto sd_base = 0b00000'00'00000'00000'000'00000'01000'11u | (static_cast<u32>(Type) << 12);  // s? x?, ?(x2)

    return sd_base                                                 //
         | (static_cast<u32>(rs_2) << 20)                          //
         | (static_cast<u32>(rs_1) << 15)                          //
         | ((static_cast<u32>(offset) & 0b1111'1110'0000u) << 20)  //
         | ((static_cast<u32>(offset) & 0b0000'0001'1111u) << 7);
}

template<ld_st_type Type>
constexpr auto load(reg rd, i32 offset, reg rs_1) -> u32 {
    constexpr auto sd_base = 0b00000'00'00000'00000'000'00000'00000'11u | (static_cast<u32>(Type) << 12);  // s? x?, ?(x2)

    return sd_base                         //
         | (static_cast<u32>(rd) << 7)     //
         | (static_cast<u32>(rs_1) << 15)  //
         | ((static_cast<u32>(offset) & 0xFFF) << 20);
}

constexpr auto jalr(reg rd, reg rs_1, i32 offset) -> u32 { return asm_immediate(offset, rs_1, 0b000, rd, 0b11001'11); }

constexpr auto jal(reg rd, i32 offset) -> u32 {
    constexpr auto jal_base = 0b11011'11u;

    return jal_base                                          //
         | (static_cast<u32>(rd) << 7u)                      //
         | (static_cast<u32>(offset) & 0x000F'F000u)         //
         | (static_cast<u32>(offset << 20u) & 0x7FE0'0000u)  //
         | (static_cast<u32>(offset << 9u) & 0x0010'0000u)   //
         | (static_cast<u32>(offset << 11u) & 0x8000'0000u);
}

enum class branch_type : u32 {
    equal = 0b000,
    not_equal = 0b001,
    less_than = 0b100,
    greater_equal = 0b101,
    less_than_unsigned = 0b110,
    greater_equal_unsigned = 0b111,
};

template<branch_type Type>
constexpr auto branch(reg rs_1, reg rs_2, i32 offset) -> u32 {
    constexpr auto mask = 0b11000'11 | (static_cast<u32>(Type) << 12);

    return mask                                              //
         | (static_cast<u32>(rs_1) << 15)                    //
         | (static_cast<u32>(rs_2) << 20)                    //
         | ((static_cast<u32>(offset) >> 4) & 0x0000'0080)   //
         | ((static_cast<u32>(offset) << 7) & 0x0000'0F00)   //
         | ((static_cast<u32>(offset) << 20) & 0x7E00'0000)  //
         | ((static_cast<u32>(offset) << 19) & 0x8000'0000);
}

}  // namespace assembler

struct translator_addi4spn {
    constexpr auto operator()(u32 compressed_word) const -> u32 {
        const auto rd = static_cast<reg>(stf::bit::extract<u32, 4, "2:0">(compressed_word) + 8u);
        const auto nz_uimm = stf::bit::extract<u32, 12, "5:4|9:6|2|3">(compressed_word);

        return assembler::alu_i<alu_action::add, false>(rd, reg::x2, nz_uimm);
    }
};

struct translator_addi16sp {
    constexpr auto operator()(u32 compressed_word) const -> u32 {
        const auto nz_imm = stf::bit::extract<u32, 12, "9|11~7|4|6|8:7|5">(compressed_word);

        return assembler::alu_i<alu_action::add, false>(reg::x2, reg::x2, (i32)arith::sext<u32, 10>(nz_imm));
    }
};

struct translator_add {
    constexpr auto operator()(u32 compressed_word) const -> u32 {
        const auto nz_rs_1_rd = static_cast<reg>(stf::bit::extract<u32, 11, "4:0">(compressed_word));
        const auto nz_rs_2 = static_cast<reg>(stf::bit::extract<u32, 6, "4:0">(compressed_word));

        return assembler::alu<alu_action::add>(nz_rs_1_rd, nz_rs_1_rd, nz_rs_2);
    }
};

template<alu_action Action, bool Word = false>
    requires(                                                                                     //
      (Action == alu_action::bxor || Action == alu_action::bor || Action == alu_action::band) ||  //
      (Action == alu_action::sub || (Action == alu_action::add && Word))
    )
struct translator_bitwise {
    constexpr auto operator()(u32 compressed_word) const -> u32 {
        const auto rs_1_rd = static_cast<reg>(stf::bit::extract<u32, 9, "2:0">(compressed_word) + 8u);
        const auto rs_2 = static_cast<reg>(stf::bit::extract<u32, 4, "2:0">(compressed_word) + 8u);

        if (Action == alu_action::sub) {
            std::ignore = std::ignore;
        }

        return assembler::alu<Action, Word>(rs_1_rd, rs_1_rd, rs_2);
    }
};

template<bool Word = false>
struct translator_addi {
    constexpr auto operator()(u32 compressed_word) const -> u32 {
        const auto nz_rs_1_rd = static_cast<reg>(stf::bit::extract<u32, 11, "4:0">(compressed_word));

        // nonzero iff. `Word`
        const auto nz_imm = stf::bit::extract<u32, 12, "5|11~7|4:0">(compressed_word);

        return assembler::alu_i<alu_action::add, Word>(nz_rs_1_rd, nz_rs_1_rd, (i32)arith::sext<u32, 6>(nz_imm));
    }
};

struct translator_andi {
    constexpr auto operator()(u32 compressed_word) const -> u32 {
        const auto nz_rs_1_rd = static_cast<reg>(stf::bit::extract<u32, 9, "2:0">(compressed_word) + 8u);
        const auto imm = stf::bit::extract<u32, 12, "5|11~7|4:0">(compressed_word);

        return assembler::alu_i<alu_action::band>(nz_rs_1_rd, nz_rs_1_rd, (i32)arith::sext<u32, 6>(imm));
    }
};

template<bool Arithmetic>
struct translator_srxi {
    constexpr auto operator()(u32 compressed_word) const -> u32 {
        const auto nz_rs_1_rd = static_cast<reg>(stf::bit::extract<u32, 9, "2:0">(compressed_word) + 8u);
        const auto nz_uimm = stf::bit::extract<u32, 12, "5|11~7|4:0">(compressed_word);

        return assembler::alu_i < Arithmetic ? alu_action::sra : alu_action::srl > (nz_rs_1_rd, nz_rs_1_rd, nz_uimm);
    }
};

struct translator_slli {
    constexpr auto operator()(u32 compressed_word) const -> u32 {
        const auto nz_rs_1_rd = static_cast<reg>(stf::bit::extract<u32, 11, "4:0">(compressed_word));
        const auto nz_uimm = stf::bit::extract<u32, 12, "5|11~7|4:0">(compressed_word);

        return assembler::alu_i<alu_action::sll>(nz_rs_1_rd, nz_rs_1_rd, nz_uimm);
    }
};

struct translator_sdsp {
    constexpr auto operator()(u32 compressed_word) const -> u32 {
        const auto rs_2 = static_cast<reg>(stf::bit::extract<u32, 6, "4:0">(compressed_word));
        const auto uimm = stf::bit::extract<u32, 12, "5:3|8:6">(compressed_word);

        return assembler::store<assembler::ld_st_type::dword>(rs_2, uimm, reg::x2);
    }
};

struct translator_swsp {
    constexpr auto operator()(u32 compressed_word) const -> u32 {
        const auto rs_2 = static_cast<reg>(stf::bit::extract<u32, 6, "4:0">(compressed_word));
        const auto uimm = stf::bit::extract<u32, 12, "5:2|7:6">(compressed_word);

        return assembler::store<assembler::ld_st_type::word>(rs_2, uimm, reg::x2);
    }
};

struct translator_sw {
    constexpr auto operator()(u32 compressed_word) const -> u32 {
        const auto rs_1 = static_cast<reg>(stf::bit::extract<u32, 9, "2:0">(compressed_word) + 8u);
        const auto rs_2 = static_cast<reg>(stf::bit::extract<u32, 4, "2:0">(compressed_word) + 8u);
        const auto uimm = stf::bit::extract<u32, 12, "5:3|9~7|2|6">(compressed_word);

        return assembler::store<assembler::ld_st_type::word>(rs_2, uimm, rs_1);
    }
};

struct translator_sd {
    constexpr auto operator()(u32 compressed_word) const -> u32 {
        const auto rs_1 = static_cast<reg>(stf::bit::extract<u32, 9, "2:0">(compressed_word) + 8u);
        const auto rs_2 = static_cast<reg>(stf::bit::extract<u32, 4, "2:0">(compressed_word) + 8u);
        const auto uimm = stf::bit::extract<u32, 12, "5:3|9~7|7:6">(compressed_word);

        return assembler::store<assembler::ld_st_type::dword>(rs_2, uimm, rs_1);
    }
};

struct translator_lui {
    constexpr auto operator()(u32 compressed_word) const -> u32 {
        const auto nz_rd = static_cast<reg>(stf::bit::extract<u32, 11, "4:0">(compressed_word));
        const auto nz_imm = stf::bit::extract<u32, 12, "17|11~7|16:12">(compressed_word) >> 12;

        return assembler::lui(nz_rd, (i32)arith::sext<u32, 6>(nz_imm));
    }
};

struct translator_li {
    constexpr auto operator()(u32 compressed_word) const -> u32 {
        const auto nz_rd = static_cast<reg>(stf::bit::extract<u32, 11, "4:0">(compressed_word));
        const auto imm = stf::bit::extract<u32, 12, "5|11~7|4:0">(compressed_word);

        return assembler::alu_i<alu_action::add, false>(nz_rd, reg::x0, (i32)arith::sext<u32, 6>(imm));
    }
};

struct translator_addw {
    constexpr auto operator()(u32 compressed_word) const -> u32 {
        const auto rs_1_rd = static_cast<reg>(stf::bit::extract<u32, 9, "2:0">(compressed_word) + 8u);
        const auto rs_2 = static_cast<reg>(stf::bit::extract<u32, 4, "2:0">(compressed_word) + 8u);

        return assembler::alu<alu_action::add, true>(rs_1_rd, rs_1_rd, rs_2);
    }
};

struct translator_mv {
    constexpr auto operator()(u32 compressed_word) const -> u32 {
        const auto nz_rd = static_cast<reg>(stf::bit::extract<u32, 11, "4:0">(compressed_word));
        const auto nz_rs_2 = static_cast<reg>(stf::bit::extract<u32, 6, "4:0">(compressed_word));

        return assembler::alu<alu_action::add>(nz_rd, reg::x0, nz_rs_2);
    }
};

struct translator_j {
    constexpr auto operator()(u32 compressed_word) const -> u32 {
        const auto imm = stf::bit::extract<u32, 12, "11|4|9:8|10|6|7|3:1|5">(compressed_word);

        return assembler::jal(reg::x0, (i32)arith::sext<u32, 12>(imm));
    }
};

struct translator_jr {
    constexpr auto operator()(u32 compressed_word) const -> u32 {
        const auto nz_rs_1 = static_cast<reg>(stf::bit::extract<u32, 11, "4:0">(compressed_word));

        return assembler::jalr(reg::x0, nz_rs_1, 0);
    }
};

struct translator_jal {
    constexpr auto operator()(u32 compressed_word) const -> u32 {
        const auto imm = stf::bit::extract<u32, 12, "11|4|9:8|10|6|7|3:1|5">(compressed_word);

        return assembler::jal(reg::x1, (i32)arith::sext<u32, 12>(imm));
    }
};

struct translator_jalr {
    constexpr auto operator()(u32 compressed_word) const -> u32 {
        const auto rs_1 = static_cast<reg>(stf::bit::extract<u32, 11, "4:0">(compressed_word));

        return assembler::jalr(reg::x1, rs_1, 0);
    }
};

template<bool Equal>
struct translator_branch {
    constexpr auto operator()(u32 compressed_word) const -> u32 {
        const auto rs_1 = static_cast<reg>(stf::bit::extract<u32, 9, "2:0">(compressed_word) + 8u);
        const auto imm = stf::bit::extract<u32, 12, "8|4:3|9~7|7:6|2:1|5">(compressed_word);

        constexpr auto branch_type = Equal ? assembler::branch_type::equal : assembler::branch_type::not_equal;

        return assembler::branch<branch_type>(rs_1, reg::x0, (i32)arith::sext<u32, 9>(imm));
    }
};

struct translator_ldsp {
    constexpr auto operator()(u32 compressed_word) const -> u32 {
        const auto uimm = stf::bit::extract<u32, 12, "5|11~7|4:3|8:6">(compressed_word);
        const auto nz_rd = static_cast<reg>(stf::bit::extract<u32, 11, "4:0">(compressed_word));

        return assembler::load<assembler::ld_st_type::dword>(nz_rd, uimm, reg::x2);
    }
};

struct translator_lwsp {
    constexpr auto operator()(u32 compressed_word) const -> u32 {
        const auto nz_rd = static_cast<reg>(stf::bit::extract<u32, 11, "4:0">(compressed_word));
        const auto uimm = stf::bit::extract<u32, 12, "5|11~7|4:2|7:6">(compressed_word);

        return assembler::load<assembler::ld_st_type::word>(nz_rd, uimm, reg::x2);
    }
};

struct translator_lw {
    constexpr auto operator()(u32 compressed_word) const -> u32 {
        const auto rs_1 = static_cast<reg>(stf::bit::extract<u32, 9, "2:0">(compressed_word) + 8u);
        const auto rd = static_cast<reg>(stf::bit::extract<u32, 4, "2:0">(compressed_word) + 8u);
        const auto uimm = stf::bit::extract<u32, 12, "5:3|9~7|2|6">(compressed_word);

        return assembler::load<assembler::ld_st_type::word>(rd, uimm, rs_1);
    }
};

struct translator_ld {
    constexpr auto operator()(u32 compressed_word) const -> u32 {
        const auto rs_1 = static_cast<reg>(stf::bit::extract<u32, 9, "2:0">(compressed_word) + 8u);
        const auto rd = static_cast<reg>(stf::bit::extract<u32, 4, "2:0">(compressed_word) + 8u);
        const auto uimm = stf::bit::extract<u32, 12, "5:3|9~7|7:6">(compressed_word);

        return assembler::load<assembler::ld_st_type::dword>(rd, uimm, rs_1);
    }
};

// clang-format off

template<typename RiscV>
inline constexpr auto is_rv32c = instruction_set(
    std::type_identity<RiscV> {},
    // quadrant 0
    RV_QUICK_INSN(RiscV, "c.invalid", RV32C, c_immediate, (bit_matcher<u32>{0xFFFF, 0x0000}), functor_nyi<RiscV>, mnemonic_only_formatter),
    RV_QUICK_INSN(RiscV, "reserved(c.addi4spn)", RV32C, c_immediate, (bit_matcher<u32>{0xFFE3, 0x0000}), functor_reserved<RiscV>, default_formatter),
    RV_QUICK_INSN_TR(RiscV, "c.addi4spn", RV32C, c_immediate, (bit_matcher<u32>{0xE003, 0x0000}), translator_addi4spn, default_formatter),
    RV_QUICK_INSN(RiscV, "c.fld", RV32C, c_immediate, (bit_matcher<u32>{0xE003, 0x2000}), functor_nyi<RiscV>, default_formatter), // overridden by c.lq (RV128C)
    RV_QUICK_INSN_TR(RiscV, "c.lw", RV32C, c_immediate, (bit_matcher<u32>{0xE003, 0x4000}), translator_lw, default_formatter),
    RV_QUICK_INSN(RiscV, "c.flw", RV32C, c_immediate, (bit_matcher<u32>{0xE003, 0x6000}), functor_nyi<RiscV>, default_formatter), // overridden by c.ld (RV64C, RV128C)
    RV_QUICK_INSN(RiscV, "c.fsd", RV32C, c_immediate, (bit_matcher<u32>{0xE003, 0xA000}), functor_nyi<RiscV>, default_formatter), // overriden by c.sq (RV128)
    RV_QUICK_INSN_TR(RiscV, "c.sw", RV32C, c_immediate, (bit_matcher<u32>{0xE003, 0xC000}), translator_sw, default_formatter),
    RV_QUICK_INSN(RiscV, "c.fsw", RV32C, c_immediate, (bit_matcher<u32>{0xE003, 0xE000}), functor_nyi<RiscV>, default_formatter), // overridden by c.sd (RV64C, RV128C)

    // quadrant 1
    RV_QUICK_INSN(RiscV, "c.nop", RV32C, c_immediate, (bit_matcher<u32>{0xFFFF, 0x0001}), functor_nop<RiscV>, default_formatter),
    RV_QUICK_INSN(RiscV, "hint(c.nop)", RV32C, c_immediate, (bit_matcher<u32>{0xEF83, 0x0001}), functor_hint<RiscV>, default_formatter),
    RV_QUICK_INSN(RiscV, "hint(c.addi)", RV32C, c_immediate, (bit_matcher<u32>{0xF07F, 0x0001}), functor_hint<RiscV>, default_formatter),
    RV_QUICK_INSN_TR(RiscV, "c.addi", RV32C, c_immediate, (bit_matcher<u32>{0xE003, 0x0001}), translator_addi<false>, default_formatter),
    RV_QUICK_INSN_TR(RiscV, "c.jal", RV32C, c_immediate, (bit_matcher<u32>{0xE003, 0x2001}), translator_jal, default_formatter), // overridden by c.addiw (RV64C, RV128C)
    RV_QUICK_INSN(RiscV, "hint(c.li)", RV32C, c_immediate, (bit_matcher<u32>{0xEF83, 0x4001}), functor_hint<RiscV>, default_formatter),
    RV_QUICK_INSN_TR(RiscV, "c.li", RV32C, c_immediate, (bit_matcher<u32>{0xE003, 0x4001}), translator_li, default_formatter),
    RV_QUICK_INSN(RiscV, "reserved(c.addi16sp)", RV32C, c_immediate, (bit_matcher<u32>{0xFFFF, 0x6101}), functor_reserved<RiscV>, default_formatter),
    RV_QUICK_INSN_TR(RiscV, "c.addi16sp", RV32C, c_immediate, (bit_matcher<u32>{0xEF83, 0x6101}), translator_addi16sp, default_formatter),

    RV_QUICK_INSN(RiscV, "reserved(c.lui)", RV32C, c_immediate, (bit_matcher<u32>{0xF07F, 0x6001}), functor_reserved<RiscV>, default_formatter),
    RV_QUICK_INSN(RiscV, "hint(c.lui)", RV32C, c_immediate, (bit_matcher<u32>{0xEF81, 0x6001}), functor_hint<RiscV>, default_formatter),
    RV_QUICK_INSN_TR(RiscV, "c.lui", RV32C, c_immediate, (bit_matcher<u32>{0xE003, 0x6001}), translator_lui, default_formatter),

    RV_QUICK_INSN(RiscV, "hint(c.srli)", RV32C, c_immediate, (bit_matcher<u32>{0xFC7D, 0x8001}), functor_hint<RiscV>, default_formatter), // overriden by c.srli64 (RV128C)
    RV_QUICK_INSN(RiscV, "nse(c.srli)", RV32C, c_immediate, (bit_matcher<u32>{0xFC03, 0x9001}), functor_nse<RiscV>, default_formatter), // overriden by c.srli (RV64C, RV128C)
    RV_QUICK_INSN_TR(RiscV, "c.srli", RV32C, c_immediate, (bit_matcher<u32>{0xEC03, 0x8001}), translator_srxi<false>, default_formatter),

    RV_QUICK_INSN(RiscV, "hint(c.srai)", RV32C, c_immediate, (bit_matcher<u32>{0xFC7D, 0x8401}), functor_hint<RiscV>, default_formatter), // overriden by c.srai64 (RV128C)
    RV_QUICK_INSN(RiscV, "nse(c.srai)", RV32C, c_immediate, (bit_matcher<u32>{0xFC03, 0x9401}), functor_nse<RiscV>, default_formatter), // overriden by c.srai (RV64C, RV128C)
    RV_QUICK_INSN_TR(RiscV, "c.srai", RV32C, c_immediate, (bit_matcher<u32>{0xEC03, 0x8401}), translator_srxi<true>, default_formatter),

    RV_QUICK_INSN_TR(RiscV, "c.andi", RV32C, c_immediate, (bit_matcher<u32>{0xEC03, 0x8801}), translator_andi, default_formatter),
    RV_QUICK_INSN_TR(RiscV, "c.sub", RV32C, c_immediate, (bit_matcher<u32>{0xFC63, 0x8C01}), (translator_bitwise<alu_action::sub, false>), default_formatter),
    RV_QUICK_INSN_TR(RiscV, "c.xor", RV32C, c_immediate, (bit_matcher<u32>{0xFC63, 0x8C21}), translator_bitwise<alu_action::bxor>, default_formatter),
    RV_QUICK_INSN_TR(RiscV, "c.or", RV32C, c_immediate, (bit_matcher<u32>{0xFC63, 0x8C41}), translator_bitwise<alu_action::bor>, default_formatter),
    RV_QUICK_INSN_TR(RiscV, "c.and", RV32C, c_immediate, (bit_matcher<u32>{0xFC63, 0x8C61}), translator_bitwise<alu_action::band>, default_formatter),

    RV_QUICK_INSN(RiscV, "reserved(c.subw)", RV32C, c_immediate, (bit_matcher<u32>{0xFC63, 0x9C01}), functor_reserved<RiscV>, default_formatter), // overriden by c.subw (RV64C, RV128C)
    RV_QUICK_INSN(RiscV, "reserved(c.addw)", RV32C, c_immediate, (bit_matcher<u32>{0xFC63, 0x9C21}), functor_reserved<RiscV>, default_formatter), // overriden by c.addw (RV64C, RV128C)
    RV_QUICK_INSN(RiscV, "reserved(c.aluw10)", RV32C, c_immediate, (bit_matcher<u32>{0xFC63, 0x9C41}), functor_reserved<RiscV>, default_formatter),
    RV_QUICK_INSN(RiscV, "reserved(c.aluw11)", RV32C, c_immediate, (bit_matcher<u32>{0xFC63, 0x9C61}), functor_reserved<RiscV>, default_formatter),

    RV_QUICK_INSN_TR(RiscV, "c.j", RV32C, c_immediate, (bit_matcher<u32>{0xE003, 0xA001}), translator_j, default_formatter),
    RV_QUICK_INSN_TR(RiscV, "c.beqz", RV32C, c_immediate, (bit_matcher<u32>{0xE003, 0xC001}), translator_branch<true>, default_formatter),
    RV_QUICK_INSN_TR(RiscV, "c.bnez", RV32C, c_immediate, (bit_matcher<u32>{0xE003, 0xE001}), translator_branch<false>, default_formatter),

    // quadrant 2
    RV_QUICK_INSN(RiscV, "nse(c.slli)", RV32C, c_immediate, (bit_matcher<u32>{0xF003, 0x1002}), functor_reserved<RiscV>, default_formatter), // overridden by c.slli (RV64C, RV128C); NSE, nzuimm[5]=1
    RV_QUICK_INSN(RiscV, "hint(c.slli64)", RV32C, c_immediate, (bit_matcher<u32>{0xF07F, 0x0002}), functor_hint<RiscV>, default_formatter), // overridden by c.slli64 (RV128C); hint nzuimm=0
    RV_QUICK_INSN(RiscV, "hint(c.slli)", RV32C, c_immediate, (bit_matcher<u32>{0xEF83, 0x0002}), functor_hint<RiscV>, default_formatter), // hint rd=0
    RV_QUICK_INSN_TR(RiscV, "c.slli", RV32C, c_immediate, (bit_matcher<u32>{0xE003, 0x0002}), translator_slli, default_formatter),

    RV_QUICK_INSN(RiscV, "c.fldsp", RV32C, c_immediate, (bit_matcher<u32>{0xE003, 0x2002}), functor_nyi<RiscV>, default_formatter), // overridden by c.lqsp (RV128C)

    RV_QUICK_INSN(RiscV, "reserved(c.lwsp)", RV32C, c_immediate, (bit_matcher<u32>{0xEF83, 0x4002}), functor_reserved<RiscV>, default_formatter), // RES, rd=0
    RV_QUICK_INSN_TR(RiscV, "c.lwsp", RV32C, c_immediate, (bit_matcher<u32>{0xE003, 0x4002}), translator_lwsp, default_formatter),

    RV_QUICK_INSN(RiscV, "c.flwsp", RV32C, c_immediate, (bit_matcher<u32>{0xE003, 0x6002}), functor_nyi<RiscV>, default_formatter), // overridden by c.ldsp (RV64C, RV128C)

    RV_QUICK_INSN(RiscV, "reserved(c.jr)", RV32C, c_immediate, (bit_matcher<u32>{0xFFFF, 0x8002}), functor_reserved<RiscV>, default_formatter),
    RV_QUICK_INSN_TR(RiscV, "c.jr", RV32C, c_immediate, (bit_matcher<u32>{0xF07F, 0x8002}), translator_jr, default_formatter),
    RV_QUICK_INSN(RiscV, "hint(c.mv)", RV32C, c_immediate, (bit_matcher<u32>{0xFF83, 0x8002}), functor_hint<RiscV>, default_formatter),
    RV_QUICK_INSN_TR(RiscV, "c.mv", RV32C, c_immediate, (bit_matcher<u32>{0xF003, 0x8002}), translator_mv, default_formatter),

    RV_QUICK_INSN(RiscV, "c.ebreak", RV32C, c_immediate, (bit_matcher<u32>{0xFFFF, 0x9002}), functor_nyi<RiscV>, default_formatter),
    RV_QUICK_INSN_TR(RiscV, "c.jalr", RV32C, c_immediate, (bit_matcher<u32>{0xF07F, 0x9002}), translator_jalr, default_formatter),
    RV_QUICK_INSN(RiscV, "hint(c.add)", RV32C, c_immediate, (bit_matcher<u32>{0xFF83, 0x9002}), functor_hint<RiscV>, default_formatter),
    RV_QUICK_INSN_TR(RiscV, "c.add", RV32C, c_immediate, (bit_matcher<u32>{0xF003, 0x9002}), translator_add, default_formatter),

    RV_QUICK_INSN(RiscV, "c.fsdsp", RV32C, c_immediate, (bit_matcher<u32>{0xE003, 0xA002}), functor_nyi<RiscV>, default_formatter), // overridden by c.sqsp (RV128C)
    RV_QUICK_INSN_TR(RiscV, "c.swsp", RV32C, c_immediate, (bit_matcher<u32>{0xE003, 0xC002}), translator_swsp, default_formatter),
    RV_QUICK_INSN(RiscV, "c.fswsp", RV32C, c_immediate, (bit_matcher<u32>{0xE003, 0xE002}), functor_nyi<RiscV>, default_formatter) // overridden by c.sdsp (RV64C, RV128C)*/
);

template<typename RiscV>
inline constexpr auto is_rv64c = instruction_set(instruction_set(
    std::type_identity<RiscV> {},

    // quadrant 0
    RV_QUICK_INSN_TR(RiscV, "c.ld", RV64C, c_immediate, (bit_matcher<u32>{0xE003, 0x6000}), translator_ld, default_formatter), // overrides c.flw (RV32C)
    RV_QUICK_INSN_TR(RiscV, "c.sd", RV64C, c_immediate, (bit_matcher<u32>{0xE003, 0xE000}), translator_sd, default_formatter), // overrides c.fsw (RV32C)

    // quadrant 1
    RV_QUICK_INSN(RiscV, "reserved(c.addiw)", RV64C, c_immediate, (bit_matcher<u32>{0xEF83, 0x2001}), functor_reserved<RiscV>, default_formatter), // overrides c.jal (RV32C)
    RV_QUICK_INSN_TR(RiscV, "c.addiw", RV64C, c_immediate, (bit_matcher<u32>{0xE003, 0x2001}), translator_addi<true>, default_formatter), // overrides c.jal (RV32C)
    RV_QUICK_INSN_TR(RiscV, "c.srli", RV64C, c_immediate, (bit_matcher<u32>{0xFC03, 0x9001}), translator_srxi<false>, default_formatter), // overrides nse(c.srli) (RV32C)
    RV_QUICK_INSN(RiscV, "c.srai", RV64C, c_immediate, (bit_matcher<u32>{0xFC03, 0x9401}), functor_nyi<RiscV>, default_formatter), // overrides nse(c.srai) (RV32C)

    RV_QUICK_INSN(RiscV, "c.subw", RV64C, c_immediate, (bit_matcher<u32>{0xFC63, 0x9C01}), functor_nyi<RiscV>, default_formatter), // overrides reserved(c.subw) (RV32C)
    RV_QUICK_INSN_TR(RiscV, "c.addw", RV64C, c_immediate, (bit_matcher<u32>{0xFC63, 0x9C21}), translator_addw, default_formatter), // overrides reserved(c.addw) (RV32C)

    // quadrant 2
    RV_QUICK_INSN_TR(RiscV, "c.slli", RV64C, c_immediate, (bit_matcher<u32>{0xF003, 0x1002}), translator_slli, default_formatter), // overrides nse(c.slli) (RV32C)
    RV_QUICK_INSN(RiscV, "reserved(c.ldsp)", RV32C, c_immediate, (bit_matcher<u32>{0xEF83, 0x6002}), functor_reserved<RiscV>, default_formatter), // overrides c.flwsp (RV64C, RV128C); RES, rd=0
    RV_QUICK_INSN_TR(RiscV, "c.ldsp", RV32C, c_immediate, (bit_matcher<u32>{0xE003, 0x6002}), translator_ldsp, default_formatter), // overrides c.flwsp (RV32C)
    RV_QUICK_INSN_TR(RiscV, "c.sdsp", RV32C, c_immediate, (bit_matcher<u32>{0xE003, 0xE002}), translator_sdsp, default_formatter) // overrides c.fswsp (RV32C)
), is_rv32c<RiscV>);

template<typename RiscV>
inline constexpr auto is_rv128c = instruction_set(instruction_set(
    std::type_identity<RiscV> {},

    // quadrant 0
    RV_QUICK_INSN(RiscV, "c.lq", RV128C, c_immediate, (bit_matcher<u32>{0xE003, 0x2000}), functor_nyi<RiscV>, default_formatter), // overrides c.fld (RV32C, RV64C)
    RV_QUICK_INSN(RiscV, "c.sq", RV128C, c_immediate, (bit_matcher<u32>{0xE003, 0xA000}), functor_nyi<RiscV>, default_formatter), // overrides c.fsd (RV32C, RV64C)

    // quadrant 1
    RV_QUICK_INSN(RiscV, "c.srli64", RV128C, c_immediate, (bit_matcher<u32>{0xFC7D, 0x8001}), functor_nyi<RiscV>, default_formatter), // overrides hint(c.srli) (RV32C, RV64C)
    RV_QUICK_INSN(RiscV, "c.srai64", RV128C, c_immediate, (bit_matcher<u32>{0xFC7D, 0x8401}), functor_nyi<RiscV>, default_formatter), // overrides hint(c.srai) (RV32C, RV64C)

    // quadrant 2
    RV_QUICK_INSN(RiscV, "c.slli64", RV128C, c_immediate, (bit_matcher<u32>{0xF07F, 0x0002}), functor_nyi<RiscV>, default_formatter), // overrides hint(c.slli) (RV32C, RV64C)
    RV_QUICK_INSN(RiscV, "reserved(c.lqsp)", RV128C, c_immediate, (bit_matcher<u32>{0xEF83, 0x2002}), functor_reserved<RiscV>, default_formatter), // overrides c.fldsp (RV32C, RV64C); RES, rd=0
    RV_QUICK_INSN(RiscV, "c.lqsp", RV128C, c_immediate, (bit_matcher<u32>{0xE003, 0x2002}), functor_nyi<RiscV>, default_formatter), // overrides c.fldsp (RV32C, RV64C)
    RV_QUICK_INSN(RiscV, "c.sqsp", RV128C, c_immediate, (bit_matcher<u32>{0xE003, 0xA002}), functor_nyi<RiscV>, default_formatter) // overrides c.fsdsp (RV32C, RV64C)
), is_rv64c<RiscV>);

}
