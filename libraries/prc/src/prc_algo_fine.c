#include <nitro.h>
#include <nitro/prc/algo_fine.h>

static inline u32 PRCi_GetPatternLocalBufferSize_Fine(void);
static void PRCi_CalcStrokeDistance_Fine(fx32 * score, fx32 * weight, const PRCiPatternData_Common * inputData, const PRCiPatternData_Common * protoData, int normalizeSize, int lengthFilterThreshold, int lengthFilterRatio, int iStroke, void * buffer);

u32 PRC_GetRecognitionBufferSizeEx_Fine (int maxPointCount, int maxStrokeCount, const PRCPrototypeDB_Fine * protoDB, const PRCRecognizeParam_Fine * param)
{
    u32 size = 0;

    (void)maxStrokeCount;
    (void)param;

    if (maxPointCount < protoDB->maxPointCount) {
        maxPointCount = protoDB->maxPointCount;
    }

    size += PRCi_ARRAY_SIZE(int, maxPointCount * maxPointCount, );
    size += PRCi_ARRAY_SIZE(int, maxPointCount * maxPointCount, );
    size += PRCi_ARRAY_SIZE(int, maxPointCount * maxPointCount, );
    size += PRCi_ARRAY_SIZE(int, ((maxPointCount + 1) * (maxPointCount + 1)), );

    return size;
}

int PRC_GetRecognizedEntriesEx_Fine (PRCPrototypeEntry ** resultEntries, fx32 * resultScores, int numRanking, void * buffer, const PRCInputPattern_Fine * input, const PRCPrototypeDB_Fine * protoDB, u32 kindMask, const PRCRecognizeParam_Fine * param)
{
    int i;
    const PRCiPatternData_Common * inputData;
    int numCompared;
    int normalizeSize;
    int lengthFilterThreshold, lengthFilterRatio;

    SDK_ASSERT(resultEntries);
    SDK_ASSERT(resultScores);
    SDK_ASSERT(input);
    SDK_ASSERT(protoDB);
    SDK_ASSERT(numRanking > 0);

    for (i = 0; i < numRanking; i++) {
        resultEntries[i] = NULL;
        resultScores[i] = 0;
    }

    normalizeSize = protoDB->normalizeSize;
    if (normalizeSize < input->normalizeSize) {
        normalizeSize = input->normalizeSize;
    }

    if (param == NULL) {
        lengthFilterThreshold = PRC_LENGTH_FILTER_DEFAULT_THRESHOLD_FINE;
        lengthFilterRatio = PRC_LENGTH_FILTER_DEFAULT_RATIO_FINE;
    } else {
        lengthFilterThreshold = param->lengthFilterThreshold;
        lengthFilterRatio = param->lengthFilterRatio;
    }

    inputData = &input->data;
    numCompared = 0;

    {
        const PRCiPrototypeEntry_Common * proto;
        int iPattern;

        proto = protoDB->patterns;

        for (iPattern = 0; iPattern < protoDB->patternCount; iPattern++, proto++) {
            const PRCiPatternData_Common * protoData;
            int iStroke;
            fx32 wholeScore, wholeWeight;

            if (!proto->entry->enabled || !(proto->entry->kind & kindMask))
                continue;

            protoData = &proto->data;

            if (inputData->strokeCount != protoData->strokeCount)
                continue;

            numCompared++;
            wholeScore = 0;
            wholeWeight = 0;

            for (iStroke = 0; iStroke < inputData->strokeCount; iStroke++) {
                fx32 score, weight;

                score = 0;

                PRCi_CalcStrokeDistance_Fine(&score, &weight, inputData, protoData, normalizeSize,
                                             lengthFilterThreshold, lengthFilterRatio, iStroke,
                                             buffer);

                wholeScore += score;
                wholeWeight += weight;
            }

            wholeScore = FX_Div(wholeScore * 4, wholeWeight * normalizeSize);

            if (wholeScore < 0)
                wholeScore = 0;
            if (wholeScore >= FX32_ONE)
                wholeScore = FX32_ONE;
            if (proto->entry->correction != 0) {
                wholeScore = FX_Mul(wholeScore, FX32_ONE - proto->entry->correction)
                             + proto->entry->correction;
            }

            if (resultScores[numRanking - 1] < wholeScore) {
                resultScores[numRanking - 1] = wholeScore;
                resultEntries[numRanking - 1] = (PRCPrototypeEntry *)proto->entry;
                for (i = numRanking - 2; i >= 0; i--) {
                    if (resultScores[i] < resultScores[i + 1]) {
                        fx32 tmpScore;
                        PRCPrototypeEntry * tmpEntry;
                        tmpScore = resultScores[i];
                        resultScores[i] = resultScores[i + 1];
                        resultScores[i + 1] = tmpScore;
                        tmpEntry = resultEntries[i];
                        resultEntries[i] = resultEntries[i + 1];
                        resultEntries[i + 1] = tmpEntry;
                    }
                }
            }
        }
    }

    return numCompared;
}

