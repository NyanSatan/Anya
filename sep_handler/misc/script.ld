ENTRY(anya_mailbox_loop)
SECTIONS
{
    .text   : { *(.text*)   } > trampoline
    .rodata : { *(.rodata*) } > trampoline
    .note.gnu.property : { *(.note.gnu.property*) } > trampoline
}
