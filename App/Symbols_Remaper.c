#include "Application.h"

//------------------------------------------------------------------------------
// Symbol remapping configuration macros
// Uncomment one of the following macros to enable specific symbol remapping:
//
// #define REMAP_7A8B9C            // 7-A-8-B-9-C pattern
// #define REMAP_789               // 7-8-9 pattern
// #define REMAP_LG_G_1            // LG-G-1 pattern
// #define REMAP_M2_M1_0_1         // -2,-1,0,1 pattern
// #define REMAP_B_G_2_3           // B-G-2-3 pattern
// #define REMAP_B_G_1_2_3         // B-G-1-2-3 pattern
// #define REMAP_1_M_2_3_4_R       // 1-M-2-3-4-R pattern
// #define REMAP_1_M_3_4_5_6       // 1-M-3-4-5-6 pattern
// #define REMAP_G_1_2_3_4_5       // G-1-2-3-4-5 pattern
// #define REMAP_0_2_3_4_5_6       // 0-2-3-4-5-6 pattern
// #define REMAP_B_1_2_3_4_5       // B-1-2-3-4-5 pattern
// #define REMAP_B_2_3_4_5_6       // B-2-3-4-5-6 pattern
// #define REMAP_LG_1_2_3_4_5      // LG-1-2-3-4-5 pattern
// #define REMAP_M1_1_2_3_4_5      // -1,1-2-3-4-5 pattern
// #define REMAP_B_M_1_2_3_4       // B-M-1-2-3-4 pattern
// #define REMAP_1_G_LG_3_4_5      // 1-G-LG-3-4-5 pattern
// #define REMAP_1_2_3_4_5_6       // 1-2-3-4-5-6 pattern
// #define REMAP_0A_0B_1_2_3_4     // 0A-0B-1-2-3-4 pattern
// #define REMAP_A_E_1_3           // A-E-1-3 pattern
// #define REMAP_L_1_2_3           // L-1-2-3 pattern
// #define REMAP_MINUS1_0_1_2_3_4  // -1,0,1,2,3,4 pattern
#define REMAP_KG_EG_OG_3_4_5_6  // KG-EG-OG-3-4-5-6 pattern
//
// If no macro is defined, default 0-9 mapping is used
//------------------------------------------------------------------------------

#define REMAP_SZ 10

#ifdef REMAP_7A8B9C
T_remap_sym remap_array[REMAP_SZ] =
{
 {0, SYM_7},
 {1, SYM_A},
 {2, SYM_8},
 {3, SYM_B},
 {4, SYM_9},
 {5, SYM_C},
 {6, SYM_6},
 {7, SYM_7},
 {8, SYM_8},
 {9, SYM_9},
};
#elif defined(REMAP_789)
T_remap_sym remap_array[REMAP_SZ] =
{
 {0, SYM_7},
 {1, SYM_8},
 {2, SYM_9},
 {3, SYM_3},
 {4, SYM_4},
 {5, SYM_5},
 {6, SYM_6},
 {7, SYM_7},
 {8, SYM_8},
 {9, SYM_9},
};

#elif defined(REMAP_LG_G_1)

T_remap_sym remap_array[REMAP_SZ] =
{
 {0, SYM_LG},
 {1, SYM_G},
 {2, SYM_1},
 {3, SYM_2},
 {4, SYM_3},
 {5, SYM_4},
 {6, SYM_5},
 {7, SYM_6},
 {8, SYM_7},
 {9, SYM_8},
};

#elif defined(REMAP_M2_M1_0_1)

T_remap_sym remap_array[REMAP_SZ] =
{
 {0, SYM_MINUS_2},
 {1, SYM_MINUS_1},
 {2, SYM_0},
 {3, SYM_1},
 {4, SYM_2},
 {5, SYM_3},
 {6, SYM_4},
 {7, SYM_5},
 {8, SYM_6},
 {9, SYM_7},
};

#elif defined(REMAP_B_G_2_3)

