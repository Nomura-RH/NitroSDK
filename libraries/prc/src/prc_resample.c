#include <nitro.h>
#include <nitro/prc/resample.h>

static BOOL PRCi_TerminateStrokes (u16 * selectedPoints, int * pSelectedPointNum, int maxPointCount, const PRCStrokes * strokes)
{
    int selectedPointNum = *pSelectedPointNum;
    const PRCPoint * inputPoints;

    inputPoints = strokes->points;
    if (selectedPointNum < 2) {

        *pSelectedPointNum = 0;
        return FALSE;
    }

    if (!PRC_IsPenUpMarker(&inputPoints[selectedPoints[selectedPointNum - 1]])) {
        if (!PRC_IsPenUpMarker(&inputPoints[selectedPoints[selectedPointNum - 2]])) {
            if (selectedPointNum < maxPointCount) {
                selectedPoints[selectedPointNum] = (u16) - 1;
                selectedPointNum++;
            } else {
                if (selectedPointNum >= 3
                    && !PRC_IsPenUpMarker(&inputPoints[selectedPoints[selectedPointNum - 3]])) {
                    selectedPoints[selectedPointNum - 1] = (u16) - 1;
                } else {
                    selectedPointNum -= 2;
                }
            }
        } else {
            selectedPointNum--;
        }
    }

    *pSelectedPointNum = selectedPointNum;
    return TRUE;
}

BOOL PRC_ResampleStrokes_None (u16 * selectedPoints, int * pSelectedPointNum, int maxPointCount, int maxStrokeCount, const PRCStrokes * strokes, int threshold, void * buffer)
{
    u16 iPoint;
    int size = strokes->size;

    (void)maxStrokeCount;
    (void)threshold;
    (void)buffer;

    if (size > maxPointCount) {
        size = maxPointCount;
    }

    if (size < 2) {
        *pSelectedPointNum = 0;
    } else {
        for (iPoint = 0; iPoint < size; iPoint++) {
            selectedPoints[iPoint] = iPoint;
        }

        *pSelectedPointNum = iPoint;

        if (!PRC_IsPenUpMarker(&strokes->points[size - 1])) {
            (void)PRCi_TerminateStrokes(selectedPoints, pSelectedPointNum, maxPointCount, strokes);
        }
    }

    return (*pSelectedPointNum > 0);
}

#define PRCi_ABS(x) (((x) >= 0)?(x):-(x))

BOOL PRC_ResampleStrokes_Distance (u16 * selectedPoints, int * pSelectedPointNum, int maxPointCount, int maxStrokeCount, const PRCStrokes * strokes, int threshold, void * buffer)
{
    int selectedPointNum;
    int strokeCount;
    int iPoint;
    int size;
    PRCPoint prevPoint;
    PRCPoint * point;
    BOOL newFlag;
    int length;

    SDK_ASSERT(maxPointCount > 0);
    SDK_ASSERT(maxStrokeCount > 0);

    (void)buffer;

    selectedPointNum = 0;
    strokeCount = 0;
    size = strokes->size;
    point = strokes->points;

    newFlag = TRUE;

    for (iPoint = 0; iPoint < size && selectedPointNum < maxPointCount; iPoint++, point++) {
        if (!PRC_IsPenUpMarker(point)) {
            if (newFlag) {
                selectedPoints[selectedPointNum] = (u16)iPoint;
                selectedPointNum++;
                length = 0;
                newFlag = FALSE;
            } else {
                length += PRCi_ABS(point->x - prevPoint.x) + PRCi_ABS(point->y - prevPoint.y);
                if (length >= threshold) {
                    selectedPoints[selectedPointNum] = (u16)iPoint;
                    selectedPointNum++;
                    length = 0;
                }
            }
            prevPoint = *point;
        } else {
            if (newFlag) {
                continue;
            } else {
                if (selectedPoints[selectedPointNum - 1] != iPoint - 1) {
                    selectedPoints[selectedPointNum] = (u16)(iPoint - 1);
                    selectedPointNum++;

                    if (selectedPointNum >= maxPointCount) {
                        break;
                    }
                }

                selectedPoints[selectedPointNum] = (u16)iPoint;
                selectedPointNum++;

                newFlag = TRUE;

                strokeCount++;
                if (strokeCount >= maxStrokeCount) {
                    iPoint++;
                    break;
                }
            }
        }
    }

    *pSelectedPointNum = selectedPointNum;

    if (!newFlag) {
        if (selectedPointNum > 0 && selectedPoints[selectedPointNum - 1] != iPoint - 1
            && selectedPointNum < maxPointCount) {
            selectedPoints[*pSelectedPointNum] = (u16)(iPoint - 1);
            (*pSelectedPointNum)++;
        }

        (void)PRCi_TerminateStrokes(selectedPoints, pSelectedPointNum, maxPointCount, strokes);
    }

    return (*pSelectedPointNum > 0);
}

