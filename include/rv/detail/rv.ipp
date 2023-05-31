#pragma once

#include <rv/detail/arith.hpp>
#include <rv/detail/instruction_descriptor.hpp>

namespace rv {

template<typename RegisterType, typename Allocator>
constexpr void risc_v<RegisterType, Allocator>::reset() {
    m_program_counter = 0;
}

template<typename RegisterType, typename Allocator>
constexpr risc_v<RegisterType, Allocator>::risc_v(generic_instruction_set<risc_v<RegisterType, Allocator>> const& isa, usize ram_sz, Allocator const& allocator)
    : m_isa(isa)
    , m_memory(ram_sz, allocator) {}

template<typename RegisterType, typename Allocator>
constexpr auto risc_v<RegisterType, Allocator>::step() -> stf::expected<void, std::string_view> {
    m_isa.try_step(*this);
    return {};
}

}  // namespace rv
