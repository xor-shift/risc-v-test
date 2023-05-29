#pragma once

#include <stuff/bit.hpp>

#include <charconv>

namespace rv::detail {

using functor_hint = functor_nop<2uz>;
using functor_reserved = functor_nyi;
using functor_nse = functor_nyi;

namespace assembler {

constexpr auto asm_immediate(u32 imm_11_0, reg rs_1, u32 funct_3, reg rd, u32 opcode) {
    return (imm_11_0 << 20) | (static_cast<u32>(rs_1) << 15) | (funct_3 << 12) | (static_cast<u32>(rd) << 7) | opcode;
}

template<detail::alu_action Action, bool Word = false>
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

template<detail::alu_action Action, bool Word = false>
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
    static constexpr auto get_translation(u32 compressed_word) -> u32 {
        const auto rd = static_cast<reg>(stf::bit::extract<u32, 4, "2:0">(compressed_word) + 8u);
        const auto nz_uimm = stf::bit::extract<u32, 12, "5:4|9:6|2|3">(compressed_word);

        return assembler::alu_i<detail::alu_action::add, false>(rd, reg::x2, nz_uimm);
    }
};

struct translator_addi16sp {
    static constexpr auto get_translation(u32 compressed_word) -> u32 {
        const auto nz_imm = stf::bit::extract<u32, 12, "9|11~7|4|6|8:7|5">(compressed_word);

        return assembler::alu_i<detail::alu_action::add, false>(reg::x2, reg::x2, (i32)arith::sext<u32, 10>(nz_imm));
    }
};

struct translator_add {
    static constexpr auto get_translation(u32 compressed_word) -> u32 {
        const auto nz_rs_1_rd = static_cast<reg>(stf::bit::extract<u32, 11, "4:0">(compressed_word));
        const auto nz_rs_2 = static_cast<reg>(stf::bit::extract<u32, 6, "4:0">(compressed_word));

        return assembler::alu<detail::alu_action::add>(nz_rs_1_rd, nz_rs_1_rd, nz_rs_2);
    }
};

template<alu_action Action, bool Word = false>
    requires(                                                                                     //
      (Action == alu_action::bxor || Action == alu_action::bor || Action == alu_action::band) ||  //
      (Action == alu_action::sub || (Action == alu_action::add && Word))
    )
struct translator_bitwise {
    static constexpr auto get_translation(u32 compressed_word) -> u32 {
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
    static constexpr auto get_translation(u32 compressed_word) -> u32 {
        const auto nz_rs_1_rd = static_cast<reg>(stf::bit::extract<u32, 11, "4:0">(compressed_word));

        // nonzero iff. `Word`
        const auto nz_imm = stf::bit::extract<u32, 12, "5|11~7|4:0">(compressed_word);

        return assembler::alu_i<detail::alu_action::add, Word>(nz_rs_1_rd, nz_rs_1_rd, (i32)arith::sext<u32, 6>(nz_imm));
    }
};

struct translator_andi {
    static constexpr auto get_translation(u32 compressed_word) -> u32 {
        const auto nz_rs_1_rd = static_cast<reg>(stf::bit::extract<u32, 9, "2:0">(compressed_word) + 8u);
        const auto imm = stf::bit::extract<u32, 12, "5|11~7|4:0">(compressed_word);

        return assembler::alu_i<detail::alu_action::band>(nz_rs_1_rd, nz_rs_1_rd, (i32)arith::sext<u32, 6>(imm));
    }
};

template<bool Arithmetic>
struct translator_srxi {
    static constexpr auto get_translation(u32 compressed_word) -> u32 {
        const auto nz_rs_1_rd = static_cast<reg>(stf::bit::extract<u32, 9, "2:0">(compressed_word) + 8u);
        const auto nz_uimm = stf::bit::extract<u32, 12, "5|11~7|4:0">(compressed_word);

        return assembler::alu_i < Arithmetic ? detail::alu_action::sra : detail::alu_action::srl > (nz_rs_1_rd, nz_rs_1_rd, nz_uimm);
    }
};

struct translator_slli {
    static constexpr auto get_translation(u32 compressed_word) -> u32 {
        const auto nz_rs_1_rd = static_cast<reg>(stf::bit::extract<u32, 11, "4:0">(compressed_word));
        const auto nz_uimm = stf::bit::extract<u32, 12, "5|11~7|4:0">(compressed_word);

        return assembler::alu_i<detail::alu_action::sll>(nz_rs_1_rd, nz_rs_1_rd, nz_uimm);
    }
};

struct translator_sdsp {
    static constexpr auto get_translation(u32 compressed_word) -> u32 {
        const auto rs_2 = static_cast<reg>(stf::bit::extract<u32, 6, "4:0">(compressed_word));
        const auto uimm = stf::bit::extract<u32, 12, "5:3|8:6">(compressed_word);

        return assembler::store<assembler::ld_st_type::dword>(rs_2, uimm, reg::x2);
    }
};

struct translator_swsp {
    static constexpr auto get_translation(u32 compressed_word) -> u32 {
        const auto rs_2 = static_cast<reg>(stf::bit::extract<u32, 6, "4:0">(compressed_word));
        const auto uimm = stf::bit::extract<u32, 12, "5:2|7:6">(compressed_word);

        return assembler::store<assembler::ld_st_type::word>(rs_2, uimm, reg::x2);
    }
};

struct translator_sw {
    static constexpr auto get_translation(u32 compressed_word) -> u32 {
        const auto rs_1 = static_cast<reg>(stf::bit::extract<u32, 9, "2:0">(compressed_word) + 8u);
        const auto rs_2 = static_cast<reg>(stf::bit::extract<u32, 4, "2:0">(compressed_word) + 8u);
        const auto uimm = stf::bit::extract<u32, 12, "5:3|9~7|2|6">(compressed_word);

        return assembler::store<assembler::ld_st_type::word>(rs_2, uimm, rs_1);
    }
};

struct translator_sd {
    static constexpr auto get_translation(u32 compressed_word) -> u32 {
        const auto rs_1 = static_cast<reg>(stf::bit::extract<u32, 9, "2:0">(compressed_word) + 8u);
        const auto rs_2 = static_cast<reg>(stf::bit::extract<u32, 4, "2:0">(compressed_word) + 8u);
        const auto uimm = stf::bit::extract<u32, 12, "5:3|9~7|7:6">(compressed_word);

        return assembler::store<assembler::ld_st_type::dword>(rs_2, uimm, rs_1);
    }
};

struct translator_lui {
    static constexpr auto get_translation(u32 compressed_word) -> u32 {
        const auto nz_rd = static_cast<reg>(stf::bit::extract<u32, 11, "4:0">(compressed_word));
        const auto nz_imm = stf::bit::extract<u32, 12, "17|11~7|16:12">(compressed_word) >> 12;

        return assembler::lui(nz_rd, (i32)arith::sext<u32, 6>(nz_imm));
    }
};

struct translator_li {
    static constexpr auto get_translation(u32 compressed_word) -> u32 {
        const auto nz_rd = static_cast<reg>(stf::bit::extract<u32, 11, "4:0">(compressed_word));
        const auto imm = stf::bit::extract<u32, 12, "5|11~7|4:0">(compressed_word);

        return assembler::alu_i<detail::alu_action::add, false>(nz_rd, reg::x0, (i32)arith::sext<u32, 6>(imm));
    }
};

struct translator_addw {
    static constexpr auto get_translation(u32 compressed_word) -> u32 {
        const auto rs_1_rd = static_cast<reg>(stf::bit::extract<u32, 9, "2:0">(compressed_word) + 8u);
        const auto rs_2 = static_cast<reg>(stf::bit::extract<u32, 4, "2:0">(compressed_word) + 8u);

        return assembler::alu<detail::alu_action::add, true>(rs_1_rd, rs_1_rd, rs_2);
    }
};

struct translator_mv {
    static constexpr auto get_translation(u32 compressed_word) -> u32 {
        const auto nz_rd = static_cast<reg>(stf::bit::extract<u32, 11, "4:0">(compressed_word));
        const auto nz_rs_2 = static_cast<reg>(stf::bit::extract<u32, 6, "4:0">(compressed_word));

        return assembler::alu<detail::alu_action::add>(nz_rd, reg::x0, nz_rs_2);
    }
};

struct translator_j {
    static constexpr auto get_translation(u32 compressed_word) -> u32 {
        const auto imm = stf::bit::extract<u32, 12, "11|4|9:8|10|6|7|3:1|5">(compressed_word);

        return assembler::jal(reg::x0, (i32)arith::sext<u32, 12>(imm));
    }
};

struct translator_jr {
    static constexpr auto get_translation(u32 compressed_word) -> u32 {
        const auto nz_rs_1 = static_cast<reg>(stf::bit::extract<u32, 11, "4:0">(compressed_word));

        return assembler::jalr(reg::x0, nz_rs_1, 0);
    }
};

struct translator_jal {
    static constexpr auto get_translation(u32 compressed_word) -> u32 {
        const auto imm = stf::bit::extract<u32, 12, "11|4|9:8|10|6|7|3:1|5">(compressed_word);

        return assembler::jal(reg::x1, (i32)arith::sext<u32, 12>(imm));
    }
};

struct translator_jalr {
    static constexpr auto get_translation(u32 compressed_word) -> u32 {
        const auto rs_1 = static_cast<reg>(stf::bit::extract<u32, 11, "4:0">(compressed_word));

        return assembler::jalr(reg::x1, rs_1, 0);
    }
};

template<bool Equal>
struct translator_branch {
    static constexpr auto get_translation(u32 compressed_word) -> u32 {
        const auto rs_1 = static_cast<reg>(stf::bit::extract<u32, 9, "2:0">(compressed_word) + 8u);
        const auto imm = stf::bit::extract<u32, 12, "8|4:3|9~7|7:6|2:1|5">(compressed_word);

        constexpr auto branch_type = Equal ? assembler::branch_type::equal : assembler::branch_type::not_equal;

        return assembler::branch<branch_type>(rs_1, reg::x0, (i32)arith::sext<u32, 9>(imm));
    }
};

struct translator_ldsp {
    static constexpr auto get_translation(u32 compressed_word) -> u32 {
        const auto uimm = stf::bit::extract<u32, 12, "5|11~7|4:3|8:6">(compressed_word);
        const auto nz_rd = static_cast<reg>(stf::bit::extract<u32, 11, "4:0">(compressed_word));

        return assembler::load<assembler::ld_st_type::dword>(nz_rd, uimm, reg::x2);
    }
};

struct translator_lwsp {
    static constexpr auto get_translation(u32 compressed_word) -> u32 {
        const auto nz_rd = static_cast<reg>(stf::bit::extract<u32, 11, "4:0">(compressed_word));
        const auto uimm = stf::bit::extract<u32, 12, "5|11~7|4:2|7:6">(compressed_word);

        return assembler::load<assembler::ld_st_type::word>(nz_rd, uimm, reg::x2);
    }
};

struct translator_lw {
    static constexpr auto get_translation(u32 compressed_word) -> u32 {
        const auto rs_1 = static_cast<reg>(stf::bit::extract<u32, 9, "2:0">(compressed_word) + 8u);
        const auto rd = static_cast<reg>(stf::bit::extract<u32, 4, "2:0">(compressed_word) + 8u);
        const auto uimm = stf::bit::extract<u32, 12, "5:3|9~7|2|6">(compressed_word);

        return assembler::load<assembler::ld_st_type::word>(rd, uimm, rs_1);
    }
};

struct translator_ld {
    static constexpr auto get_translation(u32 compressed_word) -> u32 {
        const auto rs_1 = static_cast<reg>(stf::bit::extract<u32, 9, "2:0">(compressed_word) + 8u);
        const auto rd = static_cast<reg>(stf::bit::extract<u32, 4, "2:0">(compressed_word) + 8u);
        const auto uimm = stf::bit::extract<u32, 12, "5:3|9~7|7:6">(compressed_word);

        return assembler::load<assembler::ld_st_type::dword>(rd, uimm, rs_1);
    }
};

// clang-format off

using is_rv32c = instruction_set<
    // quadrant 0
    instruction_desc<"c.invalid", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xFFFF, 0x0000>, functor_nyi>,
    instruction_desc<"reserved(c.addi4spn)", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xFFE3, 0x0000>, functor_reserved>,
    instruction_desc<"c.addi4spn", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xE003, 0x0000>, translator_addi4spn>,
    instruction_desc<"c.fld", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xE003, 0x2000>, functor_nyi>, // overridden by c.lq (RV128C)
    instruction_desc<"c.lw", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xE003, 0x4000>, translator_lw>,
    instruction_desc<"c.flw", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xE003, 0x6000>, functor_nyi>, // overridden by c.ld (RV64C, RV128C)
    instruction_desc<"c.fsd", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xE003, 0xA000>, functor_nyi>, // overriden by c.sq (RV128)
    instruction_desc<"c.sw", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xE003, 0xC000>, translator_sw>,
    instruction_desc<"c.fsw", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xE003, 0xE000>, functor_nyi>, // overridden by c.sd (RV64C, RV128C)

