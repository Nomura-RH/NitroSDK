#include <nitro.h>
#include <nitro/prc/algo_standard.h>

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

static inline void GetMiddlePoint (PRCPoint * p, const PRCPoint * p1, const PRCPoint * p2)
{
    p->x = (s16)((p1->x + p2->x) / 2);
    p->y = (s16)((p1->y + p2->y) / 2);
}

u32 PRC_GetRecognitionBufferSizeEx_Standard (int maxPointCount, int maxStrokeCount, const PRCPrototypeDB_Standard * protoDB, const PRCRecognizeParam_Standard * param)
{
    (void)maxPointCount;
    (void)maxStrokeCount;
    (void)protoDB;
    (void)param;

    return 1;
}

int PRC_GetRecognizedEntriesEx_Standard (PRCPrototypeEntry ** resultEntries, fx32 * resultScores, int numRanking, void * buffer, const PRCInputPattern_Standard * input, const PRCPrototypeDB_Standard * protoDB, u32 kindMask, const PRCRecognizeParam_Standard * param)
{
    int i;
    const PRCiPatternData_Common * inputData;
    int numCompared;
    int normalizeSize;
    int doubleWidth;

    (void)buffer;
    (void)param;

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
    doubleWidth = normalizeSize * 2;

    inputData = &input->data;
    numCompared = 0;

    {
        const PRCiPrototypeEntry_Common * proto;
        int iPattern;

        proto = protoDB->patterns;

        for (iPattern = 0; iPattern < protoDB->patternCount; iPattern++, proto++) {
            const PRCiPatternData_Common * protoData;
            int iStroke;
            fx32 wholeScore;
            fx32 wholeWeight;

            if (!proto->entry->enabled || !(proto->entry->kind & kindMask))
                continue;

            protoData = &proto->data;

            if (inputData->strokeCount != protoData->strokeCount)
                continue;

            wholeScore = 0;
            wholeWeight = 0;

            for (iStroke = 0; iStroke < inputData->strokeCount; iStroke++) {
                int iProto, iInput;
                int protoStrokeIndex, inputStrokeIndex;
                int protoSize, inputSize;
                const PRCPoint * protoPoints;
                const PRCPoint * inputPoints;
                const u16 * protoAngle;
                const u16 * inputAngle;
                const fx16 * protoRatio;
                const fx16 * inputRatio;

                fx16 protoNextRatio, inputNextRatio;
                fx32 strokeScore;
                fx16 strokeRatio;

                strokeScore = 0;

                protoStrokeIndex = protoData->strokes[iStroke];
                inputStrokeIndex = inputData->strokes[iStroke];
                protoSize = protoData->strokeSizes[iStroke];
                inputSize = inputData->strokeSizes[iStroke];
                protoPoints = &protoData->pointArray[protoStrokeIndex];
                inputPoints = &inputData->pointArray[inputStrokeIndex];
                protoAngle = &protoData->lineSegmentAngleArray[protoStrokeIndex];
                inputAngle = &inputData->lineSegmentAngleArray[inputStrokeIndex];
                protoRatio = &protoData->lineSegmentRatioToStrokeArray[protoStrokeIndex];
                inputRatio = &inputData->lineSegmentRatioToStrokeArray[inputStrokeIndex];

                SDK_ASSERT(protoSize >= 2);
                SDK_ASSERT(inputSize >= 2);

                iProto = iInput = 1;
                protoNextRatio = protoRatio[iProto];
                inputNextRatio = inputRatio[iInput];

                for (i = 0; i < protoSize + inputSize - 3; i++) {
                    int diff, score;
                    SDK_ASSERT(iProto < protoSize);
                    SDK_ASSERT(iInput < inputSize);
                    diff = (s16)(protoAngle[iProto] - inputAngle[iInput]);
                    if (diff < 0) {
                        diff = -diff;
                    }
                    score = ((32768 - diff) / 128);
                    if (protoNextRatio <= inputNextRatio) {

                        inputNextRatio -= protoNextRatio;
                        score *= (inputNextRatio < inputRatio[iInput] / 2)
                            ? (doubleWidth -
                               CityBlockDistance(&inputPoints[iInput],
                                                 &protoPoints[iProto])) : (doubleWidth -
                                                                           CityBlockDistance
                                                                               (&inputPoints
                                                                               [iInput - 1],
                                                                               &protoPoints[iProto]));
                        strokeScore += protoNextRatio * score;
                        iProto++;
                        protoNextRatio = protoRatio[iProto];

                    } else {

                        protoNextRatio -= inputNextRatio;
                        score *= (protoNextRatio < protoRatio[iProto] / 2)
                            ? (doubleWidth -
                               CityBlockDistance(&inputPoints[iInput],
                                                 &protoPoints[iProto])) : (doubleWidth -
                                                                           CityBlockDistance
                                                                               (&inputPoints[iInput],
                                                                               &protoPoints[iProto -
                                                                                            1]));
                        strokeScore += inputNextRatio * score;
                        iInput++;
                        inputNextRatio = inputRatio[iInput];

                    }
                }

                strokeRatio = protoData->strokeRatios[iStroke];
                if (strokeRatio < inputData->strokeRatios[iStroke]) {
                    strokeRatio = inputData->strokeRatios[iStroke];
                }

                wholeScore += FX_Mul(strokeScore, strokeRatio);
                wholeWeight += strokeRatio;
            }

            wholeScore = FX_Div(wholeScore, wholeWeight * doubleWidth);
            wholeScore /= 256;

            if (proto->entry->correction != 0) {
                wholeScore = FX_Mul(wholeScore, FX32_ONE - proto->entry->correction)
                             + proto->entry->correction;
            }

            if (wholeScore < 0)
                wholeScore = 0;
            if (wholeScore >= FX32_ONE)
                wholeScore = FX32_ONE;

            numCompared++;
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
