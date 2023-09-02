#include <nitro.h>
#include <nitro/prc/algo_common.h>
#include <nitro/prc/resample.h>

static void PRCi_CountPrototypeList_Common(int * wholePointCount, int * wholeStrokeCount, int * patternCount, int * maxPointCount, int * maxStrokeCount, const PRCPrototypeList * prototypeList, u32 kindMask, BOOL ignoreDisabledEntries);
static u32 PRCi_GetPrototypeDBBufferSize_Common(int wholePointCount, int wholeStrokeCount, int patternCount);
static u32 PRCi_SetPrototypeListBufferInfo_Common(PRCiPrototypeListBufferInfo_Common * WAInfo, void * buffer, int wholePointCount, int wholeStrokeCount, int patternCount);
static BOOL PRCi_ExtractPrototypeList_Common(PRCPrototypeDB_Common * protoDB, u32 kindMask, BOOL ignoreDisabledEntries);
static u32 PRCi_GetInputPatternBufferSize_Common(int pointCount, int strokeCount);
static u32 PRCi_SetPatternBufferInfo_Common(PRCiPatternBufferInfo_Common * WAInfo, void * buffer, int pointCount, int strokeCount);
static BOOL PRCi_ExtractInputPattern_Common(PRCInputPattern_Common * pattern, const PRCStrokes * strokes, int maxPointCount, int maxStrokeCount, const PRCInputPatternParam_Common * param);
static void PRCi_CalcExtraValues_Common(PRCiPatternData_Common * data);
static void PRCi_GetPatternStrokes_Common(PRCStrokes * strokes, const PRCiPatternData_Common * data);

u32 PRC_GetPrototypeDBBufferSizeEx_Common (const PRCPrototypeList * prototypeList, u32 kindMask, BOOL ignoreDisabledEntries, const PRCPrototypeDBParam_Common * param)
{
	int wholePointCount, wholeStrokeCount, patternCount;
	int maxPointCount, maxStrokeCount;
	SDK_ASSERT(prototypeList);

	(void)param;
	PRCi_CountPrototypeList_Common(&wholePointCount, &wholeStrokeCount,
	                               &patternCount, &maxPointCount, &maxStrokeCount,
	                               prototypeList, kindMask, ignoreDisabledEntries);

	return PRCi_GetPrototypeDBBufferSize_Common(wholePointCount, wholeStrokeCount, patternCount);
}

BOOL PRC_InitPrototypeDBEx_Common (PRCPrototypeDB_Common * protoDB, void * buffer, const PRCPrototypeList * prototypeList, u32 kindMask, BOOL ignoreDisabledEntries, const PRCPrototypeDBParam_Common * param)
{
	PRCiPrototypeListBufferInfo_Common WAInfo;
	u32 bufferSize;
	SDK_ASSERT(protoDB);
	SDK_ASSERT(prototypeList);
	SDK_ASSERT(buffer);
	(void)param;

	PRCi_CountPrototypeList_Common(&protoDB->wholePointCount, &protoDB->wholeStrokeCount,
	                               &protoDB->patternCount, &protoDB->maxPointCount,
	                               &protoDB->maxStrokeCount, prototypeList, kindMask,
	                               ignoreDisabledEntries);

	bufferSize = PRCi_SetPrototypeListBufferInfo_Common(&WAInfo, buffer,
	                                                    protoDB->wholePointCount,
	                                                    protoDB->wholeStrokeCount,
	                                                    protoDB->patternCount);
#ifdef SDK_DEBUG
	SDK_ASSERTMSG((bufferSize ==
	               PRC_GetPrototypeDBBufferSizeEx_Common(prototypeList, kindMask,
	                                                     ignoreDisabledEntries, param)),
	              "Internal Error: bufferSize mismatch.");
#endif

	protoDB->patterns = WAInfo.patterns;
	protoDB->lineSegmentLengthArray = WAInfo.lineSegmentLengthArray;
	protoDB->lineSegmentRatioToStrokeArray = WAInfo.lineSegmentRatioToStrokeArray;

	protoDB->lineSegmentAngleArray = WAInfo.lineSegmentAngleArray;
	protoDB->strokeArray = WAInfo.strokeArray;
	protoDB->strokeSizeArray = WAInfo.strokeSizeArray;
	protoDB->strokeLengthArray = WAInfo.strokeLengthArray;
	protoDB->strokeRatioArray = WAInfo.strokeRatioArray;
	protoDB->strokeBoundingBoxArray = WAInfo.strokeBoundingBoxArray;

	protoDB->normalizeSize = prototypeList->normalizeSize;

	protoDB->prototypeList = prototypeList;
	protoDB->buffer = buffer;
	protoDB->bufferSize = bufferSize;

	return PRCi_ExtractPrototypeList_Common(protoDB, kindMask, ignoreDisabledEntries);
}