BOOL PRC_ResampleStrokes_Angle (u16 * selectedPoints, int * pSelectedPointNum, int maxPointCount, int maxStrokeCount, const PRCStrokes * strokes, int threshold, void * buffer)
{
#define PRC_RESAMPLE_ANGLE_LENGTH_THRESHOLD 6
    int selectedPointNum;
    int strokeCount;
    int iPoint;
    int size;
    PRCPoint * point;
    BOOL newFlag;
    u16 prevAngle;
    PRCPoint prevPoint;
    BOOL firstFlag;

    SDK_ASSERT(maxPointCount > 0);
    SDK_ASSERT(maxStrokeCount > 0);

    (void)buffer;

    selectedPointNum = 0;
    strokeCount = 0;
    size = strokes->size;
    point = strokes->points;

    newFlag = TRUE;

    for (iPoint = 0; iPoint < size && selectedPointNum < maxPointCount; iPoint++, point++) {
        if (!PRC_IsPenUpMarker(point)) {
            if (newFlag) {
                selectedPoints[selectedPointNum] = (u16)iPoint;
                selectedPointNum++;
                prevPoint = *point;
                newFlag = FALSE;
                firstFlag = TRUE;
                if (iPoint + 1 < size) {
                    prevAngle =
                        FX_Atan2Idx(((point + 1)->y - point->y) << FX32_SHIFT,
                                    ((point + 1)->x - point->x) << FX32_SHIFT);
                }
            } else {
                int length;
                length = PRCi_ABS(point->x - prevPoint.x) + PRCi_ABS(point->y - prevPoint.y);
                if (length >= PRC_RESAMPLE_ANGLE_LENGTH_THRESHOLD) {
                    if (firstFlag) {

                        if (iPoint + 1 < size && !PRC_IsPenUpMarker(point + 1)) {
                            u16 currAngle, nextAngle;
                            nextAngle =
                                FX_Atan2Idx(((point + 1)->y - point->y) << FX32_SHIFT,
                                            ((point + 1)->x - point->x) << FX32_SHIFT);
                            if (PRCi_ABS((s16)(prevAngle - nextAngle)) >= threshold) {
                                currAngle =
                                    FX_Atan2Idx((point->y - prevPoint.y) << FX32_SHIFT,
                                                (point->x - prevPoint.x) << FX32_SHIFT);
                                selectedPoints[selectedPointNum] = (u16)iPoint;
                                selectedPointNum++;
                                prevAngle = currAngle;
                            }
                        }
                        firstFlag = FALSE;
                    } else {
                        u16 currAngle;
                        currAngle =
                            FX_Atan2Idx((point->y - prevPoint.y) << FX32_SHIFT,
                                        (point->x - prevPoint.x) << FX32_SHIFT);
                        if (PRCi_ABS((s16)(prevAngle - currAngle)) >= threshold) {
                            selectedPoints[selectedPointNum] = (u16)iPoint;
                            selectedPointNum++;
                            prevAngle = currAngle;
                        }
                    }
                    prevPoint = *point;
                }
            }
        } else {
            if (newFlag) {
                continue;
            } else {
                if (selectedPoints[selectedPointNum - 1] != iPoint - 1) {
                    selectedPoints[selectedPointNum] = (u16)(iPoint - 1);
                    selectedPointNum++;

                    if (selectedPointNum >= maxPointCount) {
                        break;
                    }
                }

                selectedPoints[selectedPointNum] = (u16)iPoint;
                selectedPointNum++;

                newFlag = TRUE;

                strokeCount++;
                if (strokeCount >= maxStrokeCount) {
                    iPoint++;
                    break;
                }
            }
        }
    }

    *pSelectedPointNum = selectedPointNum;

    if (!newFlag) {
        if (selectedPointNum > 0 && selectedPoints[selectedPointNum - 1] != iPoint - 1
            && selectedPointNum < maxPointCount) {
            selectedPoints[*pSelectedPointNum] = (u16)(iPoint - 1);
            (*pSelectedPointNum)++;
        }

        (void)PRCi_TerminateStrokes(selectedPoints, pSelectedPointNum, maxPointCount, strokes);
    }

    return (*pSelectedPointNum > 0);
}

