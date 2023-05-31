#pragma once

namespace rv::detail {

#define RV_INSTRUCTION(_name, _standard, _format, _matcher, _formatter, ...)    \
    instruction_desc<                                                           \
      _name, instruction_standard::_standard, opcode_format::_format, _matcher, \
      decltype([]<typename Self>(Self & self, instruction_descriptor desc) -> typename Self::register_type __VA_ARGS__), _formatter>

enum class alu_action {
    add,
    sub,

    sll,
    srl,
    sra,

    slt,
    sltu,

    bxor,
    bor,
    band,

    mul,
    mulhuu,
    mulhsu,
    mulhss,
    div,
    divu,
    rem,
    remu,
};

template<alu_action Act, bool Imm = true, bool Word = false>
struct functor_alu {
    template<typename Self>
    constexpr auto operator()(Self& self, instruction_descriptor desc) -> typename Self::register_type {
        using register_type = typename Self::register_type;

        const auto op_1 = self.m_register_bank.read_register(desc.reg_src_1());
        auto op_2 = Imm ? desc.immediate<register_type>() : self.m_register_bank.read_register(desc.reg_src_2());

        if constexpr (Act == alu_action::sra || Act == alu_action::srl || Act == alu_action::sll) {
            if constexpr (std::is_same_v<typename Self::register_type, u32>) {
                op_2 &= 0x1Fu;
            } else {
                op_2 &= 0x3Fu;
            }
        }

        auto res = impl<register_type>(op_1, op_2);

        if constexpr (Word) {
            res = arith::sext<register_type, 32>((register_type)(u32)res);
        }

        self.m_register_bank.write_register(desc.reg_dst(), res);
        return self.m_next_step_sz;
    }

private:
    template<std::unsigned_integral T>
    static constexpr auto impl(T a, T b) -> T {
        switch (Act) {
            case alu_action::add: return a + b;
            case alu_action::sub: return a - b;
            case alu_action::sll: return a << b;
            case alu_action::srl: return a >> b;
            case alu_action::sra: return arith::arithmetic_shr(a, b);
            case alu_action::slt: return arith::signed_compare(a, b) == std::strong_ordering::less ? 1u : 0u;
            case alu_action::sltu: return a < b ? 1u : 0u;
            case alu_action::bxor: return a ^ b;
            case alu_action::bor: return a | b;
            case alu_action::band: return a & b;

            case alu_action::mul: return arith::multiply<T, false, false>(a, b).second;
            case alu_action::mulhuu: return arith::multiply<T, false, false>(a, b).first;
            case alu_action::mulhsu: return arith::multiply<T, true, false>(a, b).first;
            case alu_action::mulhss: return arith::multiply<T, true, true>(a, b).first;
            case alu_action::div: return (T)((std::make_signed_t<T>)a / (std::make_signed_t<T>)b);  // TODO
            case alu_action::divu: return a / b;
            case alu_action::rem: return (T)((std::make_signed_t<T>)a % (std::make_signed_t<T>)b);  // TODO
            case alu_action::remu: return a % b;
        }
    }
};

template<std::unsigned_integral StoreAs>
struct functor_store {
    template<typename Self>
    constexpr auto operator()(Self& self, instruction_descriptor desc) -> typename Self::register_type {
        const auto reg_src_1 = self.m_register_bank.read_register(desc.reg_src_1());
        const auto reg_src_2 = self.m_register_bank.read_register(desc.reg_src_2());
        self.m_memory.template write<StoreAs>(reg_src_1 + desc.store_offset<typename Self::register_type>(), (StoreAs)reg_src_2);
        return self.m_next_step_sz;
    }
};

template<std::unsigned_integral LoadAs>
struct functor_load {
    template<typename Self>
    constexpr auto operator()(Self& self, instruction_descriptor desc) -> typename Self::register_type {
        using register_type = typename Self::register_type;

        const auto reg_src_1 = self.m_register_bank.read_register(desc.reg_src_1());
        const auto immediate = desc.immediate<u64>();
        const auto addr = reg_src_1 + immediate;
        const auto res = arith::sext<register_type, sizeof(LoadAs) * 8>((register_type)self.m_memory.template read<LoadAs>(addr));
        self.m_register_bank.write_register(desc.reg_dst(), res);
        return self.m_next_step_sz;
    }
};

template<std::unsigned_integral LoadAs>
struct functor_load_unsigned {
    template<typename Self>
    constexpr auto operator()(Self& self, instruction_descriptor desc) -> typename Self::register_type {
        using register_type = typename Self::register_type;

        const auto reg_src_1 = self.m_register_bank.read_register(desc.reg_src_1());
        const auto immediate = desc.immediate<register_type>();
        self.m_register_bank.write_register(desc.reg_dst(), (register_type)self.m_memory.template read<u8>(reg_src_1 + immediate));
        return self.m_next_step_sz;
    }
};

template<typename Predicate>
struct functor_branch {
    template<typename Self>
    constexpr auto operator()(Self& self, instruction_descriptor desc) -> typename Self::register_type {
        using register_type = typename Self::register_type;

        const auto reg_src_1 = self.m_register_bank.read_register(desc.reg_src_1());
        const auto reg_src_2 = self.m_register_bank.read_register(desc.reg_src_2());

        if (std::invoke(Predicate{}, reg_src_1, reg_src_2)) {
            return desc.branch_offset<register_type>();
        }

        return self.m_next_step_sz;
    }
};

// clang-format off

using is_rv32i = instruction_set<
    RV_INSTRUCTION("lui", RV32I, upper_immediate, uimm_matcher<0b01101'11u>, default_formatter, {
        self.m_register_bank.write_register(desc.reg_dst(), desc.upper_immediate<u64>());
        return self.m_next_step_sz;
    }),

    RV_INSTRUCTION("auipc", RV32I, upper_immediate, uimm_matcher<0b00101'11u>, default_formatter, {
        self.m_register_bank.write_register(desc.reg_dst(), self.m_program_counter + desc.upper_immediate<u64>());
        return self.m_next_step_sz;
    }),

    RV_INSTRUCTION("jal", RV32I, jump, uimm_matcher<0b11011'11>, default_formatter, {
        bool is_compressed = (self.m_memory.template read<u32>(self.m_program_counter) & 0b11) != 0b11;
        self.m_register_bank.write_register(desc.reg_dst(), self.m_program_counter + self.m_next_step_sz);
        return desc.jump_offset<u64>();
    }),

    RV_INSTRUCTION("jalr", RV32I, immediate, decltype(imm_matcher<0b11001'11, 0b000>{}), default_formatter, {
        bool is_compressed = (self.m_memory.template read<u32>(self.m_program_counter) & 0b11) != 0b11;
        const auto temp = self.m_program_counter + self.m_next_step_sz;
        self.m_program_counter = (self.m_register_bank.read_register(desc.reg_src_1()) + desc.immediate<u64>()) & (~(u64)1);
        self.m_register_bank.write_register(desc.reg_dst(), temp);
        return 0uz;
    }),

    instruction_desc<"addi", instruction_standard::RV32I, opcode_format::immediate, alu_imm_matcher<0>, functor_alu<alu_action::add>>,
    instruction_desc<"slli", instruction_standard::RV32I, opcode_format::immediate, alu_imm_matcher<1>::combine_t<0xF800'0000, 0>, functor_alu<alu_action::sll>, imm_shift_formatter>,
    instruction_desc<"slti", instruction_standard::RV32I, opcode_format::immediate, alu_imm_matcher<2>, functor_alu<alu_action::slt>>,
    instruction_desc<"sltiu", instruction_standard::RV32I, opcode_format::immediate, alu_imm_matcher<3>, functor_alu<alu_action::sltu>>,
    instruction_desc<"xori", instruction_standard::RV32I, opcode_format::immediate, alu_imm_matcher<4>, functor_alu<alu_action::bxor>>,
    instruction_desc<"srli", instruction_standard::RV32I, opcode_format::immediate, alu_imm_matcher<5>::combine_t<0xF800'0000, 0>, functor_alu<alu_action::srl>, imm_shift_formatter>,
    instruction_desc<"srai", instruction_standard::RV32I, opcode_format::immediate, alu_imm_matcher<5>::combine_t<0xF800'0000, 0x4000'0000>, functor_alu<alu_action::sra>, imm_shift_formatter>,
    instruction_desc<"ori", instruction_standard::RV32I, opcode_format::immediate, alu_imm_matcher<6>, functor_alu<alu_action::bor>>,
    instruction_desc<"andi", instruction_standard::RV32I, opcode_format::immediate, alu_imm_matcher<7>, functor_alu<alu_action::band>>,

    instruction_desc<"add", instruction_standard::RV32I, opcode_format::reg_reg, alu_matcher<0>, functor_alu<alu_action::add, false>>,
    instruction_desc<"sub", instruction_standard::RV32I, opcode_format::reg_reg, alu_matcher<0, 0b01000'00>, functor_alu<alu_action::sub, false>>,
    instruction_desc<"sll", instruction_standard::RV32I, opcode_format::reg_reg, alu_matcher<1>, functor_alu<alu_action::sll, false>>,
    instruction_desc<"slt", instruction_standard::RV32I, opcode_format::reg_reg, alu_matcher<2>, functor_alu<alu_action::slt, false>>,
    instruction_desc<"sltu", instruction_standard::RV32I, opcode_format::reg_reg, alu_matcher<3>, functor_alu<alu_action::sltu, false>>,
    instruction_desc<"xor", instruction_standard::RV32I, opcode_format::reg_reg, alu_matcher<4>, functor_alu<alu_action::bxor, false>>,
    instruction_desc<"srl", instruction_standard::RV32I, opcode_format::reg_reg, alu_matcher<5>, functor_alu<alu_action::srl, false>>,
    instruction_desc<"sra", instruction_standard::RV32I, opcode_format::reg_reg, alu_matcher<5, 0b01000'00>, functor_alu<alu_action::sra, false>>,
    instruction_desc<"or", instruction_standard::RV32I, opcode_format::reg_reg, alu_matcher<6>, functor_alu<alu_action::bor, false>>,
    instruction_desc<"and", instruction_standard::RV32I, opcode_format::reg_reg, alu_matcher<7>, functor_alu<alu_action::band, false>>,

    instruction_desc<"fence", instruction_standard::RV32I, opcode_format::immediate, bit_matcher<u32, 0xF00F'FFFF, 0b00011'11>, functor_nop<4uz>, fence_formatter>,

    instruction_desc<"ecall", instruction_standard::RV32I, opcode_format::immediate, bit_matcher<u32, 0xFFFF'FFFF, 0b11100'11>, functor_nop<4uz>, mnemonic_only_formatter>,
    instruction_desc<"ebreak", instruction_standard::RV32I, opcode_format::immediate, bit_matcher<u32, 0xFFFF'FFFF, 0b11100'11 | (1 << 20)>, functor_nop<4uz>, mnemonic_only_formatter>,

    instruction_desc<"lb", instruction_standard::RV32I, opcode_format::immediate, imm_matcher<0b00000'11, 0b000>, functor_load<u8>, load_formatter>,
    instruction_desc<"lh", instruction_standard::RV32I, opcode_format::immediate, imm_matcher<0b00000'11, 0b001>, functor_load<u16>, load_formatter>,
    instruction_desc<"lw", instruction_standard::RV32I, opcode_format::immediate, imm_matcher<0b00000'11, 0b010>, functor_load<u32>, load_formatter>,
    instruction_desc<"lbu", instruction_standard::RV32I, opcode_format::immediate, imm_matcher<0b00000'11, 0b100>, functor_load_unsigned<u8>, load_formatter>,
    instruction_desc<"lhu", instruction_standard::RV32I, opcode_format::immediate, imm_matcher<0b00000'11, 0b101>, functor_load_unsigned<u16>, load_formatter>,

    instruction_desc<"sb", instruction_standard::RV32I, opcode_format::store, imm_matcher<0b01000'11, 0b000>, functor_store<u8>>,
    instruction_desc<"sh", instruction_standard::RV32I, opcode_format::store, imm_matcher<0b01000'11, 0b001>, functor_store<u16>>,
    instruction_desc<"sw", instruction_standard::RV32I, opcode_format::store, imm_matcher<0b01000'11, 0b010>, functor_store<u32>>,

    instruction_desc<"beq", instruction_standard::RV32I, opcode_format::branch, imm_matcher<0b11000'11, 0b000>, functor_branch<decltype([](auto a, auto b) { return a == b; })>>,
    instruction_desc<"bne", instruction_standard::RV32I, opcode_format::branch, imm_matcher<0b11000'11, 0b001>, functor_branch<decltype([](auto a, auto b) { return a != b; })>>,
    instruction_desc<"blt", instruction_standard::RV32I, opcode_format::branch, imm_matcher<0b11000'11, 0b100>, functor_branch<decltype([](auto a, auto b) { return arith::signed_compare(a, b) == std::strong_ordering::less; })>>,
    instruction_desc<"bge", instruction_standard::RV32I, opcode_format::branch, imm_matcher<0b11000'11, 0b101>, functor_branch<decltype([](auto a, auto b) { return arith::signed_compare(a, b) != std::strong_ordering::less; })>>,
    instruction_desc<"bltu", instruction_standard::RV32I, opcode_format::branch, imm_matcher<0b11000'11, 0b110>, functor_branch<decltype([](auto a, auto b) { return a < b; })>>,
    instruction_desc<"bgeu", instruction_standard::RV32I, opcode_format::branch, imm_matcher<0b11000'11, 0b111>, functor_branch<decltype([](auto a, auto b) { return a >= b; })>>