#define PRCi_SINGLE_ANGLE_SCORE 128

static inline int CityBlockDistance (const PRCPoint * p1, const PRCPoint * p2)
{
    int x = p1->x - p2->x;
    int y = p1->y - p2->y;

    if (x < 0)
        x = -x;

    if (y < 0)
        y = -y;

    return (x + y);
}

#define PRCi_ABS(x) (((x) >= 0)?(x):-(x))
#define PRCi_ANGLE_SCORE(input, proto) ((32768 - PRCi_ABS((s16)(protoAngles[(proto)] - inputAngles[(input)]))) / 128)

static void PRCi_CalcStrokeDistance_Fine (fx32 * score,
                              fx32 * weight,
                              const PRCiPatternData_Common * inputData,
                              const PRCiPatternData_Common * protoData,
                              int normalizeSize,
                              int lengthFilterThreshold,
                              int lengthFilterRatio, int iStroke, void * buffer)
{
#define nMatches_(x, y) (*(nMatches + (x) * maxPointCount + (y)))
#define sumScore_(x, y) (*(sumScore + (x) * maxPointCount + (y)))
#define direction_(x, y) (*(direction + (x) * maxPointCount + (y)))
#define angleScores_(x, y) (*(angleScores + (x) * (maxPointCount + 1) + (y)))

    int iInput, iProto;
    int * nMatches;
    int * sumScore;
    int * direction;
    int * angleScores;

    int protoStrokeIndex, inputStrokeIndex;
    int protoSize, inputSize;
    const PRCPoint * protoPoints;
    const PRCPoint * inputPoints;
    const u16 * protoAngles;
    const u16 * inputAngles;
    const fx16 * protoRatios;
    const fx16 * inputRatios;
    const fx32 * protoLengths;
    const fx32 * inputLengths;
    fx32 protoStrokeLength, inputStrokeLength;
    int maxPointCount;
    int doubleWidth;

    maxPointCount = inputData->pointCount;
    if (maxPointCount < protoData->pointCount)
        maxPointCount = protoData->pointCount;

#ifdef SDK_DEBUG
    {
        int width, tmp;

        width = inputData->wholeBoundingBox.x2 - inputData->wholeBoundingBox.x1;
        tmp = inputData->wholeBoundingBox.y2 - inputData->wholeBoundingBox.y1;
        if (tmp > width)
            width = tmp;
        tmp = protoData->wholeBoundingBox.x2 - protoData->wholeBoundingBox.x1;
        if (tmp > width)
            width = tmp;
        tmp = protoData->wholeBoundingBox.y2 - protoData->wholeBoundingBox.y1;
        if (tmp > width)
            width = tmp;

        width++;
        if (width > normalizeSize) {
            OS_Warning
                ("too small normalizeSize. PRCPrototypeList.normalizeSize seems smaller than actual data.");
        }
    }
#endif
    doubleWidth = normalizeSize * 2;

    {
        int addr;
        addr = 0;
        PRCi_ALLOC_ARRAY(nMatches, int, maxPointCount * maxPointCount, buffer, addr);
        PRCi_ALLOC_ARRAY(sumScore, int, maxPointCount * maxPointCount, buffer, addr);
        PRCi_ALLOC_ARRAY(direction, int, maxPointCount * maxPointCount, buffer, addr);
        PRCi_ALLOC_ARRAY(angleScores,
                         int, ((maxPointCount + 1) * (maxPointCount + 1)), buffer, addr);
    }

    protoStrokeIndex = protoData->strokes[iStroke];
    inputStrokeIndex = inputData->strokes[iStroke];
    protoSize = protoData->strokeSizes[iStroke];
    inputSize = inputData->strokeSizes[iStroke];
    protoPoints = &protoData->pointArray[protoStrokeIndex];
    inputPoints = &inputData->pointArray[inputStrokeIndex];
    protoAngles = &protoData->lineSegmentAngleArray[protoStrokeIndex];
    inputAngles = &inputData->lineSegmentAngleArray[inputStrokeIndex];
    protoRatios = &protoData->lineSegmentRatioToStrokeArray[protoStrokeIndex];
    inputRatios = &inputData->lineSegmentRatioToStrokeArray[inputStrokeIndex];
    protoLengths = &protoData->lineSegmentLengthArray[protoStrokeIndex];
    inputLengths = &inputData->lineSegmentLengthArray[inputStrokeIndex];
    protoStrokeLength = protoData->strokeLengths[iStroke];
    inputStrokeLength = inputData->strokeLengths[iStroke];

    *weight = FX32_ONE;
    *score = 0;

    if (protoSize == 0 || inputSize == 0)
        return;

    if (inputStrokeLength > lengthFilterThreshold || protoStrokeLength > lengthFilterThreshold) {
        if (inputStrokeLength * lengthFilterRatio < protoStrokeLength
            || protoStrokeLength * lengthFilterRatio < inputStrokeLength) {
#ifdef PRC_DEBUG_RECOGNIZE_DEEPLY
            OS_Printf("Skipped because of length filter %d <=> %d\n", FX_Whole(inputStrokeLength),
                      FX_Whole(protoStrokeLength));
#endif
            return;
        }
    }

    if (protoSize == 1 || inputSize == 1) {
        for (iInput = 0; iInput < inputSize; iInput++) {
            for (iProto = 0; iProto < protoSize; iProto++) {
                angleScores_(iInput, iProto) = PRCi_SINGLE_ANGLE_SCORE;
            }
        }
    } else {
        angleScores_(0, 0) = PRCi_ANGLE_SCORE(1, 1);
        angleScores_(inputSize, 0) = PRCi_ANGLE_SCORE(inputSize - 1, 1);
        angleScores_(0, protoSize) = PRCi_ANGLE_SCORE(1, protoSize - 1);
        angleScores_(inputSize, protoSize) = PRCi_ANGLE_SCORE(inputSize - 1, protoSize - 1);

        for (iInput = 1; iInput < inputSize; iInput++) {
            angleScores_(iInput, 0) = PRCi_ANGLE_SCORE(iInput, 1);
            angleScores_(iInput, protoSize) = PRCi_ANGLE_SCORE(iInput, protoSize - 1);
        }
        for (iProto = 1; iProto < protoSize; iProto++) {
            angleScores_(0, iProto) = PRCi_ANGLE_SCORE(1, iProto);
            angleScores_(inputSize, iProto) = PRCi_ANGLE_SCORE(inputSize - 1, iProto);
        }

        for (iInput = 1; iInput < inputSize; iInput++) {
            for (iProto = 1; iProto < protoSize; iProto++) {
                angleScores_(iInput, iProto) = PRCi_ANGLE_SCORE(iInput, iProto);
            }
        }
    }

#if 1
    sumScore_(0, 0) =
        (doubleWidth - CityBlockDistance(&inputPoints[0], &protoPoints[0])) * angleScores_(0,
                                                                                           0) * 2;
    nMatches_(0, 0) = 1;
    for (iInput = 1; iInput < inputSize; iInput++) {
        sumScore_(iInput, 0) =
            (doubleWidth -
             CityBlockDistance(&inputPoints[iInput], &protoPoints[0])) * (angleScores_(iInput,
                                                                                       0) +
                                                                          angleScores_(iInput + 1,
                                                                                       1)) +
            sumScore_(iInput - 1, 0);
        nMatches_(iInput, 0) = nMatches_(iInput - 1, 0) + 1;
        direction_(iInput, 0) = 2;
    }
    for (iProto = 1; iProto < protoSize; iProto++) {
        sumScore_(0, iProto) =
            (doubleWidth -
             CityBlockDistance(&inputPoints[0], &protoPoints[iProto])) * (angleScores_(0,
                                                                                       iProto) +
                                                                          angleScores_(1,
                                                                                       iProto +
                                                                                       1)) +
            sumScore_(0, iProto - 1);
        nMatches_(0, iProto) = nMatches_(0, iProto - 1) + 1;
        direction_(0, iProto) = 1;
    }

    for (iInput = 1; iInput < inputSize; iInput++) {
        for (iProto = 1; iProto < protoSize; iProto++) {
            int sum, n, localScore;
            int sumMax, nMax, dirMax;

            localScore =
                (doubleWidth
                 - CityBlockDistance(&inputPoints[iInput], &protoPoints[iProto]))
                * (angleScores_(iInput, iProto) + angleScores_(iInput + 1, iProto + 1));

            dirMax = 0;
            sumMax = localScore + sumScore_(iInput - 1, iProto - 1);
            nMax = nMatches_(iInput - 1, iProto - 1) + 1;

            sum = localScore + sumScore_(iInput, iProto - 1);
            n = nMatches_(iInput, iProto - 1) + 1;
            if (sum * nMax > sumMax * n) {
                sumMax = sum;
                nMax = n;
                dirMax = 1;
            }

            sum = localScore + sumScore_(iInput - 1, iProto);
            n = nMatches_(iInput - 1, iProto) + 1;
            if (sum * nMax > sumMax * n) {
                sumMax = sum;
                nMax = n;
                dirMax = 2;
            }

            sumScore_(iInput, iProto) = sumMax;
            nMatches_(iInput, iProto) = nMax;
            direction_(iInput, iProto) = dirMax;
        }
    }
#elif
    {
        int beamThreshold, beamN, beamValue, beamLow, beamHigh, nextBeamHigh;
        int maxScore, maxScoreN;
        BOOL fInBeamLow;

        maxScore = 0;
        maxScoreN = 1;

        sumScore_(0, 0) =
            (doubleWidth - CityBlockDistance(&inputPoints[0], &protoPoints[0])) * angleScores_(0,
                                                                                               0) *
            2;
        nMatches_(0, 0) = 1;
        for (iProto = 1; iProto < protoSize; iProto++) {
            sumScore_(0, iProto) =
                (doubleWidth -
                 CityBlockDistance(&inputPoints[0], &protoPoints[iProto])) * (angleScores_(0,
                                                                                           iProto) +
                                                                              angleScores_(1,
                                                                                           iProto +
                                                                                           1)) +
                sumScore_(0, iProto - 1);
            nMatches_(0, iProto) = nMatches_(0, iProto - 1) + 1;
            direction_(0, iProto) = 1;
        }
        for (iInput = 1; iInput < inputSize; iInput++) {
            int sc, n;

            sc = (doubleWidth -
                  CityBlockDistance(&inputPoints[iInput], &protoPoints[0])) * (angleScores_(iInput,
                                                                                            0) +
                                                                               angleScores_(iInput +
                                                                                            1,
                                                                                            1)) +
                 sumScore_(iInput - 1, 0);
            n = nMatches_(iInput - 1, 0) + 1;
            if (sc * maxScoreN > maxScore * n) {
                maxScore = sc;
                maxScoreN = n;
            }
            sumScore_(iInput, 0) = sc;
            nMatches_(iInput, 0) = n;
            direction_(iInput, 0) = 2;
        }

        beamValue = normalizeSize * 2 * 512 / 8;
        beamLow = 1;
        nextBeamHigh = inputSize - 1;

        for (iProto = 1; iProto < protoSize; iProto++) {
            beamHigh = nextBeamHigh + 1;
            if (beamHigh < inputSize) {
                beamHigh++;
            }
            nextBeamHigh = 1;
            beamN = maxScoreN;
            beamThreshold = maxScore - beamValue * beamN;
            fInBeamLow = TRUE;
            maxScore = 0;
            maxScoreN = 1;

            if (beamLow > 1) {
                sumScore_(beamLow - 1, iProto) = 0;
                nMatches_(beamLow - 1, iProto) = nMatches_(beamLow - 1, iProto - 1) + 1;
                direction_(beamLow - 1, iProto) = 1;
            }
            for (iInput = beamLow; iInput < beamHigh; iInput++) {
                int sum, n, localScore;
                int sumMax, nMax, dirMax;

                localScore =
                    (doubleWidth
                     - CityBlockDistance(&inputPoints[iInput], &protoPoints[iProto]))
                    * (angleScores_(iInput, iProto) + angleScores_(iInput + 1, iProto + 1));

                dirMax = 0;
                sumMax = localScore + sumScore_(iInput - 1, iProto - 1);
                nMax = nMatches_(iInput - 1, iProto - 1) + 1;

                sum = localScore + sumScore_(iInput, iProto - 1);
                n = nMatches_(iInput, iProto - 1) + 1;
                if (sum * nMax > sumMax * n) {
                    sumMax = sum;
                    nMax = n;
                    dirMax = 1;
                }

                sum = localScore + sumScore_(iInput - 1, iProto);
                n = nMatches_(iInput - 1, iProto) + 1;
                if (sum * nMax > sumMax * n) {
                    sumMax = sum;
                    nMax = n;
                    dirMax = 2;
                }

                sumScore_(iInput, iProto) = sumMax;
                nMatches_(iInput, iProto) = nMax;
                direction_(iInput, iProto) = dirMax;

                if (sumMax * beamN < beamThreshold * nMax) {
                    if (fInBeamLow) {
                        beamLow = iInput;
                    }
                } else {
                    fInBeamLow = FALSE;
                    nextBeamHigh = iInput;
                }
                if (sumMax * maxScoreN > maxScore * nMax) {
                    maxScore = sumMax;
                    maxScoreN = nMax;
                }
            }
        }
        if (beamHigh < inputSize) {
            for (iInput = beamHigh; iInput < inputSize; iInput++) {
                sumScore_(iInput, protoSize - 1) = sumScore_(iInput - 1, protoSize - 1) + 1;
                nMatches_(iInput, protoSize - 1) = nMatches_(iInput - 1, protoSize - 1) + 1;
                direction_(iInput, protoSize - 1) = 2;
            }
        }
    }
#endif

#ifdef PRC_DEBUG_RECOGNIZE
    {
        int localScore, angleScore;
        iInput = inputSize - 1;
        iProto = protoSize - 1;
        while (!(iInput == 0 && iProto == 0)) {
            int dx, dy;
            dx = -1 + (direction_(iInput, iProto) & 1);
            dy = -1 + ((direction_(iInput, iProto) & 2) >> 1);
            localScore = sumScore_(iInput, iProto) - sumScore_(iInput + dx, iProto + dy);
            angleScore = angleScores_(iInput, iProto) + angleScores_(iInput + 1, iProto + 1);

            OS_Printf(" %2d <-> %2d : 0.%03d = 0.%03d * 0.%03d, average from begin: 0.%03d\n",
                      iInput, iProto, localScore / normalizeSize,
                      localScore * 512 / angleScore / normalizeSize, angleScore * 2,
                      sumScore_(iInput, iProto) / nMatches_(iInput, iProto) / normalizeSize);

            iInput += dx;
            iProto += dy;
        }
        localScore = sumScore_(iInput, iProto);
        angleScore = angleScores_(iInput, iProto) + angleScores_(iInput + 1, iProto + 1);
        OS_Printf(" %2d <-> %2d : 0.%03d = 0.%03d * 0.%03d\n", iInput, iProto,
                  localScore / normalizeSize, localScore * 512 / angleScore / normalizeSize,
                  angleScore * 2);
    }

    OS_Printf("total: %d, matches: %d\n", sumScore_(inputSize - 1, protoSize - 1),
              nMatches_(inputSize - 1, protoSize - 1));
#endif

    *score = sumScore_(inputSize - 1, protoSize - 1);
    *weight = nMatches_(inputSize - 1, protoSize - 1) << FX32_SHIFT;
}
