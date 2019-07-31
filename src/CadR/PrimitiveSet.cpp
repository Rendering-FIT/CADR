#include <CadR/PrimitiveSet.h>

using namespace cadR;

static_assert(sizeof(PrimitiveSet)<=4,
              "PrimitiveSet size is bigger than 4 bytes.\n"
              "Consider class redesign or rewising this assert.");