u32 PRC_GetInputPatternBufferSize_Common (int maxPointCount, int maxStrokeCount)
{
	return PRCi_GetInputPatternBufferSize_Common(maxPointCount, maxStrokeCount);
}

BOOL PRC_InitInputPatternEx_Common (PRCInputPattern_Common * pattern, void * buffer, const PRCStrokes * strokes, int maxPointCount, int maxStrokeCount, const PRCInputPatternParam_Common * param)
{
	PRCiPatternBufferInfo_Common WAInfo;
	PRCiPatternData_Common * data;
	u32 bufferSize;
	SDK_ASSERT(pattern);
	SDK_ASSERT(buffer);

	bufferSize = PRCi_SetPatternBufferInfo_Common(&WAInfo, buffer, maxPointCount, maxStrokeCount);

#ifdef SDK_DEBUG
	SDK_ASSERTMSG((bufferSize ==
	               PRCi_GetInputPatternBufferSize_Common(maxPointCount, maxStrokeCount)),
	              "Internal Error: bufferSize mismatch.");
#endif

	data = &pattern->data;
	data->pointArray = WAInfo.pointArray;
	data->strokes = WAInfo.strokes;
	data->strokeSizes = WAInfo.strokeSizes;
	data->lineSegmentLengthArray = WAInfo.lineSegmentLengthArray;
	data->lineSegmentRatioToStrokeArray = WAInfo.lineSegmentRatioToStrokeArray;

	data->lineSegmentAngleArray = WAInfo.lineSegmentAngleArray;
	data->strokeLengths = WAInfo.strokeLengths;
	data->strokeRatios = WAInfo.strokeRatios;
	data->strokeBoundingBoxes = WAInfo.strokeBoundingBoxes;

	pattern->buffer = buffer;
	pattern->bufferSize = bufferSize;

	return PRCi_ExtractInputPattern_Common(pattern, strokes, maxPointCount, maxStrokeCount, param);
}

void PRC_GetInputPatternStrokes_Common (PRCStrokes * strokes, const PRCInputPattern_Common * input)
{
	SDK_ASSERT(strokes);
	SDK_ASSERT(input);

	PRCi_GetPatternStrokes_Common(strokes, &input->data);
}

#define FMT_FX(f) ((f) >> FX32_SHIFT), (((f) & FX32_DEC_MASK) * 1000 / (1 << FX32_DEC_SIZE))
#define FMT_POINT(p) (p).x, (p).y
#define FMT_BB(bb) (bb).x1, (bb).y1, (bb).x2, (bb).y2

