#include <nitro.h>
#include <nitro/prc/common.h>

static BOOL PRCi_Initialized = FALSE;

void PRCi_Init (void)
{
    if (PRCi_Initialized == TRUE)
        return;

    PRCi_Initialized = TRUE;
}

void PRC_GetEntryStrokes (PRCStrokes * strokes, const PRCPrototypeList * prototypeList, const PRCPrototypeEntry * entry)
{
    if (entry != NULL) {
        strokes->points = (PRCPoint *)&prototypeList->pointArray[entry->pointIndex];
        strokes->size = entry->pointCount;
        strokes->capacity = entry->pointCount;
    } else {
        strokes->points = NULL;
        strokes->size = 0;
        strokes->capacity = 0;
    }
}

BOOL PRC_CopyStrokes (const PRCStrokes * srcStrokes, PRCStrokes * dstStrokes)
{
    int iPoint, size;
    PRCPoint * dstPoint;
    const PRCPoint * srcPoint;

    SDK_ASSERT(dstStrokes);
    SDK_ASSERT(srcStrokes);

    if (srcStrokes->size > dstStrokes->capacity) {
        return FALSE;
    }

    size = dstStrokes->size = srcStrokes->size;
    srcPoint = srcStrokes->points;
    dstPoint = dstStrokes->points;

    for (iPoint = 0; iPoint < size; iPoint++) {
        *dstPoint = *srcPoint;
        dstPoint++;
        srcPoint++;
    }

    return TRUE;
}
