#ifndef __SYMBOLS_H
#define __SYMBOLS_H

// Symbol definitions for LED display
// Digits 0-9
#define SYM_0           0
#define SYM_1           1
#define SYM_2           2
#define SYM_3           3
#define SYM_4           4
#define SYM_5           5
#define SYM_6           6
#define SYM_7           7
#define SYM_8           8
#define SYM_9           9

// Special symbols and patterns
#define SYM_FRAME_THICK 10  // Thick frame border
#define SYM_FRAME_THIN  11  // Thin frame border
#define SYM_BLANK       12  // Empty/blank symbol
#define SYM_DOT_SMALL   13  // Small dot (2x2)
#define SYM_DOT_MEDIUM  14  // Medium dot (4x4)
#define SYM_DOT_LARGE   15  // Large dot (6x6)
#define SYM_FRAME_FULL  16  // Full frame border
#define SYM_ARROW_UP    17  // Arrow pointing up
#define SYM_ARROW_DOWN  18  // Arrow pointing down
#define SYM_DIAGONAL    19  // Diagonal line pattern
#define SYM_EMPTY       20  // Empty symbol
#define SYM_LINE_BOTTOM 21  // Horizontal line at bottom
#define SYM_LINE_6      22  // Horizontal line at row 6
#define SYM_LINE_5      23  // Horizontal line at row 5
#define SYM_LINE_4      24  // Horizontal line at row 4
#define SYM_LINE_3      25  // Horizontal line at row 3
#define SYM_LINE_2      26  // Horizontal line at row 2
#define SYM_LINE_1      27  // Horizontal line at row 1
#define SYM_LINE_TOP    28  // Horizontal line at top
#define SYM_SPACE       29  // Space character

// Letters
#define SYM_A           30
#define SYM_B           31
#define SYM_C           32
#define SYM_G           33
#define SYM_K           34
#define SYM_LG          35
#define SYM_MINUS_1     36
#define SYM_R           37
#define SYM_UG          38
#define SYM_M           39
#define SYM_P           40
#define SYM_MINUS_2     41
#define SYM_PATTERN_X1  42  // Cross pattern 1
#define SYM_PATTERN_X2  43  // Cross pattern 2
#define SYM_S           44  // Letter 'S'
#define SYM_0A          45
#define SYM_0B          46
#define SYM_E           47
#define SYM_L           48
#define SYM_KG          49  // Double symbol 'KG'
#define SYM_EG          50  // Double symbol 'EG'
#define SYM_OG          51  // Double symbol 'OG'

/* Объявление массива символов для LED дисплея */
extern unsigned char Symbols[][8];

extern int Get_symbols_count(void);

#endif