void PRCi_PrintPatternData_Common (PRCiPatternData_Common * data)
{
	int iStroke, iPoint;

	OS_Printf("  strokeCount: %d\n", data->strokeCount);
	OS_Printf("  pointCount:  %d\n", data->pointCount);
	OS_Printf("  wholeLength: %d.%03d\n", FMT_FX(data->wholeLength));
	OS_Printf("  wholeBoundingBox: (%d, %d) - (%d, %d)\n", FMT_BB(data->wholeBoundingBox));

	for (iStroke = 0; iStroke < data->strokeCount; iStroke++) {
		OS_Printf("  Stroke #%d\n", iStroke);
		OS_Printf("    index : %d\n", data->strokes[iStroke]);
		OS_Printf("    size  : %d\n", data->strokeSizes[iStroke]);
		OS_Printf("    length: %d.%03d\n", FMT_FX(data->strokeLengths[iStroke]));
		OS_Printf("    ratio : %d.%03d\n", FMT_FX(data->strokeRatios[iStroke]));
		OS_Printf("    strokeBoundingBoxes: (%d, %d) - (%d, %d)\n",
		          FMT_BB(data->strokeBoundingBoxes[iStroke]));
		for (iPoint = 0; iPoint < data->strokeSizes[iStroke]; iPoint++) {
			int index;

			index = data->strokes[iStroke] + iPoint;

			OS_Printf("      #%d: pointArray[%d] = (%d, %d)\n", iPoint, index,
			          FMT_POINT(data->pointArray[index]));

			OS_Printf("        length: %d.%03d, ratio: %d.%03d, angle: %d\n",
			          FMT_FX(data->lineSegmentLengthArray[index]),
			          FMT_FX(data->lineSegmentRatioToStrokeArray[index]),

			          data->lineSegmentAngleArray[index] * 360 / 65536);
		}
	}
}

static void PRCi_CountPrototypeList_Common (int * wholePointCount, int * wholeStrokeCount, int * patternCount, int * maxPointCount, int * maxStrokeCount, const PRCPrototypeList * prototypeList, u32 kindMask, BOOL ignoreDisabledEntries)
{
	int i;
	int pointCnt, strokeCnt, patternCnt;
	int maxPointCnt, maxStrokeCnt;
	const PRCPrototypeEntry * entry;

	pointCnt = strokeCnt = patternCnt = 0;
	maxPointCnt = maxStrokeCnt = 0;

	entry = prototypeList->entries;
	for (i = 0; i < prototypeList->entrySize; i++, entry++) {
		if (!entry->enabled && ignoreDisabledEntries)
			continue;
		if (entry->kind & kindMask) {
			pointCnt += entry->pointCount;
			strokeCnt += entry->strokeCount;
			patternCnt++;

			if (maxPointCnt < entry->pointCount) {
				maxPointCnt = entry->pointCount;
			}
			if (maxStrokeCnt < entry->strokeCount) {
				maxStrokeCnt = entry->strokeCount;
			}
		}
	}

	*wholePointCount = pointCnt;
	*wholeStrokeCount = strokeCnt;
	*patternCount = patternCnt;
	*maxPointCount = maxPointCnt;
	*maxStrokeCount = maxStrokeCnt;
}

static u32 PRCi_GetPrototypeDBBufferSize_Common (int wholePointCount, int wholeStrokeCount, int patternCount)
{
	u32 addr;

	addr = 0;

	addr += PRCi_ARRAY_SIZE(PRCiPrototypeEntry_Common, patternCount, );
	addr += PRCi_ARRAY_SIZE(fx32, wholePointCount + 1, );
	addr += PRCi_ARRAY_SIZE(int, wholeStrokeCount, );
	addr += PRCi_ARRAY_SIZE(int, wholeStrokeCount, );
	addr += PRCi_ARRAY_SIZE(fx32, wholeStrokeCount, );
	addr += PRCi_ARRAY_SIZE(PRCBoundingBox, wholeStrokeCount, );
	addr += PRCi_ARRAY_SIZE(fx16, wholePointCount + 1, );
	addr += PRCi_ARRAY_SIZE(u16, wholePointCount + 1, );
	addr += PRCi_ARRAY_SIZE(fx16, wholeStrokeCount, );

	return addr;
}

