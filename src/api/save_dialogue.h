#ifndef DOGGAME_SAVE_DIALOGUE_H
#define DOGGAME_SAVE_DIALOGUE_H
#include <math.h>

static float save_dialogue_delta(float elapsed, float maximum)
{
    if (!isfinite(elapsed) || elapsed < 0.0f)
    {
        return 0.0f;
    }
    return elapsed > maximum ? maximum : elapsed;
}
#endif