    // quadrant 1
    instruction_desc<"c.nop", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xFFFF, 0x0001>, functor_nop<2uz>>,
    instruction_desc<"hint(c.nop)", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xEF83, 0x0001>, functor_hint>,
    instruction_desc<"hint(c.addi)", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xF07F, 0x0001>, functor_hint>,
    instruction_desc<"c.addi", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xE003, 0x0001>, translator_addi<false>>,
    instruction_desc<"c.jal", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xE003, 0x2001>, translator_jal>, // overridden by c.addiw (RV64C, RV128C)
    instruction_desc<"hint(c.li)", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xEF83, 0x4001>, functor_hint>,
    instruction_desc<"c.li", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xE003, 0x4001>, translator_li>,
    instruction_desc<"reserved(c.addi16sp)", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xFFFF, 0x6101>, functor_reserved>,
    instruction_desc<"c.addi16sp", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xEF83, 0x6101>, translator_addi16sp>,

    instruction_desc<"reserved(c.lui)", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xF07F, 0x6001>, functor_reserved>,
    instruction_desc<"hint(c.lui)", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xEF81, 0x6001>, functor_hint>,
    instruction_desc<"c.lui", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xE003, 0x6001>, translator_lui>,

    instruction_desc<"hint(c.srli)", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xFC7D, 0x8001>, functor_hint>, // overriden by c.srli64 (RV128C)
    instruction_desc<"nse(c.srli)", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xFC03, 0x9001>, functor_nse>, // overriden by c.srli (RV64C, RV128C)
    instruction_desc<"c.srli", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xEC03, 0x8001>, translator_srxi<false>>,

