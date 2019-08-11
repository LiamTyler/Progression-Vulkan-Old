#pragma once

/*
 * This should generate a div by zero compilation error whenever you write USING( foo )
 * when foo is NOT_IN_USE.
 */
#define IN_USE 9
#define NOT_IN_USE 0
#define USING(x) ( (9 / x) == 1 )
#define NOT_USING(x) ( x == 0 )