T_remap_sym remap_array[REMAP_SZ] =
{
 {0, SYM_B},
 {1, SYM_G},
 {2, SYM_2},
 {3, SYM_3},
 {4, SYM_4},
 {5, SYM_5},
 {6, SYM_6},
 {7, SYM_7},
 {8, SYM_8},
 {9, SYM_9},
};

#elif defined(REMAP_B_G_1_2_3)

T_remap_sym remap_array[REMAP_SZ] =
{
 {0, SYM_B},
 {1, SYM_G},
 {2, SYM_1},
 {3, SYM_2},
 {4, SYM_3},
 {5, SYM_4},
 {6, SYM_5},
 {7, SYM_6},
 {8, SYM_7},
 {9, SYM_8},
};

#elif defined(REMAP_1_M_2_3_4_R)

T_remap_sym remap_array[REMAP_SZ] =
{
 {0, SYM_1},
 {1, SYM_M},
 {2, SYM_2},
 {3, SYM_3},
 {4, SYM_4},
 {5, SYM_R},
 {6, SYM_6},
 {7, SYM_7},
 {8, SYM_8},
 {9, SYM_9},
};

#elif defined(REMAP_1_M_3_4_5_6)

T_remap_sym remap_array[REMAP_SZ] =
{
 {0, SYM_1},
 {1, SYM_M},
 {2, SYM_3},
 {3, SYM_4},
 {4, SYM_5},
 {5, SYM_6},
 {6, SYM_7},
 {7, SYM_8},
 {8, SYM_9},
 {9, SYM_9},
};

#elif defined(REMAP_G_1_2_3_4_5)

T_remap_sym remap_array[REMAP_SZ] =
{
 {0, SYM_G},
 {1, SYM_1},
 {2, SYM_2},
 {3, SYM_3},
 {4, SYM_4},
 {5, SYM_5},
 {6, SYM_6},
 {7, SYM_7},
 {8, SYM_8},
 {9, SYM_9},
};

#elif defined(REMAP_0_2_3_4_5_6)

T_remap_sym remap_array[REMAP_SZ] =
{
 {0, SYM_0},
 {1, SYM_2},
 {2, SYM_3},
 {3, SYM_4},
 {4, SYM_5},
 {5, SYM_6},
 {6, SYM_7},
 {7, SYM_8},
 {8, SYM_9},
 {9, SYM_0},
};
#elif defined(REMAP_B_1_2_3_4_5)

T_remap_sym remap_array[REMAP_SZ] =
{
 {0, SYM_B},
 {1, SYM_1},
 {2, SYM_2},
 {3, SYM_3},
 {4, SYM_4},
 {5, SYM_5},
 {6, SYM_6},
 {7, SYM_7},
 {8, SYM_8},
 {9, SYM_9},
};

#elif defined(REMAP_B_2_3_4_5_6)

T_remap_sym remap_array[REMAP_SZ] =
{
 {0, SYM_B},
 {1, SYM_2},
 {2, SYM_3},
 {3, SYM_4},
 {4, SYM_5},
 {5, SYM_6},
 {6, SYM_7},
 {7, SYM_8},
 {8, SYM_9},
 {9, SYM_0},
};

#elif defined(REMAP_LG_1_2_3_4_5)

T_remap_sym remap_array[REMAP_SZ] =
{
 {0, SYM_LG},
 {1, SYM_1},
 {2, SYM_2},
 {3, SYM_3},
 {4, SYM_4},
 {5, SYM_5},
 {6, SYM_6},
 {7, SYM_7},
 {8, SYM_8},
 {9, SYM_9},
};

#elif defined(REMAP_M1_1_2_3_4_5)

T_remap_sym remap_array[REMAP_SZ] =
{
 {0, SYM_MINUS_1},
 {1, SYM_1},
 {2, SYM_2},
 {3, SYM_3},
 {4, SYM_4},
 {5, SYM_5},
 {6, SYM_6},
 {7, SYM_7},
 {8, SYM_8},
 {9, SYM_9},
};