static u32 PRCi_SetPrototypeListBufferInfo_Common (PRCiPrototypeListBufferInfo_Common * WAInfo, void * buffer, int wholePointCount, int wholeStrokeCount, int patternCount)
{
	u32 addr;

	SDK_ALIGN4_ASSERT(buffer);

	addr = 0;

	PRCi_ALLOC_ARRAY(WAInfo->patterns, PRCiPrototypeEntry_Common, patternCount, buffer, addr);
	PRCi_ALLOC_ARRAY(WAInfo->lineSegmentLengthArray, fx32, wholePointCount + 1, buffer, addr);
	PRCi_ALLOC_ARRAY(WAInfo->strokeArray, int, wholeStrokeCount, buffer, addr);
	PRCi_ALLOC_ARRAY(WAInfo->strokeSizeArray, int, wholeStrokeCount, buffer, addr);
	PRCi_ALLOC_ARRAY(WAInfo->strokeLengthArray, fx32, wholeStrokeCount, buffer, addr);
	PRCi_ALLOC_ARRAY(WAInfo->strokeBoundingBoxArray, PRCBoundingBox, wholeStrokeCount, buffer, addr);
	PRCi_ALLOC_ARRAY(WAInfo->lineSegmentRatioToStrokeArray, fx16, wholePointCount + 1, buffer, addr);
	PRCi_ALLOC_ARRAY(WAInfo->lineSegmentAngleArray, u16, wholePointCount + 1, buffer, addr);
	PRCi_ALLOC_ARRAY(WAInfo->strokeRatioArray, fx16, wholeStrokeCount, buffer, addr);

	return addr;
}

static BOOL PRCi_ExtractPrototypeList_Common (PRCPrototypeDB_Common * protoDB, u32 kindMask, BOOL ignoreDisabledEntries)
{
	int iEntry;
	int pointCnt, strokeCnt, patternCnt;
	int entrySize;
	const PRCPrototypeEntry * entry;
	PRCiPrototypeEntry_Common * pattern;

	entry = protoDB->prototypeList->entries;
	entrySize = protoDB->prototypeList->entrySize;

	pointCnt = strokeCnt = patternCnt = 0;
	pattern = protoDB->patterns;

	for (iEntry = 0; iEntry < entrySize; iEntry++, entry++) {
		if (!entry->enabled && ignoreDisabledEntries)
			continue;

		if (entry->kind & kindMask) {
			PRCiPatternData_Common * data;

			pattern->entry = entry;
			data = &pattern->data;
			data->strokeCount = entry->strokeCount;
			data->pointCount = entry->pointCount;
			data->pointArray = &protoDB->prototypeList->pointArray[entry->pointIndex];
			data->strokes = &protoDB->strokeArray[strokeCnt];
			data->strokeSizes = &protoDB->strokeSizeArray[strokeCnt];
			data->strokeLengths = &protoDB->strokeLengthArray[strokeCnt];
			data->strokeRatios = &protoDB->strokeRatioArray[strokeCnt];
			data->strokeBoundingBoxes = &protoDB->strokeBoundingBoxArray[strokeCnt];
			data->lineSegmentLengthArray = &protoDB->lineSegmentLengthArray[pointCnt];
			data->lineSegmentRatioToStrokeArray = &protoDB->lineSegmentRatioToStrokeArray[pointCnt];

			data->lineSegmentAngleArray = &protoDB->lineSegmentAngleArray[pointCnt];

			PRCi_CalcExtraValues_Common((PRCiPatternData_Common *)data);

			pointCnt += entry->pointCount;
			strokeCnt += entry->strokeCount;
			patternCnt++;
			pattern++;
		}
	}
	SDK_ASSERT(patternCnt <= entrySize);

	return TRUE;
}

static u32 PRCi_GetInputPatternBufferSize_Common (int pointCount, int strokeCount)
{
	u32 addr;

	addr = 0;

	addr += PRCi_ARRAY_SIZE(PRCPoint, pointCount, );
	addr += PRCi_ARRAY_SIZE(int, strokeCount, );
	addr += PRCi_ARRAY_SIZE(int, strokeCount, );
	addr += PRCi_ARRAY_SIZE(fx32, pointCount + 1, );
	addr += PRCi_ARRAY_SIZE(fx32, strokeCount, );
	addr += PRCi_ARRAY_SIZE(PRCBoundingBox, strokeCount, );
	addr += PRCi_ARRAY_SIZE(fx16, pointCount + 1, );
	addr += PRCi_ARRAY_SIZE(u16, pointCount + 1, );
	addr += PRCi_ARRAY_SIZE(fx16, strokeCount, );

	return addr;
}

