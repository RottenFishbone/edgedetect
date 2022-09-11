/**
 * @file common.h
 * @author Jayden Dumouchel
 * @date 9 Sep 2022
 *
 * @brief A global set of includes/preprocessor definitions
 */

#ifndef _ED_COMMON_H
#define _ED_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>

// Inverted text "ERR" with red background 
#define ERR_TXT "\033[7;31mERR\033[0m" 
// Inverted text "INFO"
#define INFO_TXT "\033[7mINFO\033[0m"
// Inverted text "WARN"
#define WARN_TXT "\033[7mWARN\033[0m"

#define MAX(a, b) (a > b ? a : b)
#define MIN(a, b) (a < b ? a : b)

#endif
