ENTRY(anya_handle_interface_request)
SECTIONS
{
    .text   : { *(.text*)   } > trampoline
    .rodata : { *(.rodata*) } > trampoline
    .note.gnu.property : { *(.note.gnu.property*) } > trampoline
}