    instruction_desc<"hint(c.srai)", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xFC7D, 0x8401>, functor_hint>, // overriden by c.srai64 (RV128C)
    instruction_desc<"nse(c.srai)", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xFC03, 0x9401>, functor_nse>, // overriden by c.srai (RV64C, RV128C)
    instruction_desc<"c.srai", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xEC03, 0x8401>, translator_srxi<true>>,

    instruction_desc<"c.andi", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xEC03, 0x8801>, translator_andi>,
    instruction_desc<"c.sub", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xFC63, 0x8C01>, translator_bitwise<alu_action::sub, false>>,
    instruction_desc<"c.xor", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xFC63, 0x8C21>, translator_bitwise<alu_action::bxor>>,
    instruction_desc<"c.or", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xFC63, 0x8C41>, translator_bitwise<alu_action::bor>>,
    instruction_desc<"c.and", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xFC63, 0x8C61>, translator_bitwise<alu_action::band>>,

    instruction_desc<"reserved(c.subw)", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xFC63, 0x9C01>, functor_reserved>, // overriden by c.subw (RV64C, RV128C)
    instruction_desc<"reserved(c.addw)", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xFC63, 0x9C21>, functor_reserved>, // overriden by c.addw (RV64C, RV128C)
    instruction_desc<"reserved(c.aluw10)", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xFC63, 0x9C41>, functor_reserved>,
    instruction_desc<"reserved(c.aluw11)", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xFC63, 0x9C61>, functor_reserved>,

