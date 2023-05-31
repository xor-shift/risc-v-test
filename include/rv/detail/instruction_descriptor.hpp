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

template<std::unsigned_integral T, T CareAbout, T Want>
struct bit_matcher {
    using value_type = T;
    static constexpr auto care_about = CareAbout;
    static constexpr auto want = Want;

    template<T CareAboutOther, T WantOther>
    static constexpr auto mutually_exclusive = true;

    template<T CareAboutOther, T WantOther>
        requires mutually_exclusive<CareAboutOther, WantOther>
    using combine_t = bit_matcher<T, CareAbout | CareAboutOther, Want | WantOther>;

    template<typename Other>
        requires(std::is_same_v<typename Other::value_type, T>)
    using combine_with_t = combine_t<Other::care_about, Other::want>;

    static constexpr auto match(T v) -> bool {
        /*
         * !c || (v == w)
         * ~c | ~(v ^ w)
         * ~(c & (v ^ w))
         */

        return (care_about & (v ^ want)) == 0;
    }
};

static_assert(bit_matcher<u8, 0b1111'0000, 0b1010'1010>::match(0b1010'1100));
static_assert(bit_matcher<u8, 0b1100'0000, 0b1010'1010>::combine_t<0b0011'0000, 0b1010'1010>::match(0b1010'1100));
static_assert(!bit_matcher<u8, 0b1111'0000, 0b1010'1010>::match(0b1000'1100));

namespace detail {

template<typename T, typename U>
struct instruction_set_combiner;

}

template<typename... InstDefs>
struct instruction_set {
    using instruction_def_types = stf::bunch_of_types<InstDefs...>;

    template<typename Other>
    using combine_with = typename detail::instruction_set_combiner<instruction_set<InstDefs...>, Other>::type;

    template<typename Desc>
    static constexpr auto get_descriptor(u32 v) -> instruction_descriptor {
        return instruction_descriptor{
          .word = v,
          .mnemonic = Desc::mnemonic.c_str(),
          .standard = Desc::standard,
          .format = Desc::format,
        };
    }

    static constexpr auto try_parse(u32 v) -> std::optional<instruction_descriptor> {
        auto ret = std::optional<instruction_descriptor>{};
        find_description<0>(v, [v, &ret]<typename Desc>(std::type_identity<Desc>) { ret = get_descriptor<Desc>(v); });
        return ret;
    }

    template<typename OIt>
    static constexpr auto format_to(u32 v, OIt&& it) -> OIt&& {
        auto formatted = false;

        find_description<0>(v, [v, &formatted, &it]<typename Desc>(std::type_identity<Desc>) {
            formatted = true;

            using Formatter = typename Desc::formatter_type;

            it = Formatter::format_to(get_descriptor<Desc>(v), std::forward<OIt>(it));

            if constexpr (requires { Desc::functor_type::get_translation(v); }) {
                it = fmt::format_to(it, " -> ");
                return format_to(Desc::functor_type::get_translation(v), std::forward<OIt>(it));
            }
        });

        if (!formatted) {
            it = fmt::format_to(it, "unknown");
        }

        return std::forward<OIt>(it);
    }

    static constexpr auto format(u32 v) -> std::string {
        auto str = std::string{};
        format_to(v, std::back_inserter(str));
        return str;
    }

    template<typename Self>
    static constexpr auto try_step(Self& self) -> stf::expected<void, std::string_view>;

    template<typename Self>
    static constexpr auto try_execute(Self& self, u32 word, std::optional<typename Self::register_type> forced_step_sz = std::nullopt) -> stf::expected<void, std::string_view>;

private:
    template<usize N, typename Fn>
    static constexpr void find_description(u32 v, Fn&& fn) {
        using Desc = typename instruction_def_types::template nth_type<N>;
        if (Desc::match(v)) {
            std::invoke(std::forward<Fn>(fn), std::type_identity<Desc>{});
            return;
        }

        if constexpr (N + 1 != sizeof...(InstDefs)) {
            return find_description<N + 1, Fn>(v, std::forward<Fn>(fn));
        }
    }
};

namespace detail {

template<typename... InstDefs0, typename... InstDefs1>
struct instruction_set_combiner<instruction_set<InstDefs0...>, instruction_set<InstDefs1...>> : std::type_identity<instruction_set<InstDefs0..., InstDefs1...>> {};

namespace formatters {

struct default_formatter {
    template<typename OIt>
    static constexpr auto format_to(instruction_descriptor const& instruction, OIt&& it) -> OIt {
        it = fmt::format_to(it, "{}", instruction.mnemonic);

        const auto reg_name = true ? ::rv::register_name<true> : ::rv::register_name<false>;

        if (instruction.mnemonic == "unknown") {
            return it;
        }

        switch (instruction.format) {
            case ::rv::opcode_format::reg_reg:  //
                return fmt::format_to(it, " {}, {}, {}", reg_name(instruction.reg_dst()), reg_name(instruction.reg_src_1()), reg_name(instruction.reg_src_2()));
            case ::rv::opcode_format::immediate:  //
                return fmt::format_to(it, " {}, {}, {}", reg_name(instruction.reg_dst()), reg_name(instruction.reg_src_1()), static_cast<i32>(instruction.immediate()));
            case ::rv::opcode_format::upper_immediate:  //
                return fmt::format_to(it, " {}, {}", reg_name(instruction.reg_dst()), static_cast<i32>(rv::arith::sext<u32, 20>(instruction.upper_immediate() >> 12)));
            case ::rv::opcode_format::jump:  //
                return fmt::format_to(it, " {}, {}", reg_name(instruction.reg_dst()), static_cast<i32>(instruction.jump_offset()));
            case ::rv::opcode_format::store:  //
                return fmt::format_to(it, " {}, {}({})", reg_name(instruction.reg_src_2()), static_cast<i32>(instruction.store_offset()), reg_name(instruction.reg_src_1()));
            case ::rv::opcode_format::branch:  //
                return fmt::format_to(it, " {}, {}, {}", reg_name(instruction.reg_src_1()), reg_name(instruction.reg_src_2()), static_cast<i32>(instruction.branch_offset()));

            case ::rv::opcode_format::c_reg_reg: [[fallthrough]];
            case ::rv::opcode_format::c_immediate: [[fallthrough]];
            case ::rv::opcode_format::c_wide_immediate: [[fallthrough]];
            case ::rv::opcode_format::c_stack_rela_store: return it;
        }

        return it;
    }
};

struct mnemonic_only_formatter {
    template<typename OIt>
    static constexpr auto format_to(instruction_descriptor const& instruction, OIt&& it) -> OIt {
        const auto reg_name = true ? ::rv::register_name<true> : ::rv::register_name<false>;

        return it = fmt::format_to(it, "{}", instruction.mnemonic);
    }
};

struct imm_shift_formatter {
    template<typename OIt>
    static constexpr auto format_to(instruction_descriptor const& instruction, OIt&& it) -> OIt {
        const auto reg_name = true ? ::rv::register_name<true> : ::rv::register_name<false>;

        auto imm_to_print = static_cast<i32>(instruction.immediate());
        if (instruction.mnemonic == "srai" || instruction.mnemonic == "srli" || instruction.mnemonic == "slli") {
            imm_to_print &= 0b11'1111;
        } else if (instruction.mnemonic == "sraiw" || instruction.mnemonic == "srliw" || instruction.mnemonic == "slliw") {
            imm_to_print &= 0b1'1111;
        }

        return  //
          it = fmt::format_to(it, "{} {}, {}, {}", instruction.mnemonic, reg_name(instruction.reg_dst()), reg_name(instruction.reg_src_1()), imm_to_print);
    }
};

struct fence_formatter {
    template<typename OIt>
    static constexpr auto format_to(instruction_descriptor const& instruction, OIt&& it) -> OIt {
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

        return it = fmt::format_to(it, "{} {}, {}", instruction.mnemonic, iorw_lookup[pred], iorw_lookup[succ]);
    }
};

struct load_formatter {
    template<typename OIt>
    static constexpr auto format_to(instruction_descriptor const& instruction, OIt&& it) -> OIt {
        const auto reg_name = true ? ::rv::register_name<true> : ::rv::register_name<false>;

        return  //
          it = fmt::format_to(
            it, "{} {}, {}({})", instruction.mnemonic, reg_name(instruction.reg_dst()), static_cast<i32>(instruction.immediate()), reg_name(instruction.reg_src_1())
          );
    }
};

}  // namespace formatters

namespace matchers {

template<u32 Opcode>
    requires(Opcode < 0x80)
using opcode_matcher = bit_matcher<u32, 0x0000'007Fu, Opcode>;

template<u32 Funct3>
    requires(Funct3 < 8)
using funct_3_matcher = bit_matcher<u32, 0x0000'7000u, Funct3 << 12>;

template<u32 Funct7>
    requires(Funct7 < 0x80)
using funct_7_matcher = bit_matcher<u32, 0xFE00'0000u, Funct7 << 25>;

//

template<u32 Opcode>
using uimm_matcher = opcode_matcher<Opcode>;

template<u32 Opcode, u32 Funct3>
using imm_matcher = typename opcode_matcher<Opcode>::template combine_with_t<funct_3_matcher<Funct3>>;

template<u32 Action>
using alu_imm_matcher = imm_matcher<0b00100'11, Action>;

template<u32 Action, u32 Funct7 = 0>
    requires(Action < 8)
using alu_matcher = typename opcode_matcher<0b01100'11>::combine_with_t<funct_3_matcher<Action>>::template combine_with_t<funct_7_matcher<Funct7>>;

template<u32 Action>
using aluw_imm_matcher = imm_matcher<0b00110'11, Action>;

template<u32 Action, u32 Funct7 = 0>
    requires(Action < 8)
using aluw_matcher = typename opcode_matcher<0b01110'11>::combine_with_t<funct_3_matcher<Action>>::template combine_with_t<funct_7_matcher<Funct7>>;

}  // namespace matchers

using namespace formatters;
using namespace matchers;

template<usize StepSize = 4uz>
struct functor_nop {
    constexpr auto operator()(auto&, auto) -> usize { return StepSize; }
};

using functor_nyi = functor_nop<0uz>;

template<
  stf::string_literal Mnemonic,
  enum instruction_standard Standard,
  enum opcode_format Format,
  typename Matcher,
  typename Fn = functor_nyi,
  typename Formatter = default_formatter>
struct instruction_desc {
    using formatter_type = Formatter;
    using functor_type = Fn;
    static constexpr auto mnemonic = Mnemonic;
    static constexpr auto standard = Standard;
    static constexpr auto format = Format;

    static constexpr auto match(u32 instruction) -> bool { return Matcher::match(instruction); }
};

}  // namespace detail

}  // namespace rv

#include <rv/detail/instructions/rv32i.ipp>
#include <rv/detail/instructions/rv64i.ipp>
#include <rv/detail/instructions/rv64m.ipp>

#include <rv/detail/instructions/rv128c.ipp>

namespace rv {

namespace detail {

using is_rv32imc_zifencei_zicsr = is_rv32i::combine_with<is_rv32m>::combine_with<is_rv32c>::combine_with<is_rv32zifencei>::combine_with<is_rv32zicsr>;
using is_rv64imc_zifencei_zicsr = is_rv64i::combine_with<is_rv64m>::combine_with<is_rv64c>::combine_with<is_rv32zifencei>::combine_with<is_rv32zicsr>;

}  // namespace detail

using isa_type_32 = detail::is_rv32imc_zifencei_zicsr;
using isa_type_64 = detail::is_rv64imc_zifencei_zicsr;

template<typename... InstDefs>
template<typename Self>
constexpr auto instruction_set<InstDefs...>::try_step(Self& self) -> stf::expected<void, std::string_view> {
    // spdlog::trace("stepping into address {:#08X}", self.m_program_counter);
    const auto instruction_word = self.m_memory.template read<u32>(self.m_program_counter);
    if ((instruction_word & 0b11) == 0b11) {
        self.m_next_step_sz = 4;
    } else {
        self.m_next_step_sz = 2;
    }

    return try_execute(self, instruction_word);
}

template<typename... InstDefs>
template<typename Self>
constexpr auto instruction_set<InstDefs...>::try_execute(Self& self, u32 instruction_word, std::optional<typename Self::register_type> forced_step_sz)
  -> stf::expected<void, std::string_view> {
    auto ret = stf::expected<void, std::string_view>{};

    find_description<0>(instruction_word, [instruction_word, &forced_step_sz, &ret, &self]<typename Desc>(std::type_identity<Desc>) {
        const auto desc = get_descriptor<Desc>(instruction_word);

        constexpr bool is_translator = requires { Desc::functor_type::get_translation(instruction_word); };

        if constexpr (is_translator) {
            const auto translation = Desc::functor_type::get_translation(instruction_word);

            using Formatter = typename Desc::formatter_type;
            std::string formatted;
            Formatter::format_to(desc, std::back_inserter(formatted));

            spdlog::trace("@{:#010x}: {:#06x} ({}) translated into {:#010x}", self.m_program_counter, instruction_word & 0xFFFF, formatted, (u32)translation);
            return try_execute<Self>(self, translation/*, Desc::functor_type::step_sz(self, desc)*/);
        } else {
            spdlog::trace("@{:#010x}: executing {:#010x} ({})", self.m_program_counter, instruction_word, format(instruction_word));
            const auto req_step_sz = std::invoke(typename Desc::functor_type{}, self, desc);
            self.m_program_counter += forced_step_sz ? *forced_step_sz : req_step_sz;
        }
    });

    return ret;
}

}  // namespace rv
