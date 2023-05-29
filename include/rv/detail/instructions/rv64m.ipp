#pragma once

namespace rv::detail {

// clang-format off
using is_rv32m = instruction_set<
  instruction_desc<"mul", instruction_standard::RV32M, opcode_format::reg_reg, alu_matcher<0, 0b00000'01>, functor_alu<alu_action::mul, false, false>>,
  instruction_desc<"mulh", instruction_standard::RV32M, opcode_format::reg_reg, alu_matcher<1, 0b00000'01>, functor_alu<alu_action::mulhss, false, false>>,
  instruction_desc<"mulhsu", instruction_standard::RV32M, opcode_format::reg_reg, alu_matcher<2, 0b00000'01>, functor_alu<alu_action::mulhsu, false, false>>,
  instruction_desc<"mulhu", instruction_standard::RV32M, opcode_format::reg_reg, alu_matcher<3, 0b00000'01>, functor_alu<alu_action::mulhuu, false, false>>,
  instruction_desc<"div", instruction_standard::RV32M, opcode_format::reg_reg, alu_matcher<4, 0b00000'01>, functor_alu<alu_action::div, false, false>>,
  instruction_desc<"divu", instruction_standard::RV32M, opcode_format::reg_reg, alu_matcher<5, 0b00000'01>, functor_alu<alu_action::divu, false, false>>,
  instruction_desc<"rem", instruction_standard::RV32M, opcode_format::reg_reg, alu_matcher<6, 0b00000'01>, functor_alu<alu_action::rem, false, false>>,
  instruction_desc<"remu", instruction_standard::RV32M, opcode_format::reg_reg, alu_matcher<7, 0b00000'01>, functor_alu<alu_action::remu, false, false>>
>;

using is_rv64m = instruction_set<
  instruction_desc<"mulw", instruction_standard::RV64M, opcode_format::reg_reg, aluw_matcher<0, 0b00000'01>, functor_alu<alu_action::mul, false, true>>,
  instruction_desc<"divw", instruction_standard::RV64M, opcode_format::reg_reg, aluw_matcher<4, 0b00000'01>, functor_alu<alu_action::div, false, true>>,
  instruction_desc<"divuw", instruction_standard::RV64M, opcode_format::reg_reg, aluw_matcher<5, 0b00000'01>, functor_alu<alu_action::divu, false, true>>,
  instruction_desc<"remw", instruction_standard::RV64M, opcode_format::reg_reg, aluw_matcher<6, 0b00000'01>, functor_alu<alu_action::rem, false, true>>,
  instruction_desc<"remuw", instruction_standard::RV64M, opcode_format::reg_reg, aluw_matcher<7, 0b00000'01>, functor_alu<alu_action::remu, false, true>>
>::combine_with<is_rv32m>;
// clang-format on

}
