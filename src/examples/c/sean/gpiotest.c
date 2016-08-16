// gpio test
// just test reading/writing gpio pins

// c headers
#include <stdio.h>

// qot header
#include "../../../api/c/qot.h"

// printing for debugging; basically print if DEBUG is 1, nothing o.w.
#define DEBUG 1
#define debugprintf(...) (DEBUG ? printf(__VA_ARGS__) : 0)


static const char *PIN_LOW  = "0";
static const char *PIN_HIGH = "1";


int main(int argc, char **argv)
{
	return 0;
}