>;
// clang-format on

using is_rv32zifencei = instruction_set<
  instruction_desc<"fence.i", instruction_standard::Zifencei, opcode_format::immediate, bit_matcher<u32, 0xFFFF'FFFF, 0x0000'100F>, functor_nop<4uz>, mnemonic_only_formatter>>;

using is_rv32zicsr = instruction_set<
  instruction_desc<"csrrw", instruction_standard::Zicsr, opcode_format::immediate, imm_matcher<0b11100'11, 1>, functor_nop<4uz>>,
  instruction_desc<"csrrs", instruction_standard::Zicsr, opcode_format::immediate, imm_matcher<0b11100'11, 2>, functor_nop<4uz>>,
  instruction_desc<"csrrc", instruction_standard::Zicsr, opcode_format::immediate, imm_matcher<0b11100'11, 3>, functor_nop<4uz>>,
  instruction_desc<"csrrwi", instruction_standard::Zicsr, opcode_format::immediate, imm_matcher<0b11100'11, 5>, functor_nop<4uz>>,
  instruction_desc<"csrrsi", instruction_standard::Zicsr, opcode_format::immediate, imm_matcher<0b11100'11, 6>, functor_nop<4uz>>,
  instruction_desc<"csrrci", instruction_standard::Zicsr, opcode_format::immediate, imm_matcher<0b11100'11, 7>, functor_nop<4uz>>>;

}  // namespace rv::detail