    instruction_desc<"c.j", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xE003, 0xA001>, translator_j>,
    instruction_desc<"c.beqz", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xE003, 0xC001>, translator_branch<true>>,
    instruction_desc<"c.bnez", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xE003, 0xE001>, translator_branch<false>>,

    // quadrant 2
    instruction_desc<"nse(c.slli)", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xF003, 0x1002>, functor_reserved>, // overridden by c.slli (RV64C, RV128C); NSE, nzuimm[5]=1
    instruction_desc<"hint(c.slli64)", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xF07F, 0x0002>, functor_hint>, // overridden by c.slli64 (RV128C); hint nzuimm=0
    instruction_desc<"hint(c.slli)", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xEF83, 0x0002>, functor_hint>, // hint rd=0
    instruction_desc<"c.slli", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xE003, 0x0002>, translator_slli>,

    instruction_desc<"c.fldsp", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xE003, 0x2002>, functor_nyi>, // overridden by c.lqsp (RV128C)

    instruction_desc<"reserved(c.lwsp)", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xEF83, 0x4002>, functor_reserved>, // RES, rd=0
    instruction_desc<"c.lwsp", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xE003, 0x4002>, translator_lwsp>,

    instruction_desc<"c.flwsp", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xE003, 0x6002>, functor_nyi>, // overridden by c.ldsp (RV64C, RV128C)

