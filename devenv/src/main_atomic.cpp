#include <atomic>
#include <concepts>
#include <cstdint>
#include <cstdlib>

extern std::atomic_int64_t test_outputs[10];

std::atomic_int64_t test_outputs[10]{
  0x0F0E'0E0B'0D0A'0E0Dl, 0, 0, 0, 0, 0x0E0B'0A0B'0E0F'0A0Cu,
};

int64_t strex_test = 0;

struct foo {
    foo() { test_outputs[1] |= 1; }

    ~foo() { test_outputs[1] |= 2; }
};

auto test_static_construction() -> int64_t {
    struct foo foo {};
    return test_outputs[1];
}

template<std::integral T>
[[gnu::always_inline, gnu::optimize("O3")]] inline auto load_reserved(T* addr) -> T {
    long reg;
    const auto addr_reg = reinterpret_cast<std::uintptr_t>(static_cast<void*>(addr));

    if constexpr (sizeof(T) == sizeof(int64_t)) {
        asm volatile("lr.d %[to], (%[from])" : [to] "=r"(reg) : [from] "r"(addr_reg));
    } else {
        asm volatile("lr.w %[to], (%[from])" : [to] "=r"(reg) : [from] "r"(addr_reg));
    }

    return static_cast<T>(reg);
}

template<std::integral T>
[[gnu::always_inline, gnu::optimize("O3")]] inline auto store_conditional(T* addr, T val) -> bool {
    long succ;
    long reg = val;
    const auto addr_reg = reinterpret_cast<std::uintptr_t>(static_cast<void*>(addr));

    if constexpr (sizeof(T) == sizeof(int64_t)) {
        asm volatile("sc.d %[succ], %[val], (%[to])" : [succ] "=r"(succ) : [to] "r"(addr_reg), [val] "r"(reg));
    } else {
        asm volatile("sc.w %[succ], %[val], (%[to])" : [succ] "=r"(succ) : [to] "r"(addr_reg), [val] "r"(reg));
    }

    return succ;
}

int main() {
    std::atexit([] { test_outputs[1] |= 4; });

    auto temp = load_reserved(&strex_test);
    test_outputs[2] = store_conditional(&strex_test, temp | 1);
    test_outputs[3] = store_conditional(&strex_test, temp | 2);
    temp = load_reserved(&strex_test);
    test_outputs[4] = store_conditional(&strex_test, temp | 4);

    test_outputs[1].store(8, std::memory_order_release);

    return test_static_construction();
}
