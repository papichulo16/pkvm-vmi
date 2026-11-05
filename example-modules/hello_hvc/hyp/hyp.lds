
SECTIONS {
 .hyp.text : {
  __hyp_section_.hyp.text = .;
  *(.text .text.*)
 }
 .hyp.bss : {
  __hyp_section_.hyp.bss = .;
  *(.bss .bss.*)
 }
 .hyp.rodata : {
  __hyp_section_.hyp.rodata = .;
  *(.rodata .rodata.*)
 }
 .hyp.data : {
  __hyp_section_.hyp.data = .;
  *(.data .data.*)
 }
}