static u32 PRCi_SetPatternBufferInfo_Common (PRCiPatternBufferInfo_Common * WAInfo, void * buffer, int pointCount, int strokeCount)
{
	u32 addr;

	SDK_ALIGN4_ASSERT(buffer);

	addr = 0;

	PRCi_ALLOC_ARRAY(WAInfo->pointArray, PRCPoint, pointCount, buffer, addr);
	PRCi_ALLOC_ARRAY(WAInfo->strokes, int, strokeCount, buffer, addr);
	PRCi_ALLOC_ARRAY(WAInfo->strokeSizes, int, strokeCount, buffer, addr);
	PRCi_ALLOC_ARRAY(WAInfo->lineSegmentLengthArray, fx32, pointCount + 1, buffer, addr);
	PRCi_ALLOC_ARRAY(WAInfo->strokeLengths, fx32, strokeCount, buffer, addr);
	PRCi_ALLOC_ARRAY(WAInfo->strokeBoundingBoxes, PRCBoundingBox, strokeCount, buffer, addr);
	PRCi_ALLOC_ARRAY(WAInfo->lineSegmentRatioToStrokeArray, fx16, pointCount + 1, buffer, addr);
	PRCi_ALLOC_ARRAY(WAInfo->lineSegmentAngleArray, u16, pointCount + 1, buffer, addr);
	PRCi_ALLOC_ARRAY(WAInfo->strokeRatios, fx16, strokeCount, buffer, addr);

	return addr;
}

