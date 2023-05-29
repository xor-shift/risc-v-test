#pragma once

#include <stuff/random.hpp>

namespace rv::detail {

template<typename Gen = stf::random::xoshiro_256p>
constexpr auto prepare_rng() -> Gen {
    const auto base_seed = ({
        std::random_device::result_type ret;

        if consteval {
            if constexpr (std::is_same_v<std::random_device::result_type, u64>) {
                ret = 0xDEADBEEF'CAFEBABEull;
            } else if constexpr (std::is_same_v<std::random_device::result_type, u32>) {
                ret = 0xDEADBEEFul ^ 0xCAFEBABEul;
            } else {
                throw 1;
            }
        } else {
            std::random_device rd{};
            ret = rd();
        }

        ret;
    });

    const auto seed = ({
        u64 ret;
        if constexpr (std::is_same_v<std::random_device::result_type, u64>) {
            ret = base_seed;
        } else if constexpr (std::is_same_v<std::random_device::result_type, u32>) {
            stf::random::splitmix_32_generator gen{base_seed};
            ret = (u64)gen() << 32ull | (u64)gen();
        } else {
            std::unreachable();
        }
        ret;
    });

    auto gen = stf::random::xoshiro_256p{seed};

    return gen;
}

}
