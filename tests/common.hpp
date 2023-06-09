#pragma once

#include <stuff/core.hpp>

#include <gtest/gtest.h>
#include <fmt/format.h>

#include <span>

template<typename ISA, usize N = std::dynamic_extent>
auto run_tests(std::span<const std::pair<u32, std::string_view>, N> test_cases, ISA& isa, bool do_prints = false) {
    for (usize i = 0; auto const& [instruction_word, expected_str] : test_cases) {
        auto got_str = isa.format(instruction_word, false);
        const auto ok = expected_str == got_str;

        ASSERT_EQ(got_str, expected_str) << fmt::format("case {}, word: {:#08X}", i++, instruction_word);
    }
};
