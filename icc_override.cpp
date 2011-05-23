// Dispatcher-patch for Intel compiler 
// by Agner Fog (www.agner.org/optimize)
#include "asmlib.h" // Header file for asmlib library 
 
#ifdef __cplusplus 
extern "C" {        // Avoid C++ name mangling 
#endif 

// Global variable indicating cpu 
extern int __intel_cpu_indicator;  
 
// CPU dispatcher function 
void ___intel_cpu_indicator_init() { 
   // Get CPU level from asmlib library function 
   switch (InstructionSet()) { 
   case 0:  // No special instruction set supported 
      __intel_cpu_indicator = 1; 
      break; 
   case 1: case 2: // MMX supported 
      __intel_cpu_indicator = 8; 
      break; 
   case 3:  // SSE supported 
      __intel_cpu_indicator = 0x80; 
      break; 
   case 4:  // SSE2 supported 
      __intel_cpu_indicator = 0x200; 
      break; 
   case 5:  // SSE3 supported 
      __intel_cpu_indicator = 0x800; 
      break; 
   case 6: case 7:  // Supplementary-SSE3 supported 
      __intel_cpu_indicator = 0x1000; 
      break; 
   case 8: case 9:  // SSE4.1 supported 
      __intel_cpu_indicator = 0x2000; 
      break; 
   case 10: case 11: // SSE4.2 and POPCNT supported 
      __intel_cpu_indicator = 0x8000; 
      break; 
   case 12: // AVX, PCLMUL and AES supported 
   default: 
      __intel_cpu_indicator = 0x20000; 
      break; 
   } 
}
#ifdef __cplusplus 
}   // End of extern "C" 
#endif 
 