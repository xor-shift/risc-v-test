#pragma once

#include <rv/detail/arith.hpp>
#include <rv/detail/definitions.hpp>

#include <stuff/expected.hpp>

#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace rv {

struct instruction_descriptor {
    u32 word;
    std::string_view mnemonic;
    enum instruction_standard standard;
    enum opcode_format format;

    constexpr auto is_compressed() -> bool { return (word & 0b11) != 0b11; }

    // the destination register for all instruction formats
    constexpr auto reg_dst() const -> reg { return static_cast<reg>((word >> 7u) & 0b1'1111u); }

    constexpr auto reg_dst_prime() const -> reg { return static_cast<reg>(((word >> 2u) & 0b111u) + 8); }

    constexpr auto reg_src_1() const -> reg { return static_cast<reg>((word >> 15u) & 0b1'1111u); }

    constexpr auto reg_src_2() const -> reg { return static_cast<reg>((word >> 20u) & 0b1'1111u); }

    constexpr auto c_reg_src_2() const -> reg { return static_cast<reg>((word >> 2u) & 0b1'1111u); }

    template<std::unsigned_integral T = u32>
    constexpr auto immediate() const -> T {
        const u32 imm_11_0 = (word >> 20u) & 0xFFFu;
        return arith::sext<T, 12>((T)imm_11_0);
    }

    /// nzimm for compressed-immediate (CI) type instructions
    template<std::unsigned_integral T = u32>
    constexpr auto nz_immediate() const -> T {
        const u32 nzimm = ((word >> 2u) & 0b0001'1111u) | ((word >> 7u) & 0b0010'0000u);
        return arith::sext<T, 6>((T)nzimm);
    }

    /// nzuimm for compressed-wide-immediate (CIW) type instructions
    template<std::unsigned_integral T = u32>
    constexpr auto nzu_immediate() const -> T {
        const auto nzuimm = ((word >> 7u) & 0b00'0011'0000u) | ((word >> 1u) & 0b11'1100'0000u) | ((word >> 4u) & 0b00'0000'0100u) | ((word >> 2u) & 0b00'0000'0010u);
        return (T)nzuimm;
    }

    /// uimm for compressed-stack-relative-store (CSS) type instructions
    template<std::unsigned_integral T = u32>
    constexpr auto cu_immediate() const -> T {
        const auto uimm = ((word >> 7u) & 0b11'1000) | ((word >> 1u) & 0b1'1100'0000);
        return (T)uimm;
    }

    template<std::unsigned_integral T = u32>
    constexpr auto upper_immediate() const -> T {
        const u32 imm_31_12 = (word >> 12u) & 0xF'FFFFu;
        return arith::sext<T, 32>((T)(imm_31_12 << 12));
    }

    template<std::unsigned_integral T = u32>
    constexpr auto store_offset() const -> T {
        const u32 funct_7 = (word >> 25u) & 0b111'1111u;
        const u32 reg_dst = (word >> 7u) & 0b1'1111u;
        return arith::sext<T, 12>((T)((funct_7 << 5) | reg_dst));
    }

    template<std::unsigned_integral T = u32>
    constexpr auto jump_offset() const -> T {
        const u32 imm_31_12 = (word >> 12u) & 0xF'FFFFu;
        return arith::sext<T, 21>(
          T((0u                                                   //
             | (imm_31_12 & 0b1000'0000'0000'0000'0000u)          //
             | ((imm_31_12 & 0b0000'0000'0001'0000'0000u) << 2u)  //
             | ((imm_31_12 & 0b0111'1111'1110'0000'0000u) >> 9u)  //
             | ((imm_31_12 & 0b0000'0000'0000'1111'1111u) << 11u))
            << 1u)
        );
    }

    template<std::unsigned_integral T = u32>
    constexpr auto branch_offset() const -> T {
        return arith::sext<T, 13>(
          T(0u                                       //
            | (word & 0x8000'0000u) >> (31u - 12u)   //
            | ((word & 0x7E00'0000u) >> (25u - 5u))  //
            | ((word & 0x0000'0F00u) >> (8u - 1u))   //
            | ((word & 0x0000'0080u) << (11u - 7u)))
        );
        ;
    }

    template<std::unsigned_integral T = u32, T ShiftBits = 5>
    constexpr auto shift_amt() const -> T {
        return (word >> 20) & (((T)1 << ShiftBits) - 1);
    }

    template<std::unsigned_integral T = u32>
    constexpr auto shift_type() const -> T {
        return word >> 27;
    }
};

template<std::unsigned_integral T>
struct bit_matcher {
    using value_type = T;
    T care_about;
    T want;

    constexpr auto combine(T other_care_about, T other_want) const -> bit_matcher {
        return {
          .care_about = static_cast<T>(care_about | other_care_about),
          .want = static_cast<T>(want | other_want),
        };
    }

    constexpr auto combine_with(bit_matcher const& other) const -> bit_matcher { return combine(other.care_about, other.want); }

    constexpr auto match(T v) const -> bool {
        /*
         * !c || (v == w)
         * ~c | ~(v ^ w)
         * ~(c & (v ^ w))
         */

        return (care_about & (v ^ want)) == 0;
    }
};

static_assert(bit_matcher<u8>{0b1111'0000, 0b1010'1010}.match(0b1010'1100));
static_assert(bit_matcher<u8>{0b1100'0000, 0b1010'1010}.combine(0b0011'0000, 0b1010'1010).match(0b1010'1100));
static_assert(!bit_matcher<u8>{0b1111'0000, 0b1010'1010}.match(0b1000'1100));

template<typename RiscV>
struct instruction_properties {
    using processor_type = RiscV;
    using register_type = typename processor_type::register_type;

    using executor_type = void (*)(processor_type&, instruction_descriptor);
    using translator_type = u32 (*)(u32);
    using formatter_type = std::string (*)(instruction_descriptor, bool);

    std::string_view mnemonic;
    instruction_standard standard;
    opcode_format format;
    bit_matcher<u32> matcher;

    executor_type executor;
    translator_type translator;
    formatter_type formatter;
};

#define RV_QUICK_INSN(_rv, _mnemonic, _standard, _format, _matcher, _functor, _formatter)                                                           \
    rv::instruction_properties<_rv> {                                                                                                               \
        .mnemonic = _mnemonic, .standard = rv::instruction_standard::_standard, .format = rv::opcode_format::_format, .matcher = _matcher,          \
        .executor = [](_rv& self, instruction_descriptor desc) -> void { _functor{}(self, desc); }, .translator = nullptr, .formatter = _formatter, \
    }

#define RV_QUICK_INSN_TR(_rv, _mnemonic, _standard, _format, _matcher, _functor, _formatter)                                                                    \
    rv::instruction_properties<_rv> {                                                                                                                           \
        .mnemonic = _mnemonic, .standard = rv::instruction_standard::_standard, .format = rv::opcode_format::_format, .matcher = _matcher, .executor = nullptr, \
        .translator = [](u32 instruction_word) -> u32 { return _functor{}(instruction_word); }, .formatter = _formatter,                                        \
    }

#define RV_QUICK_INSN_FN(_rv, _mnemonic, _standard, _format, _matcher, _formatter, ...)                                                    \
    rv::instruction_properties<_rv> {                                                                                                      \
        .mnemonic = _mnemonic, .standard = rv::instruction_standard::_standard, .format = rv::opcode_format::_format, .matcher = _matcher, \
        .executor = [](_rv & self, instruction_descriptor desc) -> void __VA_ARGS__, .translator = nullptr, .formatter = _formatter,       \
    }

namespace detail {

template<u32 Opcode>
    requires(Opcode < 0x80)
inline constexpr bit_matcher<u32> opcode_matcher = bit_matcher<u32>{0x0000'007Fu, Opcode};

template<u32 Funct3>
    requires(Funct3 < 8)
inline constexpr bit_matcher<u32> funct_3_matcher = bit_matcher<u32>{0x0000'7000u, Funct3 << 12};

template<u32 Funct7>
    requires(Funct7 < 0x80)
inline constexpr bit_matcher<u32> funct_7_matcher = bit_matcher<u32>{0xFE00'0000u, Funct7 << 25};

//

template<u32 Opcode>
inline constexpr bit_matcher<u32> uimm_matcher = opcode_matcher<Opcode>;

template<u32 Opcode, u32 Funct3>
inline constexpr bit_matcher<u32> imm_matcher = opcode_matcher<Opcode>.combine_with(funct_3_matcher<Funct3>);

template<u32 Action>
inline constexpr bit_matcher<u32> alu_imm_matcher = imm_matcher<0b00100'11, Action>;

template<u32 Action, u32 Funct7 = 0>
    requires(Action < 8)
inline constexpr bit_matcher<u32> alu_matcher = opcode_matcher<0b01100'11>.combine_with(funct_3_matcher<Action>).combine_with(funct_7_matcher<Funct7>);

template<u32 Action>
inline constexpr bit_matcher<u32> aluw_imm_matcher = imm_matcher<0b00110'11, Action>;

template<u32 Action, u32 Funct7 = 0>
    requires(Action < 8)
inline constexpr bit_matcher<u32> aluw_matcher = opcode_matcher<0b01110'11>.combine_with(funct_3_matcher<Action>).combine_with(funct_7_matcher<Funct7>);

}  // namespace detail

namespace detail {

constexpr auto default_formatter(instruction_descriptor instruction, bool abi_registers = false) -> std::string {
    auto str = std::string{};
    auto it = std::back_inserter(str);
    it = fmt::format_to(it, "{}", instruction.mnemonic);

    const auto reg_name = abi_registers ? ::rv::register_name<true> : ::rv::register_name<false>;

    if (instruction.mnemonic == "unknown") {
        return str;
    }

    switch (instruction.format) {
        case ::rv::opcode_format::reg_reg:  //
            it = fmt::format_to(it, " {}, {}, {}", reg_name(instruction.reg_dst()), reg_name(instruction.reg_src_1()), reg_name(instruction.reg_src_2()));
            break;
        case ::rv::opcode_format::immediate:  //
            it = fmt::format_to(it, " {}, {}, {}", reg_name(instruction.reg_dst()), reg_name(instruction.reg_src_1()), static_cast<i32>(instruction.immediate()));
            break;
        case ::rv::opcode_format::upper_immediate:  //
            it = fmt::format_to(it, " {}, {}", reg_name(instruction.reg_dst()), static_cast<i32>(rv::arith::sext<u32, 20>(instruction.upper_immediate() >> 12)));
            break;
        case ::rv::opcode_format::jump:  //
            it = fmt::format_to(it, " {}, {}", reg_name(instruction.reg_dst()), static_cast<i32>(instruction.jump_offset()));
            break;
        case ::rv::opcode_format::store:  //
            it = fmt::format_to(it, " {}, {}({})", reg_name(instruction.reg_src_2()), static_cast<i32>(instruction.store_offset()), reg_name(instruction.reg_src_1()));
            break;
        case ::rv::opcode_format::branch:  //
            it = fmt::format_to(it, " {}, {}, {}", reg_name(instruction.reg_src_1()), reg_name(instruction.reg_src_2()), static_cast<i32>(instruction.branch_offset()));
            break;

        case ::rv::opcode_format::c_reg_reg: [[fallthrough]];
        case ::rv::opcode_format::c_immediate: [[fallthrough]];
        case ::rv::opcode_format::c_wide_immediate: [[fallthrough]];
        case ::rv::opcode_format::c_stack_rela_store: break;
    }

    return str;
}

constexpr auto mnemonic_only_formatter(instruction_descriptor instruction, bool abi_registers) -> std::string {
    auto str = std::string{};
    auto it = std::back_inserter(str);
    const auto reg_name = abi_registers ? ::rv::register_name<true> : ::rv::register_name<false>;

    it = fmt::format_to(it, "{}", instruction.mnemonic);

    return str;
}

constexpr auto imm_shift_formatter(instruction_descriptor instruction, bool abi_registers) -> std::string {
    auto str = std::string{};
    auto it = std::back_inserter(str);
    const auto reg_name = abi_registers ? ::rv::register_name<true> : ::rv::register_name<false>;

    auto imm_to_print = static_cast<i32>(instruction.immediate());
    if (instruction.mnemonic == "srai" || instruction.mnemonic == "srli" || instruction.mnemonic == "slli") {
        imm_to_print &= 0b11'1111;
    } else if (instruction.mnemonic == "sraiw" || instruction.mnemonic == "srliw" || instruction.mnemonic == "slliw") {
        imm_to_print &= 0b1'1111;
    }

    it = fmt::format_to(it, "{} {}, {}, {}", instruction.mnemonic, reg_name(instruction.reg_dst()), reg_name(instruction.reg_src_1()), imm_to_print);

    return str;
}

constexpr auto fence_formatter(instruction_descriptor instruction, bool abi_registers) -> std::string {
    auto str = std::string{};
    auto it = std::back_inserter(str);
    const u32 pred = (instruction.immediate() >> 4u) & 0xFu;
    const u32 succ = (instruction.immediate()) & 0xFu;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc99-designator"
    constexpr const char* iorw_lookup[16]{
      [0b0000] = "invalid(0)", [0b0001] = "w",   [0b0010] = "r",   [0b0011] = "rw",    //
      [0b0100] = "o",          [0b0101] = "ow",  [0b0110] = "or",  [0b0111] = "orw",   //
      [0b1000] = "i",          [0b1001] = "iw",  [0b1010] = "ir",  [0b1011] = "irw",   //
      [0b1100] = "io",         [0b1101] = "iow", [0b1110] = "ior", [0b1111] = "iorw",  //
    };
#pragma clang diagnostic pop

    it = fmt::format_to(it, "{} {}, {}", instruction.mnemonic, iorw_lookup[pred], iorw_lookup[succ]);

    return str;
}

constexpr auto load_formatter(instruction_descriptor instruction, bool abi_registers) -> std::string {
    auto str = std::string{};
    auto it = std::back_inserter(str);
    const auto reg_name = abi_registers ? ::rv::register_name<true> : ::rv::register_name<false>;

    it = fmt::format_to(it, "{} {}, {}({})", instruction.mnemonic, reg_name(instruction.reg_dst()), static_cast<i32>(instruction.immediate()), reg_name(instruction.reg_src_1()));

    return str;
}

}  // namespace detail

template<typename RiscV>
struct generic_instruction_set {
    virtual constexpr ~generic_instruction_set() = default;

    virtual constexpr auto operator[](size_t i) const -> instruction_properties<RiscV> const& = 0;
    virtual constexpr auto operator[](size_t i) -> instruction_properties<RiscV>& = 0;

    virtual constexpr auto match(u32 instruction_word) const -> std::optional<instruction_properties<RiscV>> = 0;
    virtual auto format(u32 instruction_word, bool abi_register_names = false) const -> std::string = 0;
    virtual constexpr auto get_descriptor_for(instruction_properties<RiscV> const& props, u32 instruction_word) -> instruction_descriptor = 0;

    virtual constexpr void try_step(RiscV& self) const = 0;
};

template<typename RiscV, usize NumInstructions>
struct instruction_set final : generic_instruction_set<RiscV> {
    constexpr instruction_set() = default;

    template<typename... Ts>
    explicit constexpr instruction_set(std::type_identity<RiscV>, Ts&&... instructions) {
        // std::copy(il.begin(), il.end(), m_instructions.begin());

        instruction_properties<RiscV> arr[] = {instructions...};
        std::copy(std::begin(arr), std::end(arr), m_instructions.begin());
    }

    template<usize LhsNumInsns, usize RhsNumInsns>
    explicit constexpr instruction_set(instruction_set<RiscV, LhsNumInsns> const& lhs, instruction_set<RiscV, RhsNumInsns> const& rhs) {
        std::copy_n(lhs.m_instructions.begin(), LhsNumInsns, m_instructions.begin());
        std::copy_n(rhs.m_instructions.begin(), RhsNumInsns, m_instructions.begin() + LhsNumInsns);
    }

    constexpr auto operator[](size_t i) const -> instruction_properties<RiscV> const& { return m_instructions[i]; }
    constexpr auto operator[](size_t i) -> instruction_properties<RiscV>& { return m_instructions[i]; }

    constexpr auto match(u32 instruction_word) const -> std::optional<instruction_properties<RiscV>> {
        auto it = std::find_if(m_instructions.begin(), m_instructions.end(), [instruction_word](auto const& insn_prop) { return insn_prop.matcher.match(instruction_word); });

        if (it == m_instructions.end()) {
            return std::nullopt;
        }

        return *it;
    }

    auto format(u32 instruction_word, bool abi_register_names = false) const -> std::string {
        /*return match(instruction_word)  //
          .transform([instruction_word, abi_register_names](auto const& insn_prop) {
              return insn_prop.formatter(get_descriptor_for_impl(insn_prop, instruction_word), abi_register_names);
          })  //
          .value_or("unknown"s);*/

        const auto res = match(instruction_word);
        if (!res) {
            return "unknown";
        }

        const auto desc = get_descriptor_for_impl(*res, instruction_word);

        const auto formatted = (res->formatter)(desc, abi_register_names);

        if (res->translator) {
            const auto translated = (res->translator)(instruction_word);
            return fmt::format("{} -> {}", formatted, format(translated, abi_register_names));
        }

        return formatted;
    }

    static constexpr auto get_descriptor_for_impl(instruction_properties<RiscV> const& props, u32 instruction_word) -> instruction_descriptor {
        return {
          .word = instruction_word,
          .mnemonic = props.mnemonic,
          .standard = props.standard,
          .format = props.format,
        };
    }
    constexpr auto get_descriptor_for(instruction_properties<RiscV> const& props, u32 instruction_word) -> instruction_descriptor {
        return get_descriptor_for_impl(props, instruction_word);
    }

    constexpr void try_step(RiscV& self) const {
        // spdlog::trace("stepping into address {:#08X}", self.m_program_counter);
        const auto instruction_word = self.m_memory.template read<u32>(self.m_program_counter);
        if ((instruction_word & 0b11) == 0b11) {
            self.m_next_step_sz = 4;
        } else {
            self.m_next_step_sz = 2;
        }

        return try_execute(self, instruction_word);
    }

    constexpr void try_execute(RiscV& self, u32 instruction_word) const {
        const auto res = match(instruction_word);

        if (!res) {
            self.m_next_step_sz = 0;
            spdlog::warn("unknown instruction @ {:#010x}", self.m_program_counter);
            return;
        }

        const auto desc = get_descriptor_for_impl(*res, instruction_word);

        if (res->translator != nullptr) {
            const auto translation = (res->translator)(instruction_word);
            //const auto formatted = (res->formatter)(desc, true);
            //spdlog::trace("@{:#010x}: {:#06x} ({}) translated into {:#010x}", self.m_program_counter, instruction_word & 0xFFFF, formatted, (u32)translation);
            return try_execute(self, translation);
        }

        //spdlog::trace("@{:#010x}: executing {:#010x} ({})", self.m_program_counter, instruction_word, format(instruction_word, true));
        (res->executor)(self, desc);
        self.m_program_counter += self.m_next_step_sz;
    }

    std::array<instruction_properties<RiscV>, NumInstructions> m_instructions{};
};

template<typename RiscV, usize LhsNumInsns, usize RhsNumInsns>
instruction_set(instruction_set<RiscV, LhsNumInsns> const&, instruction_set<RiscV, RhsNumInsns> const&) -> instruction_set<RiscV, LhsNumInsns + RhsNumInsns>;

template<typename RiscV, typename... Ts>
instruction_set(std::type_identity<RiscV>, Ts&&...) -> instruction_set<RiscV, sizeof...(Ts)>;

template<typename Self>
struct functor_nop {
    constexpr auto operator()(Self& self, instruction_descriptor desc) {}
};

template<typename Self>
struct functor_nyi {
    constexpr auto operator()(Self& self, instruction_descriptor desc) { self.jump(0); }
};

template<typename Self, alu_action Act, bool Imm = true, bool Word = false>
struct functor_alu {
    constexpr void operator()(Self& self, instruction_descriptor desc) const {
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

template<typename Self, std::unsigned_integral StoreAs>
struct functor_store {
    constexpr auto operator()(Self& self, instruction_descriptor desc) {
        const auto reg_src_1 = self.m_register_bank.read_register(desc.reg_src_1());
        const auto reg_src_2 = self.m_register_bank.read_register(desc.reg_src_2());
        self.m_memory.template write<StoreAs>(reg_src_1 + desc.store_offset<typename Self::register_type>(), (StoreAs)reg_src_2);
    }
};

template<typename Self, std::unsigned_integral LoadAs>
struct functor_load {
    constexpr auto operator()(Self& self, instruction_descriptor desc) {
        using register_type = typename Self::register_type;

        const auto reg_src_1 = self.m_register_bank.read_register(desc.reg_src_1());
        const auto immediate = desc.immediate<u64>();
        const auto addr = reg_src_1 + immediate;
        const auto res = arith::sext<register_type, sizeof(LoadAs) * 8>((register_type)self.m_memory.template read<LoadAs>(addr));
        self.m_register_bank.write_register(desc.reg_dst(), res);
    }
};

template<typename Self, std::unsigned_integral LoadAs>
struct functor_load_unsigned {
    constexpr auto operator()(Self& self, instruction_descriptor desc) {
        using register_type = typename Self::register_type;

        const auto reg_src_1 = self.m_register_bank.read_register(desc.reg_src_1());
        const auto immediate = desc.immediate<register_type>();
        self.m_register_bank.write_register(desc.reg_dst(), (register_type)self.m_memory.template read<u8>(reg_src_1 + immediate));
    }
};

template<typename Self, typename Predicate>
struct functor_branch {
    constexpr auto operator()(Self& self, instruction_descriptor desc) {
        using register_type = typename Self::register_type;

        const auto reg_src_1 = self.m_register_bank.read_register(desc.reg_src_1());
        const auto reg_src_2 = self.m_register_bank.read_register(desc.reg_src_2());

        if (std::invoke(Predicate{}, reg_src_1, reg_src_2)) {
            self.jump(desc.branch_offset<register_type>());
        }
    }
};

}  // namespace rv

#include <rv/detail/instructions/rv32i.ipp>
#include <rv/detail/instructions/rv64i.ipp>
#include <rv/detail/instructions/rv64m.ipp>

#include <rv/detail/instructions/rv128c.ipp>

namespace rv {

template<typename RiscV>
inline constexpr auto is_rv32i =
  instruction_set(instruction_set(instruction_set(instruction_set(detail::is_rv32i<RiscV>, detail::is_rv32m<RiscV>), detail::is_rv32zifencei<RiscV>), detail::is_rv32zicsr<RiscV>), detail::is_rv32c<RiscV>);

template<typename RiscV>
inline constexpr auto is_rv64i =
  instruction_set(instruction_set(instruction_set(instruction_set(detail::is_rv64i<RiscV>, detail::is_rv64m<RiscV>), detail::is_rv32zifencei<RiscV>), detail::is_rv32zicsr<RiscV>), detail::is_rv64c<RiscV>);

}  // namespace rv
