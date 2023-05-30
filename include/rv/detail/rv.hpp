#pragma once

#include <rv/detail/memory.hpp>
#include <rv/detail/registers.hpp>

namespace rv {

struct infmt_ihex_tag {};
struct infmt_bin_tag {};
struct infmt_elf_tag {};

template<typename RegisterType, typename ISA, typename Allocator = std::allocator<u8>>
struct risc_v {
    using register_type = RegisterType;
    using float_type = RegisterType;

    constexpr risc_v(usize ram_sz = 0x1'0000, Allocator const& allocator = Allocator());

    constexpr void reset();

    constexpr void reset_and_load(std::string_view filename, infmt_ihex_tag);
    constexpr void reset_and_load(std::string_view filename, infmt_bin_tag);
    constexpr void reset_and_load(std::string_view filename, infmt_elf_tag);

    constexpr auto step() -> stf::expected<void, std::string_view>;

    // observers

    constexpr auto memory() const -> rv::memory<register_type, Allocator> const& { return m_memory; }
    constexpr auto memory() -> rv::memory<register_type, Allocator>& { return m_memory; }
    constexpr auto program_counter() -> register_type { return m_program_counter; }
    constexpr auto read_register(reg reg) -> register_type { return m_register_bank.read_register(reg); }

    register_bank<register_type> m_register_bank;
    rv::memory<register_type, Allocator> m_memory;
    register_type m_program_counter = 0;
    register_type m_next_step_sz = 4;
};

}  // namespace rv

#include <rv/detail/rv.ipp>