#elif defined(REMAP_B_M_1_2_3_4)

T_remap_sym remap_array[REMAP_SZ] =
{
 {0, SYM_B},
 {1, SYM_M},
 {2, SYM_1},
 {3, SYM_2},
 {4, SYM_3},
 {5, SYM_4},
 {6, SYM_5},
 {7, SYM_6},
 {8, SYM_7},
 {9, SYM_8},
};

#elif defined(REMAP_1_G_LG_3_4_5)

T_remap_sym remap_array[REMAP_SZ] =
{
 {0, SYM_1},
 {1, SYM_G},
 {2, SYM_LG},
 {3, SYM_3},
 {4, SYM_4},
 {5, SYM_5},
 {6, SYM_6},
 {7, SYM_7},
 {8, SYM_8},
 {9, SYM_9},
};

#elif defined(REMAP_1_2_3_4_5_6)

T_remap_sym remap_array[REMAP_SZ] =
{
 {0, SYM_1},
 {1, SYM_2},
 {2, SYM_3},
 {3, SYM_4},
 {4, SYM_5},
 {5, SYM_6},
 {6, SYM_7},
 {7, SYM_8},
 {8, SYM_9}, {9, SYM_0},
};
#elif defined(REMAP_0A_0B_1_2_3_4)

T_remap_sym remap_array[REMAP_SZ] =
{
 {0, SYM_0A},
 {1, SYM_0B},
 {2, SYM_1},
 {3, SYM_2},
 {4, SYM_3},
 {5, SYM_4},
 {6, SYM_5},
 {7, SYM_6},
 {8, SYM_7},
 {9, SYM_8},
};

#elif defined(REMAP_A_E_1_3)

T_remap_sym remap_array[REMAP_SZ] =
{
 {0, SYM_A},
 {1, SYM_E},
 {2, SYM_1},
 {3, SYM_3},
 {4, SYM_4},
 {5, SYM_5},
 {6, SYM_6},
 {7, SYM_7},
 {8, SYM_8},
 {9, SYM_9},
};

#elif defined(REMAP_L_1_2_3)

T_remap_sym remap_array[REMAP_SZ] =
{
 {0, SYM_L},
 {1, SYM_1},
 {2, SYM_2},
 {3, SYM_3},
 {4, SYM_4},
 {5, SYM_5},
 {6, SYM_6},
 {7, SYM_7},
 {8, SYM_8},
 {9, SYM_9},
};

#elif defined(REMAP_MINUS1_0_1_2_3_4)

T_remap_sym remap_array[REMAP_SZ] =
{
 {0, SYM_MINUS_1},
 {1, SYM_0},
 {2, SYM_1},
 {3, SYM_2},
 {4, SYM_3},
 {5, SYM_4},
 {6, SYM_5},
 {7, SYM_6},
 {8, SYM_7},
 {9, SYM_8},
};

#elif defined(REMAP_KG_EG_OG_3_4_5_6)

T_remap_sym remap_array[REMAP_SZ] =
{
 {0, SYM_KG},
 {1, SYM_EG},
 {2, SYM_OG},
 {3, SYM_3},
 {4, SYM_4},
 {5, SYM_5},
 {6, SYM_6},
 {7, SYM_7},
 {8, SYM_8},
 {9, SYM_9},
};

#else

T_remap_sym remap_array[REMAP_SZ] =
{
 {0, SYM_0},
 {1, SYM_1},
 {2, SYM_2},
 {3, SYM_3},
 {4, SYM_4},
 {5, SYM_5},
 {6, SYM_6},
 {7, SYM_7},
 {8, SYM_8},
 {9, SYM_9},
};

#endif

/*-----------------------------------------------------------------------------------------------------


  \param code

  \return int
-----------------------------------------------------------------------------------------------------*/
int32_t Remap_sym_code(int32_t code)
{
  if (code < REMAP_SZ)
  {
    return remap_array[code].to_sym;
  }
  return code;
}
