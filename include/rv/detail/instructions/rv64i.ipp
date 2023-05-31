#pragma once

namespace rv::detail {

// clang-format off
template<typename RiscV>
inline constexpr auto is_rv64i = instruction_set(instruction_set(
  std::type_identity<RiscV>{},
  RV_QUICK_INSN(RiscV, "ld", RV64I, immediate, (imm_matcher<0b00000'11, 0b011>), (functor_load<RiscV, u64>), load_formatter),
  RV_QUICK_INSN(RiscV, "lwu", RV64I, immediate, (imm_matcher<0b00000'11, 0b110>), (functor_load_unsigned<RiscV, u32>), load_formatter),
  RV_QUICK_INSN(RiscV, "ldu", RV128I, immediate, (imm_matcher<0b00000'11, 0b111>), (functor_load_unsigned<RiscV, u64>), load_formatter),
  RV_QUICK_INSN(RiscV, "sd", RV64I, store, (imm_matcher<0b01000'11, 0b011>), (functor_store<RiscV, u64>), default_formatter),

  RV_QUICK_INSN(RiscV, "addiw", RV64I, immediate, aluw_imm_matcher<0>, (functor_alu<RiscV, alu_action::add, true, true>), default_formatter),
  RV_QUICK_INSN(RiscV, "slliw", RV64I, immediate, (aluw_imm_matcher<1>.combine(0xFC00'0000, 0)), (functor_alu<RiscV, alu_action::sll, true, true>), imm_shift_formatter),
  RV_QUICK_INSN(RiscV, "srliw", RV64I, immediate, (aluw_imm_matcher<5>.combine(0xFC00'0000, 0)), (functor_alu<RiscV, alu_action::srl, true, true>), imm_shift_formatter),
  RV_QUICK_INSN(RiscV, "sraiw", RV64I, immediate, (aluw_imm_matcher<5>.combine(0xFC00'0000, 0x4000'0000)), (functor_alu<RiscV, alu_action::sra, true, true>), imm_shift_formatter),

  RV_QUICK_INSN(RiscV, "addw", RV64I, reg_reg, (aluw_matcher<0>), (functor_alu<RiscV, alu_action::add, false, true>), default_formatter),
  RV_QUICK_INSN(RiscV, "subw", RV64I, reg_reg, (aluw_matcher<0, 0b01000'00>), (functor_alu<RiscV, alu_action::sub, false, true>), default_formatter),
  RV_QUICK_INSN(RiscV, "sllw", RV64I, reg_reg, (aluw_matcher<1>), (functor_alu<RiscV, alu_action::sll, false, true>), default_formatter),
  RV_QUICK_INSN(RiscV, "srlw", RV64I, reg_reg, (aluw_matcher<5>), (functor_alu<RiscV, alu_action::srl, false, true>), default_formatter),
  RV_QUICK_INSN(RiscV, "sraw", RV64I, reg_reg, (aluw_matcher<5, 0b01000'00>), (functor_alu<RiscV, alu_action::sra, false, true>), default_formatter)
), is_rv32i<RiscV>);
// clang-format on

}  // namespace rv::detail
