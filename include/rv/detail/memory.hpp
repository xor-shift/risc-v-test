#pragma once

#include <stuff/expected.hpp>

#include <rv/detail/rand.hpp>

#include <filesystem>
#include <fstream>

namespace rv {

namespace detail {

struct intel_hex_record {
    static constexpr auto from_line(std::string_view line) -> stf::expected<intel_hex_record, std::string_view> {
        if (line.ends_with("\r")) {  //\r\n
            line.remove_suffix(1);
        }

        if (line.size() < 11) {
            return stf::unexpected{"line is too short"};
        }

        if (!line.starts_with(':')) {
            return stf::unexpected{"line doesn't start with ':'"};
        }

        line = line.substr(1);

        if (line.size() % 2 != 0) {
            return stf::unexpected{"line has an odd number of nibbles"};
        }

        auto ret = intel_hex_record{};
        ret.m_payload = line.substr(0, line.size() - 2);
        ret.m_checksum = construct_byte(std::span<const char, 2>(line.substr(line.size() - 2)));

        ret.byte_count = ret.consume_byte();

        const auto address_upper = ret.consume_byte();
        const auto address_lower = ret.consume_byte();
        ret.address = (address_upper << 8) | address_lower;

        const auto record_type_byte = ret.consume_byte();

        switch (record_type_byte) {
            case 0x00: ret.record_type = record_type::data; break;
            case 0x01: ret.record_type = record_type::end_of_file; break;
            case 0x02: ret.record_type = record_type::extended_segment_address; break;
            case 0x03: ret.record_type = record_type::start_segment_address; break;
            case 0x04: ret.record_type = record_type::extended_linear_address; break;
            case 0x05: ret.record_type = record_type::start_linear_address; break;
            default: return stf::unexpected{"bad record type byte"};
        }

        switch (ret.record_type) {
            case record_type::end_of_file:
                if (ret.byte_count != 0) {
                    return stf::unexpected{"bad byte count for an End of File record"};
                }
                break;

            case record_type::start_segment_address: [[fallthrough]];
            case record_type::start_linear_address:
                if (ret.byte_count != 4) {
                    return stf::unexpected{"bad byte count for an Start Address record"};
                }
                break;

            case record_type::extended_segment_address: [[fallthrough]];
            case record_type::extended_linear_address:
                if (ret.byte_count != 2) {
                    return stf::unexpected{"bad byte count for an Extended Address record"};
                }
                break;

            default: break;
        }

        return ret;
    }

    u8 byte_count;
    u16 address;
    enum record_type {
        data,
        end_of_file,
        extended_segment_address,
        start_segment_address,
        extended_linear_address,
        start_linear_address,
    } record_type;

    constexpr auto consume_byte() -> u8 {
        const auto ret = consume_byte_from(m_running_sum, m_payload);
        m_payload.remove_prefix(2);
        return ret;
    }

private:
    std::string_view m_payload;
    u8 m_checksum;

    int m_running_sum = 0;

    static constexpr auto nibble(char c) -> int {
        if (c >= 'A' && c <= 'F') {
            return 10 + (c - 'A');
        }

        if (c >= 'a' && c <= 'f') {
            return 10 + (c - 'a');
        }

        return c - '0';
    }

    static constexpr auto construct_byte(std::span<const char, 2> chars) -> u8 { return (u8)(nibble(chars[0]) << 4) | nibble(chars[1]); }

    static constexpr auto consume_byte_from(int& rolling_sum, std::string_view& view) -> u8 {
        stf::assume(view.size() >= 2);

        const char c[2]{view[0], view[1]};
        const auto res = construct_byte(c);
        rolling_sum += res;
        return res;
    }
};

}  // namespace detail

struct infmt_ihex_tag {};
struct infmt_bin_tag {};
struct infmt_elf_tag {};

template<typename RegisterType = u64, typename Allocator = std::allocator<u8>>
struct memory {
    using register_type = RegisterType;