    instruction_desc<"reserved(c.jr)", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xFFFF, 0x8002>, functor_reserved>,
    instruction_desc<"c.jr", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xF07F, 0x8002>, translator_jr>,
    instruction_desc<"hint(c.mv)", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xFF83, 0x8002>, functor_hint>,
    instruction_desc<"c.mv", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xF003, 0x8002>, translator_mv>,

    instruction_desc<"c.ebreak", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xFFFF, 0x9002>, functor_nyi>,
    instruction_desc<"c.jalr", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xF07F, 0x9002>, translator_jalr>,
    instruction_desc<"hint(c.add)", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xFF83, 0x9002>, functor_hint>,
    instruction_desc<"c.add", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xF003, 0x9002>, translator_add>,

    instruction_desc<"c.fsdsp", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xE003, 0xA002>, functor_nyi>, // overridden by c.sqsp (RV128C)
    instruction_desc<"c.swsp", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xE003, 0xC002>, translator_swsp>,
    instruction_desc<"c.fswsp", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xE003, 0xE002>, functor_nyi> // overridden by c.sdsp (RV64C, RV128C)
>;

using is_rv64c = instruction_set<
    // quadrant 0
    instruction_desc<"c.ld", instruction_standard::RV64C, opcode_format::c_immediate, bit_matcher<u32, 0xE003, 0x6000>, translator_ld>, // overrides c.flw (RV32C)
    instruction_desc<"c.sd", instruction_standard::RV64C, opcode_format::c_immediate, bit_matcher<u32, 0xE003, 0xE000>, translator_sd>, // overrides c.fsw (RV32C)