BOOL PRC_ResampleStrokes_Recursive (u16 * selectedPoints, int * pSelectedPointNum, int maxPointCount, int maxStrokeCount, const PRCStrokes * strokes, int threshold, void * buffer)
{
    u16 beginIndex, endIndex;
    int stackSize;
    int stackTop, stackTail;
    int strokeCount;
    int selectedPointNum;
    int size;
    PRCPoint * inputPoints;
    u16 * stackP1;
    u16 * stackP2;
    int squaredThreshold;

    stackP1 = (u16 *)buffer;
    stackP2 = (u16 *)buffer + maxPointCount;

    squaredThreshold = threshold * threshold;

    beginIndex = 0;
    endIndex = 0;
    strokeCount = 0;
    selectedPointNum = 0;

    inputPoints = strokes->points;
    size = strokes->size;

    while (1) {
        if (selectedPointNum + 3 > maxPointCount || strokeCount > maxStrokeCount) {

            break;
        }

        while (endIndex < size && PRC_IsPenUpMarker(&inputPoints[endIndex])) {
            endIndex++;
        }

        beginIndex = endIndex;
        if (beginIndex >= size) {
            break;
        }

        while (endIndex < size && !PRC_IsPenUpMarker(&inputPoints[endIndex])) {
            endIndex++;
        }

        if (endIndex < size) {
            selectedPoints[selectedPointNum] = endIndex;
            selectedPointNum++;
        } else {
            selectedPoints[selectedPointNum] = (u16) - 1;
            selectedPointNum++;
        }

        SDK_ASSERT(endIndex > 0);

        selectedPoints[selectedPointNum] = beginIndex;
        selectedPointNum++;
        selectedPoints[selectedPointNum] = (u16)(endIndex - 1);
        selectedPointNum++;

        strokeCount++;

        if (selectedPointNum >= maxPointCount) {
            break;
        }

        if (endIndex - beginIndex <= 2)
            continue;

        stackP1[0] = beginIndex;
        stackP2[0] = (u16)(endIndex - 1);
        stackSize = 1;
        stackTop = 0;
        stackTail = 1;

        while (stackSize > 0) {
            u16 p1, p2;
            int x1, y1, x2, y2, xDir, yDir, offs;
            int lastX, lastY;
            int i;
            int maxDist;
            u16 maxP;

            p1 = stackP1[stackTop];
            p2 = stackP2[stackTop];
            stackTop++;

            if (stackTop >= maxPointCount) {
                stackTop = 0;
            }

            stackSize--;

            if (p2 - p1 <= 1)
                continue;

            x1 = inputPoints[p1].x;
            y1 = inputPoints[p1].y;
            x2 = inputPoints[p2].x;
            y2 = inputPoints[p2].y;

            xDir = x2 - x1;
            yDir = y2 - y1;
            offs = -(x1 * y2 - x2 * y1);

            maxDist = -1;
            maxP = (u16) - 1;
            lastX = -1;
            lastY = -1;

            for (i = p1 + 1; i < p2; i++) {
                int dist;
                int x, y;
                x = inputPoints[i].x;
                y = inputPoints[i].y;

                if (lastX == x && lastY == y)
                    continue;
                lastX = x;
                lastY = y;

                dist = x * yDir - y * xDir + offs;
                if (dist < 0) {
                    dist = -dist;
                }

                if (maxDist < dist) {
                    maxP = (u16)i;
                    maxDist = dist;
                }
            }

            if (maxDist * maxDist >= (xDir * xDir + yDir * yDir) * squaredThreshold) {
                selectedPoints[selectedPointNum] = maxP;
                selectedPointNum++;
                stackP1[stackTail] = maxP;
                stackP2[stackTail] = p2;
                stackTail++;
                if (stackTail >= maxPointCount) {
                    stackTail = 0;
                }
                stackSize++;
                stackP1[stackTail] = p1;
                stackP2[stackTail] = maxP;
                stackTail++;
                if (stackTail >= maxPointCount) {
                    stackTail = 0;
                }
                stackSize++;
                SDK_ASSERT(stackSize <= maxPointCount);
                if (selectedPointNum >= maxPointCount) {

                    break;
                }
            }
        }
    }

    *pSelectedPointNum = selectedPointNum;

    {
        int i, j;
        for (i = 0; i < selectedPointNum - 1; i++) {
            for (j = i + 1; j < selectedPointNum; j++) {
                if (selectedPoints[i] > selectedPoints[j]) {
                    u16 tmp;
                    tmp = selectedPoints[i];
                    selectedPoints[i] = selectedPoints[j];
                    selectedPoints[j] = tmp;
                }
            }
        }
    }

    return (*pSelectedPointNum > 0);
}