static BOOL PRCi_ExtractInputPattern_Common (PRCInputPattern_Common * pattern, const PRCStrokes * strokes, int maxPointCount, int maxStrokeCount, const PRCInputPatternParam_Common * param)
{
	PRCiPatternData_Common * data;
	u16 iPoint, strokeCount, wholePointCount;
	int wx, wy, w, dx, dy;
	u16 * selectedPoints;
	int selectedPointNum;
	int normalizeSize;
	PRCResampleMethod resampleMethod;
	int resampleThreshold;

	if (param == NULL) {
		normalizeSize = 0;
		resampleMethod = PRC_RESAMPLE_DEFAULT_METHOD_COMMON;
		resampleThreshold = PRC_RESAMPLE_DEFAULT_THRESHOLD_COMMON;
	} else {
		normalizeSize = param->normalizeSize;
		resampleMethod = param->resampleMethod;
		resampleThreshold = param->resampleThreshold;
	}

	if (normalizeSize > 0) {
		PRCBoundingBox boundingBox;
		PRCPoint * point;
		int size;

		boundingBox.x1 = PRC_LARGE_ENOUGH_X;
		boundingBox.y1 = PRC_LARGE_ENOUGH_Y;
		boundingBox.x2 = PRC_SMALL_ENOUGH_X;
		boundingBox.y2 = PRC_SMALL_ENOUGH_Y;

		point = strokes->points;
		size = strokes->size;

		for (iPoint = 0; iPoint < size; iPoint++, point++) {
			if (PRC_IsPenUpMarker(point))
				continue;
			if (point->x < boundingBox.x1)
				boundingBox.x1 = point->x;
			if (point->x > boundingBox.x2)
				boundingBox.x2 = point->x;
			if (point->y < boundingBox.y1)
				boundingBox.y1 = point->y;
			if (point->y > boundingBox.y2)
				boundingBox.y2 = point->y;
		}

		wx = boundingBox.x2 - boundingBox.x1;
		wy = boundingBox.y2 - boundingBox.y1;
		w = (wx >= wy) ? wx : wy;
		dx = (boundingBox.x1 + boundingBox.x2) / 2 - w / 2;
		dy = (boundingBox.y1 + boundingBox.y2) / 2 - w / 2;
	}

	data = &pattern->data;
	selectedPoints = (u16 *)data->lineSegmentAngleArray;
	selectedPointNum = 0;

	if (resampleMethod == PRC_RESAMPLE_METHOD_NONE) {
		if (!PRC_ResampleStrokes_None
		        (selectedPoints, &selectedPointNum, maxPointCount, maxStrokeCount, strokes, 0,
		        (void *)data->pointArray)) {
			selectedPointNum = 0;
		}
	} else {
		BOOL result = FALSE;
		int threshold;

		threshold = resampleThreshold;
		if (normalizeSize > 0) {

			if (resampleMethod == PRC_RESAMPLE_METHOD_DISTANCE
			    || resampleMethod == PRC_RESAMPLE_METHOD_RECURSIVE) {
				if (threshold > 0) {

					threshold = threshold * w / normalizeSize;
					if (threshold == 0)
						threshold = 1;
				}
			}
		}

		switch (resampleMethod) {
		case PRC_RESAMPLE_METHOD_DISTANCE:
			result =
				PRC_ResampleStrokes_Distance(selectedPoints, &selectedPointNum, maxPointCount,
				                             maxStrokeCount, strokes, threshold,
				                             (void *)data->pointArray);
			break;
		case PRC_RESAMPLE_METHOD_ANGLE:
			result =
				PRC_ResampleStrokes_Angle(selectedPoints, &selectedPointNum, maxPointCount,
				                          maxStrokeCount, strokes, threshold,
				                          (void *)data->pointArray);
			break;
		case PRC_RESAMPLE_METHOD_RECURSIVE:
			result =
				PRC_ResampleStrokes_Recursive(selectedPoints, &selectedPointNum, maxPointCount,
				                              maxStrokeCount, strokes, threshold,
				                              (void *)data->pointArray);
			break;
		default:
			OS_Warning("invalid resample method type");
			break;
		}

		if (!result) {
			selectedPointNum = 0;
		}
	}

	SDK_ASSERT(selectedPointNum <= maxPointCount);

	wholePointCount = 0;
	strokeCount = 0;

	if (selectedPointNum > 0) {
		int i;
		BOOL newFlag;
		PRCPoint * dstPoint;
		PRCPoint prevPoint;
		PRCPoint * point;
		PRCPoint * inputPoints;
		int pointCount;

		inputPoints = strokes->points;

		pointCount = 0;
		dstPoint = (PRCPoint *)data->pointArray;
		prevPoint.x = -1;
		prevPoint.y = -1;
		newFlag = TRUE;

		if (normalizeSize > 0) {
			u32 scale;

			scale = (((u32)normalizeSize) << 16) / (w + 1);

			for (i = 0; i < selectedPointNum; i++) {
				s16 x, y;
				point = &inputPoints[selectedPoints[i]];
				if (selectedPoints[i] == (u16) - 1 || PRC_IsPenUpMarker(point)) {

					if (pointCount >= 2) {
						dstPoint->x = prevPoint.x = PRC_PEN_UP_MARKER_X;
						dstPoint->y = prevPoint.y = PRC_PEN_UP_MARKER_Y;
						dstPoint++;
						wholePointCount++;
						strokeCount++;
					} else {

						int i;
						for (i = 0; i < pointCount; i++) {

							dstPoint--;
							wholePointCount--;
						}
					}
					newFlag = TRUE;
					pointCount = 0;
				} else {

					x = (s16)(((u32)(point->x - dx) * scale) >> 16);
					y = (s16)(((u32)(point->y - dy) * scale) >> 16);

					if (prevPoint.x != x || prevPoint.y != y) {
						dstPoint->x = prevPoint.x = x;
						dstPoint->y = prevPoint.y = y;
						dstPoint++;
						wholePointCount++;
						pointCount++;
					}
					newFlag = FALSE;
				}
			}
		} else {
			for (i = 0; i < selectedPointNum; i++) {
				point = &inputPoints[selectedPoints[i]];
				if (selectedPoints[i] == (u16) - 1 || PRC_IsPenUpMarker(point)) {

					if (pointCount >= 2) {
						dstPoint->x = prevPoint.x = PRC_PEN_UP_MARKER_X;
						dstPoint->y = prevPoint.y = PRC_PEN_UP_MARKER_Y;
						dstPoint++;
						wholePointCount++;
						strokeCount++;
					} else {
						int i;
						for (i = 0; i < pointCount; i++) {

							dstPoint--;
							wholePointCount--;
						}
					}
					newFlag = TRUE;
					pointCount = 0;
				} else {
					if (prevPoint.x != point->x || prevPoint.y != point->y) {
						*dstPoint = prevPoint = *point;
						dstPoint++;
						wholePointCount++;
						pointCount++;
					}
					newFlag = FALSE;
				}
			}
		}
		SDK_ASSERTMSG(newFlag, "Internal error: non-terminated resampled stroke.");
	}

	data->pointCount = wholePointCount;
	data->strokeCount = strokeCount;

	if (wholePointCount == 0) {
		pattern->normalizeSize = (normalizeSize > 0) ? normalizeSize : 1;
		return FALSE;
	}

	PRCi_CalcExtraValues_Common((PRCiPatternData_Common *)data);

	if (normalizeSize > 0) {
		pattern->normalizeSize = normalizeSize;
	} else {
		PRCBoundingBox * box = &pattern->data.wholeBoundingBox;
		pattern->normalizeSize = (box->x2 - box->x1);
		if (pattern->normalizeSize < (box->y2 - box->y1)) {
			pattern->normalizeSize = box->y2 - box->y1;
		}
	}

	return TRUE;
}