    // quadrant 1
    instruction_desc<"reserved(c.addiw)", instruction_standard::RV64C, opcode_format::c_immediate, bit_matcher<u32, 0xEF83, 0x2001>, functor_reserved>, // overrides c.jal (RV32C)
    instruction_desc<"c.addiw", instruction_standard::RV64C, opcode_format::c_immediate, bit_matcher<u32, 0xE003, 0x2001>, translator_addi<true>>, // overrides c.jal (RV32C)
    instruction_desc<"c.srli", instruction_standard::RV64C, opcode_format::c_immediate, bit_matcher<u32, 0xFC03, 0x9001>, translator_srxi<false>>, // overrides nse(c.srli) (RV32C)
    instruction_desc<"c.srai", instruction_standard::RV64C, opcode_format::c_immediate, bit_matcher<u32, 0xFC03, 0x9401>, functor_nyi>, // overrides nse(c.srai) (RV32C)

    instruction_desc<"c.subw", instruction_standard::RV64C, opcode_format::c_immediate, bit_matcher<u32, 0xFC63, 0x9C01>, functor_nyi>, // overrides reserved(c.subw) (RV32C)
    instruction_desc<"c.addw", instruction_standard::RV64C, opcode_format::c_immediate, bit_matcher<u32, 0xFC63, 0x9C21>, translator_addw>, // overrides reserved(c.addw) (RV32C)

    // quadrant 2
    instruction_desc<"c.slli", instruction_standard::RV64C, opcode_format::c_immediate, bit_matcher<u32, 0xF003, 0x1002>, translator_slli>, // overrides nse(c.slli) (RV32C)
    instruction_desc<"reserved(c.ldsp)", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xEF83, 0x6002>, functor_reserved>, // overrides c.flwsp (RV64C, RV128C); RES, rd=0
    instruction_desc<"c.ldsp", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xE003, 0x6002>, translator_ldsp>, // overrides c.flwsp (RV32C)
    instruction_desc<"c.sdsp", instruction_standard::RV32C, opcode_format::c_immediate, bit_matcher<u32, 0xE003, 0xE002>, translator_sdsp> // overrides c.fswsp (RV32C)
>::combine_with<is_rv32c>;

using is_rv128c = instruction_set<
    // quadrant 0
    instruction_desc<"c.lq", instruction_standard::RV128C, opcode_format::c_immediate, bit_matcher<u32, 0xE003, 0x2000>, functor_nyi>, // overrides c.fld (RV32C, RV64C)
    instruction_desc<"c.sq", instruction_standard::RV128C, opcode_format::c_immediate, bit_matcher<u32, 0xE003, 0xA000>, functor_nyi>, // overrides c.fsd (RV32C, RV64C)

    // quadrant 1
    instruction_desc<"c.srli64", instruction_standard::RV128C, opcode_format::c_immediate, bit_matcher<u32, 0xFC7D, 0x8001>, functor_nyi>, // overrides hint(c.srli) (RV32C, RV64C)
    instruction_desc<"c.srai64", instruction_standard::RV128C, opcode_format::c_immediate, bit_matcher<u32, 0xFC7D, 0x8401>, functor_nyi>, // overrides hint(c.srai) (RV32C, RV64C)

    // quadrant 2
    instruction_desc<"c.slli64", instruction_standard::RV128C, opcode_format::c_immediate, bit_matcher<u32, 0xF07F, 0x0002>, functor_nyi>, // overrides hint(c.slli) (RV32C, RV64C)
    instruction_desc<"reserved(c.lqsp)", instruction_standard::RV128C, opcode_format::c_immediate, bit_matcher<u32, 0xEF83, 0x2002>, functor_reserved>, // overrides c.fldsp (RV32C, RV64C); RES, rd=0
    instruction_desc<"c.lqsp", instruction_standard::RV128C, opcode_format::c_immediate, bit_matcher<u32, 0xE003, 0x2002>, functor_nyi>, // overrides c.fldsp (RV32C, RV64C)
    instruction_desc<"c.sqsp", instruction_standard::RV128C, opcode_format::c_immediate, bit_matcher<u32, 0xE003, 0xA002>, functor_nyi> // overrides c.fsdsp (RV32C, RV64C)
>::combine_with<is_rv64c>;

}
