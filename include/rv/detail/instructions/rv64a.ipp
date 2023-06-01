#pragma once

namespace rv::detail {

template<usize Funct5>
inline constexpr auto amow_matcher = opcode_matcher<0b01011'11>.combine(0xF800'0000u, Funct5 << 27).combine_with(funct_3_matcher<0b010>);

template<usize Funct5>
inline constexpr auto amod_matcher = opcode_matcher<0b01011'11>.combine(0xF800'0000u, Funct5 << 27).combine_with(funct_3_matcher<0b011>);

enum class atomic_operation {
    swap,
    add,
    bxor,
    band,
    bor,
    min,
    max,
    minu,
    maxu,
};

template<typename Self, atomic_operation Op, bool DoubleWord>
struct functor_amo {
    constexpr void operator()(Self& self, instruction_descriptor desc) const {
        if constexpr (DoubleWord && std::is_same_v<typename Self::register_type, u32>) {
            self.jump(0);
            return;
        }

        const auto acquire = ((desc.word >> 26u) & 1u) != 0u;
        const auto release = ((desc.word >> 25u) & 1u) != 0u;

        using ld_st_type = std::conditional_t<DoubleWord, u64, u32>;

        const auto addr = self.m_register_bank.read_register(desc.reg_src_1());
        const auto ldval_raw = self.m_memory.template read<ld_st_type>(addr);
        const auto ldval = arith::sext<typename Self::register_type, sizeof(ld_st_type) * 8>(ldval_raw);

        const auto reg_src_2 = self.m_register_bank.read_register(desc.reg_src_2());

        const auto write_back = ([&] -> typename Self::register_type {
            switch (Op) {
                case atomic_operation::swap: return reg_src_2;
                case atomic_operation::add: return reg_src_2 + ldval;
                case atomic_operation::bxor: return reg_src_2 ^ ldval;
                case atomic_operation::band: return reg_src_2 & ldval;
                case atomic_operation::bor: return reg_src_2 | ldval;
                case atomic_operation::min: return arith::signed_compare(reg_src_2, ldval) == std::strong_ordering::less ? reg_src_2 : ldval;
                case atomic_operation::max: return arith::signed_compare(reg_src_2, ldval) != std::strong_ordering::less ? reg_src_2 : ldval;
                case atomic_operation::minu: return std::min(reg_src_2, ldval);
                case atomic_operation::maxu: return std::max(reg_src_2, ldval);
            }
        })();

        self.m_register_bank.write_register(desc.reg_dst(), ldval);
        self.m_memory.template write<ld_st_type>(addr, write_back);

        // self.jump(0);
    }
};

template<typename Self, bool DoubleWord>
struct functor_load_reserved {
    constexpr void operator()(Self& self, instruction_descriptor desc) const {
        using ld_st_type = std::conditional_t<DoubleWord, u64, u32>;

        const auto acquire = ((desc.word >> 26u) & 1u) != 0u;
        const auto release = ((desc.word >> 25u) & 1u) != 0u;

        const auto addr = self.m_register_bank.read_register(desc.reg_src_1());
        const auto ld_val_raw = self.m_memory.template load_reserved<ld_st_type>(addr);
        const auto ld_val = arith::sext<typename Self::register_type, sizeof(ld_st_type) * 8>(ld_val_raw);
        self.m_register_bank.write_register(desc.reg_dst(), ld_val);
    }
};

template<typename Self, bool DoubleWord>
struct functor_store_conditional {
    constexpr void operator()(Self& self, instruction_descriptor desc) const {
        using ld_st_type = std::conditional_t<DoubleWord, u64, u32>;

        const auto acquire = ((desc.word >> 26u) & 1u) != 0u;
        const auto release = ((desc.word >> 25u) & 1u) != 0u;

        const auto addr = self.m_register_bank.read_register(desc.reg_src_1());
        const auto st_val = self.m_register_bank.read_register(desc.reg_src_2());
        const auto success = self.m_memory.template store_conditional<ld_st_type>(addr, st_val);
        self.m_register_bank.write_register(desc.reg_dst(), success ? 0u : 1u);
    }
};

inline auto formatter_amo(instruction_descriptor desc, bool abi_registers = false) -> std::string {
    const auto reg_name = abi_registers ? ::rv::register_name<true> : ::rv::register_name<false>;

    const auto acquire = ((desc.word >> 26u) & 1u) != 0u;
    const auto release = ((desc.word >> 25u) & 1u) != 0u;

    return fmt::format(
      "{}{}{}{} {}, {}, ({})",        //
      desc.mnemonic,                  //
      acquire || release ? "." : "",  //
      acquire ? "aq" : "",            //
      release ? "rl" : "",            //
      reg_name(desc.reg_dst()),       //
      reg_name(desc.reg_src_2()),     //
      reg_name(desc.reg_src_1())      //
    );
}

