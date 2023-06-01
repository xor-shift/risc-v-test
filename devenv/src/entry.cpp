#include <cstring>

extern "C" [[noreturn]] void _start();
extern "C" long __bss_start[];
extern "C" long __bss_end[];
extern "C" long __data_start_flash[];
extern "C" long __data_start_ram[];
extern "C" long __data_end_ram[];

extern "C" [[gnu::alias("_ZN2rv6detail11entry_pointEv"), noreturn]] void _c_entry_point();
extern "C" [[gnu::alias("_ZN2rv6detail12trap_handlerEv")]] void _c_trap_handler();

namespace rv::detail {

void load_data() {
    long* data_ptr_flash = __data_start_flash;
    long* data_ptr_ram = __data_start_ram;
    long* data_ptr_ram_end = __data_end_ram;

    while (data_ptr_ram != data_ptr_ram_end) {
        *data_ptr_ram++ = *data_ptr_flash++;
    }
}

[[noreturn]] void entry_point() {
    load_data();
    _start();
}

void trap_handler() {
    //
}

}  // namespace rv::detail
