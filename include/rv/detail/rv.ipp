#pragma once

#include <rv/detail/arith.hpp>
#include <rv/detail/instruction_descriptor.hpp>

namespace rv {

template<typename RegisterType, typename ISA, typename Allocator>
constexpr risc_v<RegisterType, ISA, Allocator>::risc_v(usize ram_sz, Allocator const& allocator)
    : m_memory(ram_sz, allocator) {
    auto ifs = std::ifstream("../devenv/build-debug/risc_v_test_prog.hex");
    m_memory.load_hex(ifs);
}

template<typename RegisterType, typename ISA, typename Allocator>
constexpr auto risc_v<RegisterType, ISA, Allocator>::step() -> stf::expected<void, std::string_view> {
    return ISA::try_step(*this);
}

}