    constexpr memory(register_type ram_sz, Allocator const& allocator = Allocator())
        : m_allocator(allocator)
        , m_memory_size((usize)ram_sz)
        , m_memory(m_allocator.allocate(ram_sz)) {
        if consteval {
            for (usize i = 0; i < m_memory_size; i++) {
                std::construct_at(m_memory + i, 0);
            }
        } else {
            auto gen = detail::prepare_rng();
            auto dist = std::uniform_int_distribution<u8>{};
            std::generate(m_memory, m_memory + m_memory_size, [&dist, &gen] { return dist(gen); });
        }
    }

    constexpr ~memory() { m_allocator.deallocate(m_memory, (usize)m_memory_size); }

    template<typename FileTypeTag>
    auto load_from(std::string_view filename, FileTypeTag, usize offset = 0) -> stf::expected<void, std::string_view> {
        auto ifs = std::ifstream(std::filesystem::path(filename));
        if (!ifs) {
            return stf::unexpected{"could not open file"};
        }

        return load_from(ifs, FileTypeTag{}, offset);
    }

    auto load_from(std::basic_istream<char>& input_stream, infmt_ihex_tag, usize offset = 0) -> stf::expected<void, std::string_view> {
        u32 base_address = 0;

        for (std::string str_raw; std::getline(input_stream, str_raw);) {
            std::string_view record_line = str_raw;
            auto record = TRYX(detail::intel_hex_record::from_line(record_line));

            switch (record.record_type) {
                case detail::intel_hex_record::record_type::data:
                    for (usize i = 0; i < record.byte_count; i++) {
                        m_memory[record.address + i + base_address] = record.consume_byte();
                    }
                    break;
                case detail::intel_hex_record::record_type::extended_segment_address:
                    base_address = record.consume_byte() << 8;
                    base_address |= record.consume_byte();
                    base_address *= 16;
                    break;
                default: return stf::unexpected { "unhandled record type" };
            }
        }

        return {};
    }

    auto load_from(std::basic_istream<char>& input_stream, infmt_bin_tag, usize offset = 0) -> stf::expected<void, std::string_view> {
        std::copy(std::istreambuf_iterator<char>(input_stream), std::istreambuf_iterator<char>(), m_memory);
        return {};
    }

    template<std::unsigned_integral T>
    constexpr auto read(register_type address) const -> T {
        std::array<u8, sizeof(T)> buf{0};
        std::copy_n(m_memory + address, std::min<T>(m_memory_size - address, sizeof(T)), buf.data());
        return stf::bit::convert_endian(std::bit_cast<T>(buf), std::endian::little, std::endian::little);
    }

    template<std::unsigned_integral T>
    constexpr void write(register_type address, T data) {
        const auto buf = std::bit_cast<std::array<u8, sizeof(T)>>(stf::bit::convert_endian(data, std::endian::native, std::endian::little));
        std::copy_n(buf.data(), std::min<T>(m_memory_size - address, buf.size()), m_memory + address);
    }

    constexpr auto data() -> u8* { return m_memory; }
    constexpr auto data() const -> const u8* { return m_memory; }

    constexpr auto size() const -> usize { return (usize)m_memory_size; }


    template<std::unsigned_integral T>
    constexpr auto load_reserved(register_type address) -> T {
        m_reservation = address;
        return read<T>(address);
    }

    template<std::unsigned_integral T>
    constexpr auto store_conditional(register_type address, T v) -> bool {
        if (!m_reservation) {
            return false;
        }

        if (*m_reservation != address) {
            return false;
        }

        m_reservation = std::nullopt;
        write<T>(address, v);

        return true;
    }

private:
    Allocator m_allocator;

    std::optional<register_type> m_reservation = std::nullopt;
    register_type m_memory_size = 0;
    u8* m_memory = nullptr;
};

}  // namespace rv
