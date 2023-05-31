#pragma once

#include <rv/detail/memory.hpp>
#include <rv/detail/registers.hpp>

namespace rv {

enum class csr_write_type {
    write,
    set,
    clear,
}

template<typename RegisterType, typename ISA, typename Allocator = std::allocator<u8>>
struct risc_v {
    using register_type = RegisterType;
    using float_type = RegisterType;

    constexpr risc_v(usize ram_sz = 0x1'0000, Allocator const& allocator = Allocator());

    constexpr void reset();

    template<typename FileTypeTag>
    constexpr void load(std::string_view filename, FileTypeTag, usize offset = 0) {
        reset();
        m_memory.load_from(filename, FileTypeTag{}, offset);
    }

    constexpr auto step() -> stf::expected<void, std::string_view>;

    // observers

    constexpr auto memory() const -> rv::memory<register_type, Allocator> const& { return m_memory; }
    constexpr auto memory() -> rv::memory<register_type, Allocator>& { return m_memory; }
    constexpr auto program_counter() -> register_type { return m_program_counter; }
    constexpr auto read_register(reg reg) -> register_type { return m_register_bank.read_register(reg); }

    template<csr_write_type Type>
    constexpr auto csr_read_write(reg destination, register_type value, u32 addr) {
        auto val = m_register_bank.read_register(src);
        //
    }

    register_bank<register_type> m_register_bank;
    rv::memory<register_type, Allocator> m_memory;
    register_type m_program_counter = 0;

    register_type m_next_step_sz = 4;
};

}  // namespace rv

#include <rv/detail/rv.ipp>
