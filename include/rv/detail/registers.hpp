#pragma once

#include <rv/detail/arith.hpp>
#include <rv/detail/definitions.hpp>
#include <rv/detail/rand.hpp>

namespace rv {

template<typename RegisterType = u64, typename FloatType = RegisterType>
struct register_bank {
    using register_type = RegisterType;
    using float_type = RegisterType;

    constexpr register_bank() {
        auto gen = detail::prepare_rng();
        auto dist = std::uniform_int_distribution<register_type>{};
        for (auto& reg : m_registers) {
            reg = dist(gen);
            //reg = 0;
        }
    }

    template<std::unsigned_integral T>
    constexpr auto register_rw(rv::reg reg_dst, T dst_val, rv::reg reg_src_1, rv::reg reg_src_2) -> std::pair<register_type, register_type> {
        if (reg_dst == reg_src_1 || reg_dst == reg_src_2) {
            throw std::runtime_error("data hazard, destination register equals one of the source registers");
        }

        m_registers[static_cast<u32>(reg_dst)] = sext<register_type>((register_type)dst_val, std::numeric_limits<T>::digits);
        return std::make_pair(read_register(reg_src_1), read_register(reg_src_2));
    }

    constexpr void write_register(rv::reg reg, std::unsigned_integral auto val) { m_registers[static_cast<u32>(reg)] = arith::sext<register_type, 32>((register_type)val); }
    constexpr auto read_register(rv::reg reg) -> register_type { return reg == rv::reg::zero ? (register_type)0 : m_registers[static_cast<u32>(reg)]; }

    constexpr void write_register(rv::float_reg reg, float_type val) { m_float_registers[static_cast<u32>(reg)] = val; }
    constexpr auto read_register(rv::float_reg reg) -> float_type { return m_float_registers[static_cast<u32>(reg)]; }

private:
    register_type m_registers[32]{};
    float_type m_float_registers[32]{};
};

}
