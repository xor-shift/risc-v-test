#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>

extern "C" void __libc_preinit_array();
extern "C" void __libc_init_array();
extern "C" void __libc_fini_array();
extern "C" [[noreturn]] void _start();
extern "C" int __bss_start[];
extern "C" int __bss_end[];
extern "C" long __data_start_flash[];
extern "C" long __data_start_ram[];
extern "C" long __data_end_ram[];

extern "C" int main(int, char**);

extern "C" [[gnu::alias("_ZN2rv6detail11entry_pointEv")]] void _c_entry_point();
extern "C" [[gnu::alias("_ZN2rv6detail12trap_handlerEv")]] void _c_trap_handler();

extern "C" void __cxa_pure_virtual();
extern "C" int __cxa_atexit(void (*function)(void*), void* argument, void* dso);
extern "C" void __cxa_finalize(void* function);

namespace rv::detail {

void zerofill_bss() {
    for (int* i = __bss_start; i < __bss_end; i++) {
        // avoid memset on -O3
        // *reinterpret_cast<volatile int*>(i) = 0;
        // ^ we have -lc now

        *i = 0;
    }

    // memset(__bss_start, 0, (__bss_end - __bss_start) * sizeof(int));
}

void load_data() {
    long* data_ptr_flash = __data_start_flash;
    long* data_ptr_ram = __data_start_ram;
    long* data_ptr_ram_end = __data_end_ram;

    while (data_ptr_ram != data_ptr_ram_end) {
        *data_ptr_ram++ = *data_ptr_flash++;
    }
}

void entry_point() {
    zerofill_bss();
    load_data();
    _start();

    //__libc_init_array();

    //char* dummy_argv[]{};
    //main(0, nullptr);

    //__cxa_finalize(nullptr);
    //__libc_fini_array();
}

void trap_handler() {
    //
}

struct atexit_entry {
    void (*function)(void*);
    void* argument;
    void* dso;

    constexpr bool callable() const { return function != nullptr; }

    void call_and_destroy() {
        function(argument);
        argument = nullptr;
    }
};

static constexpr size_t atexit_max_entries = 64;
static size_t atexit_entry_count = 0;

static atexit_entry atexit_entries[atexit_max_entries]{};

void finalise_all_atexit() {
    for (size_t i = 0; i < rv::detail::atexit_entry_count; i++) {
        auto& entry = rv::detail::atexit_entries[rv::detail::atexit_entry_count - i - 1];

        if (!entry.callable()) {
            continue;
        }

        entry.call_and_destroy();
    }
}

void finalise_specific_atexit(void (*expected_function)(void*)) {
    if (expected_function == nullptr) {
        std::unreachable();
    }

    for (size_t i = 0; i < rv::detail::atexit_entry_count; i++) {
        auto& entry = rv::detail::atexit_entries[i];

        if (entry.function != expected_function) {
            continue;
        }

        entry.call_and_destroy();

        break;
    }
}

}  // namespace rv::detail

/*
extern "C" void __cxa_pure_virtual() {
    // STUB
}

extern "C" int __cxa_atexit(void (*function)(void*), void* argument, void* dso) {
    if (rv::detail::atexit_entry_count == rv::detail::atexit_max_entries) {
        return -1;
    }

    rv::detail::atexit_entries[rv::detail::atexit_entry_count++] = {
      .function = function,
      .argument = argument,
      .dso = dso,
    };

    return 0;
}

extern "C" void __cxa_finalize(void* function) {
    if (function == nullptr) [[likely]] {
        rv::detail::finalise_all_atexit();
        return;
    }

    rv::detail::finalise_specific_atexit(reinterpret_cast<void (*)(void*)>(function));
}
*/

// extern "C" void __cxa_finalize(void *f);
