#include <nitro.h>
#include <nitro/prc/algo_light.h>

u32 PRC_GetRecognitionBufferSizeEx_Light (int maxPointCount, int maxStrokeCount, const PRCPrototypeDB_Light * protoDB, const PRCRecognizeParam_Light * param)
{
    (void)maxPointCount;
    (void)maxStrokeCount;
    (void)protoDB;
    (void)param;

    return 1;
}

int PRC_GetRecognizedEntriesEx_Light (PRCPrototypeEntry ** resultEntries, fx32 * resultScores, int numRanking, void * buffer, const PRCInputPattern_Light * input, const PRCPrototypeDB_Light * protoDB, u32 kindMask, const PRCRecognizeParam_Light * param)
{
    int i;
    const PRCiPatternData_Common * inputData;
    int numCompared;

    (void)buffer;
    (void)param;

    SDK_ASSERT(resultEntries);
    SDK_ASSERT(resultScores);
    SDK_ASSERT(input);
    SDK_ASSERT(protoDB);
    SDK_ASSERT(numRanking > 0);

    for (i = 0; i < numRanking; i++) {
        resultEntries[i] = NULL;
        resultScores[i] = FX32_ONE * 32768;
    }

    inputData = &input->data;
    numCompared = 0;

    {
        const PRCiPrototypeEntry_Common * proto;
        int iPattern;
        fx32 border;

        border = FX32_ONE;
        border *= 32768;

        proto = protoDB->patterns;

        for (iPattern = 0; iPattern < protoDB->patternCount; iPattern++, proto++) {
            const PRCiPatternData_Common * protoData;
            int iStroke;
            fx32 wholeScore;
            fx32 patternBorder;

            if (!proto->entry->enabled || !(proto->entry->kind & kindMask))
                continue;

            protoData = &proto->data;

            if (inputData->strokeCount != protoData->strokeCount)
                continue;

            wholeScore = 0;

            patternBorder = border;
            if (proto->entry->correction != 0) {

                if (proto->entry->correction < -FX32_ONE) {
                    patternBorder >>= 1;
                } else if (proto->entry->correction < 0)   {

                    patternBorder = FX_Mul(patternBorder, FX32_ONE + proto->entry->correction / 2);
                } else if (proto->entry->correction < FX32_ONE / 2)   {

                    patternBorder = FX_Mul(patternBorder, FX32_ONE + proto->entry->correction * 2);
                } else {
                    patternBorder = FX32_ONE * 32768;
                }
            }

            for (iStroke = 0; iStroke < inputData->strokeCount; iStroke++) {
                int iProto, iInput;
                int protoStrokeIndex, inputStrokeIndex;
                int protoSize, inputSize;
                const u16 * protoAngle;
                const u16 * inputAngle;
                const fx16 * protoRatio;
                const fx16 * inputRatio;
                fx16 protoNextRatio, inputNextRatio;
                fx32 score;
                fx32 localBorder;
                fx16 strokeRatio;
                int loopEnd;

                score = 0;

                protoStrokeIndex = protoData->strokes[iStroke];
                inputStrokeIndex = inputData->strokes[iStroke];
                protoSize = protoData->strokeSizes[iStroke];
                inputSize = inputData->strokeSizes[iStroke];
                protoAngle = &protoData->lineSegmentAngleArray[protoStrokeIndex];
                inputAngle = &inputData->lineSegmentAngleArray[inputStrokeIndex];
                protoRatio = &protoData->lineSegmentRatioToStrokeArray[protoStrokeIndex];
                inputRatio = &inputData->lineSegmentRatioToStrokeArray[inputStrokeIndex];

                strokeRatio = protoData->strokeRatios[iStroke];

                if (strokeRatio == FX32_ONE) {
                    localBorder = patternBorder;
                } else if (strokeRatio >= FX32_ONE / 2)   {
                    localBorder = patternBorder * 2;
                } else if (strokeRatio >= FX32_ONE / 3)   {
                    localBorder = patternBorder * 3;
                } else {
                    localBorder = FX32_ONE * 32768;
                }

                SDK_ASSERT(protoSize >= 2);
                SDK_ASSERT(inputSize >= 2);

                iProto = iInput = 1;
                protoNextRatio = protoRatio[iProto];
                inputNextRatio = inputRatio[iInput];
                loopEnd = protoSize + inputSize - 3;
                for (i = 0; i < loopEnd; i++) {
                    int diff;
                    SDK_ASSERT(iProto < protoSize);
                    SDK_ASSERT(iInput < inputSize);
                    diff = (s16)(protoAngle[iProto] - inputAngle[iInput]);
                    if (diff < 0) {
                        diff = -diff;
                    }
                    if (protoNextRatio <= inputNextRatio) {
                        score += protoNextRatio * diff;
                        iProto++;
                        inputNextRatio -= protoNextRatio;
                        protoNextRatio = protoRatio[iProto];
                    } else {
                        score += inputNextRatio * diff;
                        iInput++;
                        protoNextRatio -= inputNextRatio;
                        inputNextRatio = inputRatio[iInput];
                    }

                    if (score > localBorder) {

                        wholeScore = FX32_ONE * 32768;
                        goto quit_compare;
                    }
                }

                wholeScore += FX_Mul(score, strokeRatio);
            }

            if (proto->entry->correction != 0) {
                wholeScore = FX_Mul(wholeScore, FX32_ONE - proto->entry->correction);
            }
quit_compare:
            numCompared++;
            if (resultScores[numRanking - 1] > wholeScore) {
                resultScores[numRanking - 1] = wholeScore;
                resultEntries[numRanking - 1] = (PRCPrototypeEntry *)proto->entry;
                for (i = numRanking - 2; i >= 0; i--) {
                    if (resultScores[i] > resultScores[i + 1]) {
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

                border = resultScores[numRanking - 1];
            }
        }
    }

    {
        int iRank;
        fx32 score;
        for (iRank = 0; iRank < numRanking; iRank++) {
            if (resultEntries[iRank] == NULL)
                break;

            score = resultScores[iRank];
            score = FX32_ONE - (score / 32768);
            if (score < 0)
                score = 0;
            if (score >= FX32_ONE)
                score = FX32_ONE;

            resultScores[iRank] = score;
        }
    }

    return numCompared;
}
