#pragma once

namespace rv::detail {

// clang-format off

template<typename RiscV>
inline constexpr auto is_rv32i = instruction_set(std::type_identity<RiscV> {},
    RV_QUICK_INSN_FN(RiscV, "lui", RV32I, upper_immediate, uimm_matcher<0b01101'11u>, default_formatter, {
        self.m_register_bank.write_register(desc.reg_dst(), desc.upper_immediate<u64>());
    }),

    RV_QUICK_INSN_FN(RiscV, "auipc", RV32I, upper_immediate, uimm_matcher<0b00101'11u>, default_formatter, {
        self.m_register_bank.write_register(desc.reg_dst(), self.m_program_counter + desc.upper_immediate<u64>());
    }),

    RV_QUICK_INSN_FN(RiscV, "jal", RV32I, jump, uimm_matcher<0b11011'11>, default_formatter, {
        bool is_compressed = (self.m_memory.template read<u32>(self.m_program_counter) & 0b11) != 0b11;
        self.m_register_bank.write_register(desc.reg_dst(), self.m_program_counter + self.m_next_step_sz);
        self.jump(desc.jump_offset<u64>());
    }),

    RV_QUICK_INSN_FN(RiscV, "jalr", RV32I, immediate, (imm_matcher<0b11001'11, 0b000>), default_formatter, {
        bool is_compressed = (self.m_memory.template read<u32>(self.m_program_counter) & 0b11) != 0b11;
        const auto temp = self.m_program_counter + self.m_next_step_sz;
        self.jump_to((self.m_register_bank.read_register(desc.reg_src_1()) + desc.immediate<u64>()) & (~(u64)1));
        self.m_register_bank.write_register(desc.reg_dst(), temp);
    }),

    RV_QUICK_INSN(RiscV, "addi", RV32I, immediate, alu_imm_matcher<0>, (functor_alu<RiscV, alu_action::add>), default_formatter),
    RV_QUICK_INSN(RiscV, "slli", RV32I, immediate, (alu_imm_matcher<1>.combine(0xF800'0000, 0)), (functor_alu<RiscV, alu_action::sll>), imm_shift_formatter),
    RV_QUICK_INSN(RiscV, "slti", RV32I, immediate, alu_imm_matcher<2>, (functor_alu<RiscV, alu_action::slt>), default_formatter),
    RV_QUICK_INSN(RiscV, "sltiu", RV32I, immediate, alu_imm_matcher<3>, (functor_alu<RiscV, alu_action::sltu>), default_formatter),
    RV_QUICK_INSN(RiscV, "xori", RV32I, immediate, alu_imm_matcher<4>, (functor_alu<RiscV, alu_action::bxor>), default_formatter),
    RV_QUICK_INSN(RiscV, "srli", RV32I, immediate, (alu_imm_matcher<5>.combine(0xF800'0000, 0)), (functor_alu<RiscV, alu_action::srl>), imm_shift_formatter),
    RV_QUICK_INSN(RiscV, "srai", RV32I, immediate, (alu_imm_matcher<5>.combine(0xF800'0000, 0x4000'0000)), (functor_alu<RiscV, alu_action::sra>), imm_shift_formatter),
    RV_QUICK_INSN(RiscV, "ori", RV32I, immediate, alu_imm_matcher<6>, (functor_alu<RiscV, alu_action::bor>), default_formatter),
    RV_QUICK_INSN(RiscV, "andi", RV32I, immediate, alu_imm_matcher<7>, (functor_alu<RiscV, alu_action::band>), default_formatter),

    RV_QUICK_INSN(RiscV, "add", RV32I, reg_reg, alu_matcher<0>, (functor_alu<RiscV, alu_action::add, false>), default_formatter),
    RV_QUICK_INSN(RiscV, "sub", RV32I, reg_reg, (alu_matcher<0, 0b01000'00>), (functor_alu<RiscV, alu_action::sub, false>), default_formatter),
    RV_QUICK_INSN(RiscV, "sll", RV32I, reg_reg, alu_matcher<1>, (functor_alu<RiscV, alu_action::sll, false>), default_formatter),
    RV_QUICK_INSN(RiscV, "slt", RV32I, reg_reg, alu_matcher<2>, (functor_alu<RiscV, alu_action::slt, false>), default_formatter),
    RV_QUICK_INSN(RiscV, "sltu", RV32I, reg_reg, alu_matcher<3>, (functor_alu<RiscV, alu_action::sltu, false>), default_formatter),
    RV_QUICK_INSN(RiscV, "xor", RV32I, reg_reg, alu_matcher<4>, (functor_alu<RiscV, alu_action::bxor, false>), default_formatter),
    RV_QUICK_INSN(RiscV, "srl", RV32I, reg_reg, alu_matcher<5>, (functor_alu<RiscV, alu_action::srl, false>), default_formatter),
    RV_QUICK_INSN(RiscV, "sra", RV32I, reg_reg, (alu_matcher<5, 0b01000'00>), (functor_alu<RiscV, alu_action::sra, false>), default_formatter),
    RV_QUICK_INSN(RiscV, "or", RV32I, reg_reg, alu_matcher<6>, (functor_alu<RiscV, alu_action::bor, false>), default_formatter),
    RV_QUICK_INSN(RiscV, "and", RV32I, reg_reg, alu_matcher<7>, (functor_alu<RiscV, alu_action::band, false>), default_formatter),

    RV_QUICK_INSN(RiscV, "fence", RV32I, immediate, (bit_matcher<u32>{0xF00F'FFFF, 0b00011'11}), functor_nop<RiscV>, fence_formatter),

    RV_QUICK_INSN(RiscV, "ecall", RV32I, immediate, (bit_matcher<u32>{0xFFFF'FFFF, 0b11100'11}), functor_nop<RiscV>, mnemonic_only_formatter),
    RV_QUICK_INSN(RiscV, "ebreak", RV32I, immediate, (bit_matcher<u32>{0xFFFF'FFFF, 0b11100'11 | (1 << 20)}), functor_nop<RiscV>, mnemonic_only_formatter),

    RV_QUICK_INSN(RiscV, "lb", RV32I, immediate, (imm_matcher<0b00000'11, 0b000>), (functor_load<RiscV, u8>), load_formatter),
    RV_QUICK_INSN(RiscV, "lh", RV32I, immediate, (imm_matcher<0b00000'11, 0b001>), (functor_load<RiscV, u16>), load_formatter),
    RV_QUICK_INSN(RiscV, "lw", RV32I, immediate, (imm_matcher<0b00000'11, 0b010>), (functor_load<RiscV, u32>), load_formatter),
    RV_QUICK_INSN(RiscV, "lbu", RV32I, immediate, (imm_matcher<0b00000'11, 0b100>), (functor_load_unsigned<RiscV, u8>), load_formatter),
    RV_QUICK_INSN(RiscV, "lhu", RV32I, immediate, (imm_matcher<0b00000'11, 0b101>), (functor_load_unsigned<RiscV, u16>), load_formatter),

    RV_QUICK_INSN(RiscV, "sb", RV32I, store, (imm_matcher<0b01000'11, 0b000>), (functor_store<RiscV, u8>), default_formatter),
    RV_QUICK_INSN(RiscV, "sh", RV32I, store, (imm_matcher<0b01000'11, 0b001>), (functor_store<RiscV, u16>), default_formatter),
    RV_QUICK_INSN(RiscV, "sw", RV32I, store, (imm_matcher<0b01000'11, 0b010>), (functor_store<RiscV, u32>), default_formatter),

    RV_QUICK_INSN(RiscV, "beq", RV32I, branch, (imm_matcher<0b11000'11, 0b000>), (functor_branch<RiscV, decltype([](auto a, auto b) { return a == b; })>), default_formatter),
    RV_QUICK_INSN(RiscV, "bne", RV32I, branch, (imm_matcher<0b11000'11, 0b001>), (functor_branch<RiscV, decltype([](auto a, auto b) { return a != b; })>), default_formatter),
    RV_QUICK_INSN(RiscV, "blt", RV32I, branch, (imm_matcher<0b11000'11, 0b100>), (functor_branch<RiscV, decltype([](auto a, auto b) { return arith::signed_compare(a, b) == std::strong_ordering::less; })>), default_formatter),
    RV_QUICK_INSN(RiscV, "bge", RV32I, branch, (imm_matcher<0b11000'11, 0b101>), (functor_branch<RiscV, decltype([](auto a, auto b) { return arith::signed_compare(a, b) != std::strong_ordering::less; })>), default_formatter),
    RV_QUICK_INSN(RiscV, "bltu", RV32I, branch, (imm_matcher<0b11000'11, 0b110>), (functor_branch<RiscV, decltype([](auto a, auto b) { return a < b; })>), default_formatter),
    RV_QUICK_INSN(RiscV, "bgeu", RV32I, branch, (imm_matcher<0b11000'11, 0b111>), (functor_branch<RiscV, decltype([](auto a, auto b) { return a >= b; })>), default_formatter)
);
// clang-format on

template<typename RiscV>
inline constexpr auto is_rv32zifencei = instruction_set(
  std::type_identity<RiscV>{},
  RV_QUICK_INSN(RiscV, "fence.i", Zifencei, immediate, (bit_matcher<u32>{0xFFFF'FFFF, 0x0000'100F}), functor_nop<RiscV>, mnemonic_only_formatter)
);

template<typename RiscV>
inline constexpr auto is_rv32zicsr = instruction_set(
  std::type_identity<RiscV>{},
  RV_QUICK_INSN(RiscV, "csrrw", Zicsr, immediate, (imm_matcher<0b11100'11, 1>), functor_nop<RiscV>, mnemonic_only_formatter),
  RV_QUICK_INSN(RiscV, "csrrs", Zicsr, immediate, (imm_matcher<0b11100'11, 2>), functor_nop<RiscV>, mnemonic_only_formatter),
  RV_QUICK_INSN(RiscV, "csrrc", Zicsr, immediate, (imm_matcher<0b11100'11, 3>), functor_nop<RiscV>, mnemonic_only_formatter),
  RV_QUICK_INSN(RiscV, "csrrwi", Zicsr, immediate, (imm_matcher<0b11100'11, 5>), functor_nop<RiscV>, mnemonic_only_formatter),
  RV_QUICK_INSN(RiscV, "csrrsi", Zicsr, immediate, (imm_matcher<0b11100'11, 6>), functor_nop<RiscV>, mnemonic_only_formatter),
  RV_QUICK_INSN(RiscV, "csrrci", Zicsr, immediate, (imm_matcher<0b11100'11, 7>), functor_nop<RiscV>, mnemonic_only_formatter)
);

}  // namespace rv::detail
