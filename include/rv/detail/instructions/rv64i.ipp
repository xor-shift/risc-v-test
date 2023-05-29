#pragma once

namespace rv::detail {

// clang-format off
using is_rv64i = instruction_set<
  instruction_desc<"ld", instruction_standard::RV64I, opcode_format::immediate, imm_matcher<0b00000'11, 0b011>, functor_load<u64>, load_formatter>,
  instruction_desc<"lwu", instruction_standard::RV64I, opcode_format::immediate, imm_matcher<0b00000'11, 0b110>, functor_load_unsigned<u32>, load_formatter>,
  instruction_desc<"ldu", instruction_standard::RV128I, opcode_format::immediate, imm_matcher<0b00000'11, 0b111>, functor_load_unsigned<u64>, load_formatter>,
  instruction_desc<"sd", instruction_standard::RV64I, opcode_format::store, imm_matcher<0b01000'11, 0b011>, functor_store<u64>>,

  instruction_desc<"addiw", instruction_standard::RV64I, opcode_format::immediate, aluw_imm_matcher<0>, functor_alu<alu_action::add, true, true>>,
  instruction_desc<"slliw", instruction_standard::RV64I, opcode_format::immediate, aluw_imm_matcher<1>::combine_t<0xFC00'0000, 0>, functor_alu<alu_action::sll, true, true>, imm_shift_formatter>,
  instruction_desc<"srliw", instruction_standard::RV64I, opcode_format::immediate, aluw_imm_matcher<5>::combine_t<0xFC00'0000, 0>, functor_alu<alu_action::srl, true, true>, imm_shift_formatter>,
  instruction_desc<"sraiw", instruction_standard::RV64I, opcode_format::immediate, aluw_imm_matcher<5>::combine_t<0xFC00'0000, 0x4000'0000>, functor_alu<alu_action::sra, true, true>, imm_shift_formatter>,

  instruction_desc<"addw", instruction_standard::RV64I, opcode_format::reg_reg, aluw_matcher<0>, functor_alu<alu_action::add, false, true>>,
  instruction_desc<"subw", instruction_standard::RV64I, opcode_format::reg_reg, aluw_matcher<0, 0b01000'00>, functor_alu<alu_action::sub, false, true>>,
  instruction_desc<"sllw", instruction_standard::RV64I, opcode_format::reg_reg, aluw_matcher<1>, functor_alu<alu_action::sll, false, true>>,
  instruction_desc<"srlw", instruction_standard::RV64I, opcode_format::reg_reg, aluw_matcher<5>, functor_alu<alu_action::srl, false, true>>,
  instruction_desc<"sraw", instruction_standard::RV64I, opcode_format::reg_reg, aluw_matcher<5, 0b01000'00>, functor_alu<alu_action::sra, false, true>>
>::combine_with<is_rv32i>;
// clang-format on

}
