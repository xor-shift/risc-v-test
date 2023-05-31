#pragma once

#include <rv/detail/arith.hpp>
#include <rv/detail/instruction_descriptor.hpp>

namespace rv {

template<typename RegisterType, typename ISA, typename Allocator>
constexpr void risc_v<RegisterType, ISA, Allocator>::reset() {
    m_program_counter = 0;
}

template<typename RegisterType, typename ISA, typename Allocator>
constexpr risc_v<RegisterType, ISA, Allocator>::risc_v(usize ram_sz, Allocator const& allocator)
    : m_memory(ram_sz, allocator) {}

template<typename RegisterType, typename ISA, typename Allocator>
constexpr auto risc_v<RegisterType, ISA, Allocator>::step() -> stf::expected<void, std::string_view> {
    return ISA::try_step(*this);
}

}  // namespace rv