void PRCi_CalcExtraValues_Common (PRCiPatternData_Common * data)
{
	int iStroke, iPoint;
	BOOL newFlag;
	int size;
	fx32 strokeLength;
	int strokeSize;
	PRCBoundingBox boundingBox;
	PRCPoint prevPoint;
	const PRCPoint * point;

	data->wholeLength = 0;
	data->wholeBoundingBox.x1 = PRC_LARGE_ENOUGH_X;
	data->wholeBoundingBox.y1 = PRC_LARGE_ENOUGH_Y;
	data->wholeBoundingBox.x2 = PRC_SMALL_ENOUGH_X;
	data->wholeBoundingBox.y2 = PRC_SMALL_ENOUGH_Y;

	size = data->pointCount;

	{
		point = data->pointArray;

		if (size >= 2 && !PRC_IsPenUpMarker(point + 1)) {
			prevPoint.x = (s16)(point[0].x * 2 - point[1].x);
			prevPoint.y = (s16)(point[0].y * 2 - point[1].y);
		} else {
			prevPoint = point[0];
		}

		iStroke = 0;
		newFlag = TRUE;
		for (iPoint = 0; iPoint < size; iPoint++) {
			fx32 segmentLength;
			u16 segmentAngle;

			if (!PRC_IsPenUpMarker(point)) {
				if (newFlag) {
					SDK_ASSERT(iStroke < data->strokeCount);

					data->strokes[iStroke] = iPoint;
					strokeLength = 0;
					strokeSize = 0;

					boundingBox.x1 = point->x;
					boundingBox.y1 = point->y;
					boundingBox.x2 = point->x;
					boundingBox.y2 = point->y;

					segmentLength = 0;
					segmentAngle = PRCi_CalcAngle(&prevPoint, point);
				} else {
					if (point->x < boundingBox.x1)
						boundingBox.x1 = point->x;
					if (point->x > boundingBox.x2)
						boundingBox.x2 = point->x;
					if (point->y < boundingBox.y1)
						boundingBox.y1 = point->y;
					if (point->y > boundingBox.y2)
						boundingBox.y2 = point->y;

					segmentLength = PRCi_CalcDistance(&prevPoint, point);
					if (segmentLength == 0) {
						segmentLength = PRC_TINY_LENGTH;
					}
					segmentAngle = PRCi_CalcAngle(&prevPoint, point);
				}

				strokeLength += segmentLength;
				strokeSize++;

				prevPoint = *point;
				newFlag = FALSE;
			} else {
				SDK_ASSERTMSG(!newFlag, "the pattern contains a zero-length stroke.");

				if (strokeLength == 0) {
					OS_Warning("the pattern contains a zero-Euclid-length stroke.");
					strokeLength = PRC_TINY_LENGTH;
				}

				data->strokeLengths[iStroke] = strokeLength;
				data->strokeSizes[iStroke] = strokeSize;

				if (iPoint + 1 < size) {
					segmentLength = PRCi_CalcDistance(&prevPoint, point + 1);
					if (segmentLength == 0) {
						segmentLength = PRC_TINY_LENGTH;
					}
					segmentAngle = PRCi_CalcAngle(&prevPoint, point + 1);
				} else {
					segmentLength = 0;
					if (iPoint >= 2 && PRC_IsPenUpMarker(point - 2) && PRC_IsPenUpMarker(point - 1)) {
						segmentAngle = PRCi_CalcAngle(point - 2, point - 1);
					} else {
						segmentAngle = 0;
					}
				}
				data->strokeBoundingBoxes[iStroke] = boundingBox;

				if (boundingBox.x1 < data->wholeBoundingBox.x1)
					data->wholeBoundingBox.x1 = boundingBox.x1;
				if (boundingBox.x2 > data->wholeBoundingBox.x2)
					data->wholeBoundingBox.x2 = boundingBox.x2;
				if (boundingBox.y1 < data->wholeBoundingBox.y1)
					data->wholeBoundingBox.y1 = boundingBox.y1;
				if (boundingBox.y2 > data->wholeBoundingBox.y2)
					data->wholeBoundingBox.y2 = boundingBox.y2;

				data->wholeLength += strokeLength;

				iStroke++;
				newFlag = TRUE;
			}

			data->lineSegmentLengthArray[iPoint] = segmentLength;
			data->lineSegmentAngleArray[iPoint] = segmentAngle;

			point++;
		}
		SDK_ASSERTMSG(newFlag, "the pattern contains a non-terminated stroke.");

		if (data->wholeLength == 0) {
			OS_Warning("the pattern is zero-Euclid-length.");
			data->wholeLength = PRC_TINY_LENGTH;
		}
	}

	{
		fx32 sumSegmentToStrokeLength, sumStrokeToWholeLength;
		fx16 prevSegmentToStrokeRatio, prevStrokeToWholeRatio;

		iStroke = 0;
		sumStrokeToWholeLength = 0;

		sumSegmentToStrokeLength = 0;
		prevStrokeToWholeRatio = 0;

		prevSegmentToStrokeRatio = 0;
		newFlag = TRUE;
		point = data->pointArray;

		for (iPoint = 0; iPoint < size; iPoint++, point++) {
			fx16 tmpRatio;

			sumSegmentToStrokeLength += data->lineSegmentLengthArray[iPoint];
			tmpRatio = (fx16)FX_Div(sumSegmentToStrokeLength, data->strokeLengths[iStroke]);
			data->lineSegmentRatioToStrokeArray[iPoint] = (fx16)(tmpRatio - prevSegmentToStrokeRatio);
			prevSegmentToStrokeRatio = tmpRatio;

			if (!PRC_IsPenUpMarker(point)) {
				if (newFlag) {
					sumStrokeToWholeLength += data->strokeLengths[iStroke];
					tmpRatio = (fx16)FX_Div(sumStrokeToWholeLength, data->wholeLength);
					data->strokeRatios[iStroke] = (fx16)(tmpRatio - prevStrokeToWholeRatio);
					prevStrokeToWholeRatio = tmpRatio;
					newFlag = FALSE;
				}
			} else {
				iStroke++;
				newFlag = TRUE;
			}
		}
	}
}

void PRCi_GetPatternStrokes_Common (PRCStrokes * strokes, const PRCiPatternData_Common * data)
{
	strokes->size = data->pointCount;
	strokes->capacity = data->pointCount;
	strokes->points = (PRCPoint *)data->pointArray;
}