// clang-format off
template<typename RiscV>
inline constexpr auto is_rv32a = instruction_set(
    std::type_identity<RiscV> {},
    RV_QUICK_INSN(RiscV, "lr.w",      RV32A, reg_reg, amow_matcher<0b00010>, (functor_load_reserved<RiscV, false>), default_formatter),
    RV_QUICK_INSN(RiscV, "sc.w",      RV32A, reg_reg, amow_matcher<0b00011>, (functor_store_conditional<RiscV, false>), default_formatter),
    RV_QUICK_INSN(RiscV, "amoswap.w", RV32A, reg_reg, amow_matcher<0b00001>, (functor_amo<RiscV, atomic_operation::swap, false>), formatter_amo),
    RV_QUICK_INSN(RiscV, "amoadd.w",  RV32A, reg_reg, amow_matcher<0b00000>, (functor_amo<RiscV, atomic_operation::add, false>), formatter_amo),
    RV_QUICK_INSN(RiscV, "amoxor.w",  RV32A, reg_reg, amow_matcher<0b00100>, (functor_amo<RiscV, atomic_operation::bxor, false>), formatter_amo),
    RV_QUICK_INSN(RiscV, "amoand.w",  RV32A, reg_reg, amow_matcher<0b01100>, (functor_amo<RiscV, atomic_operation::band, false>), formatter_amo),
    RV_QUICK_INSN(RiscV, "amoor.w",   RV32A, reg_reg, amow_matcher<0b01000>, (functor_amo<RiscV, atomic_operation::bor, false>), formatter_amo),
    RV_QUICK_INSN(RiscV, "amomin.w",  RV32A, reg_reg, amow_matcher<0b10000>, (functor_amo<RiscV, atomic_operation::min, false>), formatter_amo),
    RV_QUICK_INSN(RiscV, "amomax.w",  RV32A, reg_reg, amow_matcher<0b10100>, (functor_amo<RiscV, atomic_operation::max, false>), formatter_amo),
    RV_QUICK_INSN(RiscV, "amominu.w", RV32A, reg_reg, amow_matcher<0b11000>, (functor_amo<RiscV, atomic_operation::minu, false>), formatter_amo),
    RV_QUICK_INSN(RiscV, "amomaxu.w", RV32A, reg_reg, amow_matcher<0b11100>, (functor_amo<RiscV, atomic_operation::maxu, false>), formatter_amo)
);

template<typename RiscV>
inline constexpr auto is_rv64a = instruction_set(instruction_set(
    std::type_identity<RiscV> {},
    RV_QUICK_INSN(RiscV, "lr.d",      RV32A, reg_reg, amod_matcher<0b00010>, (functor_load_reserved<RiscV, true>), default_formatter),
    RV_QUICK_INSN(RiscV, "sc.d",      RV32A, reg_reg, amod_matcher<0b00011>, (functor_store_conditional<RiscV, true>), default_formatter),
    RV_QUICK_INSN(RiscV, "amoswap.d", RV32A, reg_reg, amod_matcher<0b00001>, (functor_amo<RiscV, atomic_operation::swap, true>), formatter_amo),
    RV_QUICK_INSN(RiscV, "amoadd.d",  RV32A, reg_reg, amod_matcher<0b00000>, (functor_amo<RiscV, atomic_operation::add, true>), formatter_amo),
    RV_QUICK_INSN(RiscV, "amoxor.d",  RV32A, reg_reg, amod_matcher<0b00100>, (functor_amo<RiscV, atomic_operation::bxor, true>), formatter_amo),
    RV_QUICK_INSN(RiscV, "amoand.d",  RV32A, reg_reg, amod_matcher<0b01100>, (functor_amo<RiscV, atomic_operation::band, true>), formatter_amo),
    RV_QUICK_INSN(RiscV, "amoor.d",   RV32A, reg_reg, amod_matcher<0b01000>, (functor_amo<RiscV, atomic_operation::bor, true>), formatter_amo),
    RV_QUICK_INSN(RiscV, "amomin.d",  RV32A, reg_reg, amod_matcher<0b10000>, (functor_amo<RiscV, atomic_operation::min, true>), formatter_amo),
    RV_QUICK_INSN(RiscV, "amomax.d",  RV32A, reg_reg, amod_matcher<0b10100>, (functor_amo<RiscV, atomic_operation::max, true>), formatter_amo),
    RV_QUICK_INSN(RiscV, "amominu.d", RV32A, reg_reg, amod_matcher<0b11000>, (functor_amo<RiscV, atomic_operation::minu, true>), formatter_amo),
    RV_QUICK_INSN(RiscV, "amomaxu.d", RV32A, reg_reg, amod_matcher<0b11100>, (functor_amo<RiscV, atomic_operation::maxu, true>), formatter_amo)
), is_rv32a<RiscV>);
// clang-format on

}  // namespace rv::detail
