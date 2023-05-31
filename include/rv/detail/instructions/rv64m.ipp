#pragma once

namespace rv::detail {

// clang-format off
template<typename RiscV>
inline constexpr auto is_rv32m = instruction_set(
    std::type_identity<RiscV> {},
    RV_QUICK_INSN(RiscV, "mul", RV32M, reg_reg, (alu_matcher<0, 0b00000'01>), (functor_alu<RiscV, alu_action::mul, false, false>), default_formatter),
    RV_QUICK_INSN(RiscV, "mulh", RV32M, reg_reg, (alu_matcher<1, 0b00000'01>), (functor_alu<RiscV, alu_action::mulhss, false, false>), default_formatter),
    RV_QUICK_INSN(RiscV, "mulhsu", RV32M, reg_reg, (alu_matcher<2, 0b00000'01>), (functor_alu<RiscV, alu_action::mulhsu, false, false>), default_formatter),
    RV_QUICK_INSN(RiscV, "mulhu", RV32M, reg_reg, (alu_matcher<3, 0b00000'01>), (functor_alu<RiscV, alu_action::mulhuu, false, false>), default_formatter),
    RV_QUICK_INSN(RiscV, "div", RV32M, reg_reg, (alu_matcher<4, 0b00000'01>), (functor_alu<RiscV, alu_action::div, false, false>), default_formatter),
    RV_QUICK_INSN(RiscV, "divu", RV32M, reg_reg, (alu_matcher<5, 0b00000'01>), (functor_alu<RiscV, alu_action::divu, false, false>), default_formatter),
    RV_QUICK_INSN(RiscV, "rem", RV32M, reg_reg, (alu_matcher<6, 0b00000'01>), (functor_alu<RiscV, alu_action::rem, false, false>), default_formatter),
    RV_QUICK_INSN(RiscV, "remu", RV32M, reg_reg, (alu_matcher<7, 0b00000'01>), (functor_alu<RiscV, alu_action::remu, false, false>), default_formatter)
);

template<typename RiscV>
inline constexpr auto is_rv64m = instruction_set(instruction_set(
    std::type_identity<RiscV> {},
    RV_QUICK_INSN(RiscV, "mulw", RV64M, reg_reg, (aluw_matcher<0, 0b00000'01>), (functor_alu<RiscV, alu_action::mul, false, true>), default_formatter),
    RV_QUICK_INSN(RiscV, "divw", RV64M, reg_reg, (aluw_matcher<4, 0b00000'01>), (functor_alu<RiscV, alu_action::div, false, true>), default_formatter),
    RV_QUICK_INSN(RiscV, "divuw", RV64M, reg_reg, (aluw_matcher<5, 0b00000'01>), (functor_alu<RiscV, alu_action::divu, false, true>), default_formatter),
    RV_QUICK_INSN(RiscV, "remw", RV64M, reg_reg, (aluw_matcher<6, 0b00000'01>), (functor_alu<RiscV, alu_action::rem, false, true>), default_formatter),
    RV_QUICK_INSN(RiscV, "remuw", RV64M, reg_reg, (aluw_matcher<7, 0b00000'01>), (functor_alu<RiscV, alu_action::remu, false, true>), default_formatter)
), is_rv32m<RiscV>);
// clang-format on

}  // namespace rv::detail
