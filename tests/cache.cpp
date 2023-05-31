#include <gtest/gtest.h>

#include <stuff/core.hpp>

template<typename AddrType = u64, usize AddrBitsPerLine = 6>
struct cache {
    static constexpr usize address_bits_per_line = AddrBitsPerLine;
    static constexpr usize bytes_per_line = 1uz << AddrBitsPerLine;
    static constexpr usize line_count = 64;

    using address_type = AddrType;

    struct line_type {
        bool valid;
        address_type address;
        std::array<u8, bytes_per_line> data;
    };

private:
    std::array<line_type, line_count> m_lines {};
};

TEST(cache, cache) {

}
