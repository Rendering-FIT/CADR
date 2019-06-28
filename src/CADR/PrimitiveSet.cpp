#include <CADR/DrawCommand.h>

using namespace cd;

static_assert(sizeof(DrawCommand)<=4,
              "DrawCommand size is bigger than 4 bytes.\n"
              "Consider class redesign or rewising this assert.");
