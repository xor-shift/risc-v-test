#include <cstring>

extern "C" void _start();
extern "C" long __bss_start[];
extern "C" long __bss_end[];
extern "C" long __data_start_flash[];
extern "C" long __data_start_ram[];
extern "C" long __data_end_ram[];

extern "C" [[gnu::alias("_ZN2rv6detail11entry_pointEv")]] void _c_entry_point();
extern "C" [[gnu::alias("_ZN2rv6detail12trap_handlerEv")]] void _c_trap_handler();

namespace rv::detail {

void zerofill_bss() {
    for (long* i = __bss_start; i < __bss_end; i++) {
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
}

void trap_handler() {
    //
}

}  // namespace rv::detail
