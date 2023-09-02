#include <nitro/types.h>
#include <nitro/mi/uncompress.h>
#include <nitro/mi/compress.h>

static u32 SearchLZ(const u8 * startp, const u8 * nextp, u32 remainSize, u16 * offset, u32 maxLength);

u32 MI_CompressLZImpl (const u8 * srcp, u32 size, u8 * dstp, BOOL exFormat)
{
    u32 LZDstCount;
    u32 LZSrcCount;
    const u8 * srcStartp;
    u8 LZCompFlags;
    u8 * LZCompFlagsp;
    u16 lastOffset;
    u32 lastLength;
    u8 i;
    u32 dstMax;
    const u32 MAX_LENGTH = (exFormat)? (0xFFFF + 0xFF + 0xF + 3U) : (0xF + 3U);

    SDK_ALIGN2_ASSERT(srcp);

    *(u32 *)dstp = size << 8 | MI_COMPRESSION_LZ | (exFormat? 1 : 0);
    dstp += 4;
    LZDstCount = 4;
    LZSrcCount = 0;
    srcStartp = srcp;
    dstMax = size;

    while (size > 0) {
        LZCompFlags = 0;
        LZCompFlagsp = dstp++;
        LZDstCount++;

        for (i = 0; i < 8; i++) {
            LZCompFlags <<= 1;
            if (size <= 0) {
                continue;
            }

            if ((lastLength = SearchLZ(srcStartp, srcp, size, &lastOffset, MAX_LENGTH)) != 0) {
                u32 length;

                LZCompFlags |= 0x1;

                if (LZDstCount + 2 >= dstMax) {
                    return 0;
                }

                if (exFormat) {
                    if ( lastLength >= 0xFF + 0xF + 3 ) {
                        length = (u32)(lastLength - 0xFF - 0xF - 3);
                        *dstp++ = (u8)(0x10 | (length >> 12));
                        *dstp++ = (u8)(length >> 4);
                        LZDstCount += 2;
                    } else if ( lastLength >= 0xF + 2 )   {
                        length = (u32)(lastLength - 0xF - 2);
                        *dstp++ = (u8)(length >> 4);
                        LZDstCount += 1;
                    } else {
                        length = (u32)(lastLength - 1);
                    }
                } else {
                    length = (u32)(lastLength - 3);
                }

                *dstp++ = (u8)(length << 4 | (lastOffset - 1) >> 8);
                *dstp++ = (u8)((lastOffset - 1) & 0xff);
                LZDstCount += 2;
                srcp += lastLength;
                size -= lastLength;
            } else {
                if (LZDstCount + 1 >= dstMax) {
                    return 0;
                }

                *dstp++ = *srcp++;
                size--;
                LZDstCount++;
            }
        }

        *LZCompFlagsp = LZCompFlags;
    }

    i = 0;
    while ((LZDstCount + i) & 0x3) {
        *dstp++ = 0;
        i++;
    }

    return LZDstCount;
}

static u32 SearchLZ (const u8 * startp, const u8 * nextp, u32 remainSize, u16 * offset, u32 maxLength)
{
    const u8 * searchp;
    const u8 * headp, * searchHeadp;
    u16 maxOffset;
    u32 currLength = 2;
    u32 tmpLength;

    if (remainSize < 3) {
        return 0;
    }

    searchp = nextp - 4096;
    if (searchp < startp) {
        searchp = startp;
    }

    while (nextp - searchp >= 2) {
        headp = nextp;
        searchHeadp = searchp;
        tmpLength = 0;

        while ((*headp != *searchp) || (*(headp + 1) != *(searchp + 1)) || (*(headp + 2) != *(searchp + 2))) {
            searchp++;
            if (nextp - searchp < 2) {
                goto search_end;
            }
        }

        searchHeadp = searchp + 3;
        headp += 3;
        tmpLength = 3;

        while (((u32)(headp - nextp) < remainSize) && (*headp == *searchHeadp)) {
            headp++;
            searchHeadp++;
            tmpLength++;

            if (tmpLength == maxLength) {
                break;
            }
        }

        if (tmpLength > currLength) {
            currLength = tmpLength;
            maxOffset = (u16)(nextp - searchp);

            if (currLength == maxLength || currLength == remainSize) {
                break;
            }
        }

        searchp++;
    }
search_end:
    if (currLength < 3) {
        return 0;
    }

    *offset = maxOffset;
    return currLength;
}

typedef struct {
    u16 windowPos;
    u16 windowLen;
    s16 * LZOffsetTable;
    s16 * LZByteTable;
    s16 * LZEndTable;
} LZCompressInfo;

static void LZInitTable(LZCompressInfo * info, u8 * work);
static void SlideByte(LZCompressInfo * info, const u8 * srcp);
static inline void LZSlide(LZCompressInfo * info, const u8 * srcp, u32 n);
static u32 SearchLZFast(LZCompressInfo * info, const u8 * nextp, u32 remainSize, u16 * offset, u32 maxLength);

static void LZInitTable (LZCompressInfo * info, u8 * work)
{
    u16 i;

    info->LZOffsetTable = (s16 *)work;
    info->LZByteTable = (s16 *)work + 4096;
    info->LZEndTable = (s16 *)work + 4096 + 256;

    for (i = 0; i < 256; i++) {
        info->LZByteTable[i] = -1;
        info->LZEndTable[i] = -1;
    }

    info->windowPos = 0;
    info->windowLen = 0;
}

static void SlideByte (LZCompressInfo * info, const u8 * srcp)
{
    s16 offset;
    u8 in_data = *srcp;
    u16 insert_offset;

    s16 * const LZByteTable = info->LZByteTable;
    s16 * const LZOffsetTable = info->LZOffsetTable;
    s16 * const LZEndTable = info->LZEndTable;
    const u16 windowPos = info->windowPos;
    const u16 windowLen = info->windowLen;

    if (windowLen == 4096) {
        u8 out_data = *(srcp - 4096);

        if ((LZByteTable[out_data] = LZOffsetTable[LZByteTable[out_data]]) == -1) {
            LZEndTable[out_data] = -1;
        }

        insert_offset = windowPos;
    } else {
        insert_offset = windowLen;
    }

    offset = LZEndTable[in_data];

    if (offset == -1) {
        LZByteTable[in_data] = (s16)insert_offset;
    } else {
        LZOffsetTable[offset] = (s16)insert_offset;
    }

    LZEndTable[in_data] = (s16)insert_offset;
    LZOffsetTable[insert_offset] = -1;

    if (windowLen == 4096) {
        info->windowPos = (u16)((windowPos + 1) % 0x1000);
    } else {
        info->windowLen++;
    }
}

static inline void LZSlide (LZCompressInfo * info, const u8 * srcp, u32 n)
{
    u32 i;

    for (i = 0; i < n; i++) {
        SlideByte(info, srcp++);
    }
}

u32 MI_CompressLZFastImpl (const u8 * srcp, u32 size, u8 * dstp, u8 * work, BOOL exFormat)
{
    u32 LZDstCount;
    u8 LZCompFlags;
    u8 * LZCompFlagsp;
    u16 lastOffset;
    u32 lastLength;
    u8 i;
    u32 dstMax;
    LZCompressInfo info;
    const u32 MAX_LENGTH = (exFormat)? (0xFFFF + 0xFF + 0xF + 3U) : (0xF + 3U);

    SDK_ALIGN2_ASSERT(srcp);
    SDK_NULL_ASSERT(work);

    *(u32 *)dstp = size << 8 | MI_COMPRESSION_LZ | (exFormat? 1 : 0);
    dstp += 4;
    LZDstCount = 4;
    dstMax = size;
    LZInitTable(&info, work);

    while (size > 0) {
        LZCompFlags = 0;
        LZCompFlagsp = dstp++;
        LZDstCount++;

        for (i = 0; i < 8; i++) {
            LZCompFlags <<= 1;
            if (size <= 0) {
                continue;
            }

            if ((lastLength = SearchLZFast(&info, srcp, size, &lastOffset, MAX_LENGTH)) != 0 ) {
                u32 length;

                LZCompFlags |= 0x1;

                if (LZDstCount + 2 >= dstMax) {
                    return 0;
                }

                if (exFormat) {
                    if ( lastLength >= 0xFF + 0xF + 3 ) {
                        length = (u32)(lastLength - 0xFF - 0xF - 3);
                        *dstp++ = (u8)(0x10 | (length >> 12));
                        *dstp++ = (u8)(length >> 4);
                        LZDstCount += 2;
                    } else if ( lastLength >= 0xF + 2 )   {
                        length = (u32)(lastLength - 0xF - 2);
                        *dstp++ = (u8)(length >> 4);
                        LZDstCount += 1;
                    } else {
                        length = (u32)(lastLength - 1);
                    }
                } else {
                    length = (u32)(lastLength - 3);
                }

                *dstp++ = (u8)(length << 4 | (lastOffset - 1) >> 8);
                *dstp++ = (u8)((lastOffset - 1) & 0xff);
                LZDstCount += 2;

                LZSlide(&info, srcp, lastLength);

                srcp += lastLength;
                size -= lastLength;
            } else {
                if (LZDstCount + 1 >= dstMax) {
                    return 0;
                }

                LZSlide(&info, srcp, 1);

                *dstp++ = *srcp++;
                size--;
                LZDstCount++;
            }
        }
        *LZCompFlagsp = LZCompFlags;
    }

    i = 0;
    while ((LZDstCount + i) & 0x3) {
        *dstp++ = 0;
        i++;
    }

    return LZDstCount;
}

static u32 SearchLZFast (LZCompressInfo * info, const u8 * nextp, u32 remainSize, u16 * offset, u32 maxLength)
{
    const u8 * searchp;
    const u8 * headp, * searchHeadp;
    u16 maxOffset;
    u32 currLength = 2;
    u32 tmpLength;
    s32 w_offset;
    s16 * const LZOffsetTable = info->LZOffsetTable;
    const u16 windowPos = info->windowPos;
    const u16 windowLen = info->windowLen;

    if (remainSize < 3) {
        return 0;
    }

    w_offset = info->LZByteTable[*nextp];

    while (w_offset != -1) {
        if (w_offset < windowPos) {
            searchp = nextp - windowPos + w_offset;
        } else {
            searchp = nextp - windowLen - windowPos + w_offset;
        }

        if (*(searchp + 1) != *(nextp + 1) || *(searchp + 2) != *(nextp + 2)) {
            w_offset = LZOffsetTable[w_offset];
            continue;
        }

        if (nextp - searchp < 2) {
            break;
        }

        tmpLength = 3;
        searchHeadp = searchp + 3;
        headp = nextp + 3;

        while (((u32)(headp - nextp) < remainSize) && (*headp == *searchHeadp)) {
            headp++;
            searchHeadp++;
            tmpLength++;

            if (tmpLength == maxLength) {
                break;
            }
        }

        if (tmpLength > currLength) {
            currLength = tmpLength;
            maxOffset = (u16)(nextp - searchp);

            if (currLength == maxLength || currLength == remainSize) {
                break;
            }
        }

        w_offset = LZOffsetTable[w_offset];
    }

    if (currLength < 3) {
        return 0;
    }

    *offset = maxOffset;
    return currLength;
}

u32 MI_CompressRL (const u8 * srcp, u32 size, u8 * dstp)
{
    u32 RLDstCount;
    u32 RLSrcCount;
    u8 RLCompFlag;
    u8 runLength;
    u8 rawDataLength;
    u32 i;

    const u8 * startp;

    *(u32 *)dstp = size << 8 | MI_COMPRESSION_RL;
    RLDstCount = 4;
    RLSrcCount = 0;
    rawDataLength = 0;
    RLCompFlag = 0;

    while (RLSrcCount < size) {
        startp = &srcp[RLSrcCount];

        for (i = 0; i < 128; i++) {
            if (RLSrcCount + rawDataLength >= size) {
                rawDataLength = (u8)(size - RLSrcCount);
                break;
            }

            if (RLSrcCount + rawDataLength + 2 < size) {
        		if (startp[i] == startp[i + 1] && startp[i] == startp[i + 2]) {
					RLCompFlag = 1;
					break;
				}
			}

			rawDataLength++;
		}

		if (rawDataLength) {
			if (RLDstCount + rawDataLength + 1 >= size) {
				return 0;
			}

			dstp[RLDstCount++] = (u8)(rawDataLength - 1);

			for (i = 0; i < rawDataLength; i++) {
				dstp[RLDstCount++] = srcp[RLSrcCount++];
			}

			rawDataLength = 0;
		}

		if (RLCompFlag) {
			runLength = 3;
			for (i = 3; i < 128 + 2; i++) {

				if (RLSrcCount + runLength >= size) {
					runLength = (u8)(size - RLSrcCount);
					break;
				}

				if (srcp[RLSrcCount] != srcp[RLSrcCount + runLength]) {
					break;
				}

				runLength++;
			}

			if (RLDstCount + 2 >= size) {
				return 0;
			}

			dstp[RLDstCount++] = (u8)(0x80 | (runLength - 3));
			dstp[RLDstCount++] = srcp[RLSrcCount];

			RLSrcCount += runLength;
			RLCompFlag = 0;
		}
	}

	i = 0;
	while ((RLDstCount + i) & 0x3) {
		dstp[RLDstCount + i] = 0;
		i++;
	}

	return RLDstCount;
}

#define HUFF_END_L  0x80
#define HUFF_END_R  0x40

static void HuffUpdateParentDepth(u16 leftNo, u16 rightNo);
static void HuffMakeCode(u16 nodeNo, u32 paHuffCode);
static u8 HuffCountHWord(u16 nodeNo);
static void HuffMakeHuffTree(u16 rootNo);
static void HuffMakeSubsetHuffTree(u16 huffTreeNo, u8 rightNodeFlag);
static u8 HuffRemainingNodeCanSetOffset(u8 costHWord);
static void HuffSetOneNodeOffset(u16 huffTreeNo, u8 rightNodeFlag);

typedef struct {
	u32 Freq;
	u16 No;
	s16 PaNo;
	s16 ChNo[2];
	u16 PaDepth;
	u16 LeafDepth;
	u32 HuffCode;
	u8 Bit;
	u8 _padding;
	u16 HWord;
} HuffData;

static HuffData * HuffTable;
static const HuffData HuffTableInitData = { 0, 0, 0, {-1, -1}, 0, 0, 0, 0, 0 };
static u8 HuffTreeTop;
static u8 * HuffTree;

typedef struct {
	u8 leftOffsetNeed;
	u8 rightOffsetNeed;
	u16 leftNodeNo;
	u16 rightNodeNo;
} HuffTreeCtrlData;

static HuffTreeCtrlData * HuffTreeCtrl;
static const HuffTreeCtrlData HuffTreeCtrlInitData = { 1, 1, 0, 0 };
static u16 sHuffDataNum;

typedef struct {
	HuffData HuffTable[512];
	u8 HuffTree[256 * 2];
	HuffTreeCtrlData HuffTreeCtrl[256];
} HuffWork;

u32 MI_CompressHuffman (const u8 * srcp, u32 size, u8 * dstp, u8 huffBitSize, u8 * work)
{
	u32 HuffDstCount;
	u8 tmp;
	u16 nodeNum;
	u16 tableTop;
	s32 leftNo, rightNo;
	s32 i, ii, iii;
	u8 srcTmp;
	u32 bitStream = 0;
	u32 streamLength = 0;
	u16 rootNo;

	SDK_NULL_ASSERT(srcp);
	SDK_NULL_ASSERT(dstp);
	SDK_ASSERT(huffBitSize == 4 || huffBitSize == 8);
	SDK_NULL_ASSERT(work);
	SDK_ALIGN4_ASSERT(work);

	HuffTable = ((HuffWork *) work)->HuffTable;
	HuffTree = ((HuffWork *) work)->HuffTree;
	HuffTreeCtrl = ((HuffWork *) work)->HuffTreeCtrl;

	sHuffDataNum = (u16)(1 << huffBitSize);
	tableTop = sHuffDataNum;

	for (i = 0; i < (u16)(sHuffDataNum * 2); i++) {
		HuffTable[i] = HuffTableInitData;
		HuffTable[i].No = (u16)i;
	}

	if (huffBitSize == 8) {
		for (i = 0; i < size; i++) {
			HuffTable[srcp[i]].Freq++;
		}
	} else {
		for (i = 0; i < size; i++) {
			tmp = (u8)((srcp[i] & 0xf0) >> 4);
			HuffTable[tmp].Freq++;
			tmp = (u8)(srcp[i] & 0x0f);
			HuffTable[tmp].Freq++;
		}
	}

	leftNo = rightNo = -1;
	while (1) {
		for (i = 0; i < tableTop; i++) {
			if ((HuffTable[i].Freq != 0) && (HuffTable[i].PaNo == 0)) {
				leftNo = i;
				break;
			}
		}

		for (i = (u16)leftNo; i < tableTop; i++) {
			if ((HuffTable[i].Freq != 0) &&
			    (HuffTable[i].PaNo == 0) && (HuffTable[i].Freq < HuffTable[leftNo].Freq)) {
				leftNo = i;
			}
		}

		for (i = 0; i < tableTop; i++) {
			if ((HuffTable[i].Freq != 0) && (HuffTable[i].PaNo == 0) && (i != leftNo)) {
				rightNo = i;
				break;
			}
		}

		for (i = (u16)rightNo; i < tableTop; i++) {
			if ((HuffTable[i].Freq != 0) &&
			    (HuffTable[i].PaNo == 0) &&
			    (HuffTable[i].Freq < HuffTable[rightNo].Freq) && (i != leftNo)) {
				rightNo = i;
			}
		}

		if (rightNo < 0) {
			if (tableTop == sHuffDataNum) {
				HuffTable[tableTop].Freq = HuffTable[leftNo].Freq;
				HuffTable[tableTop].ChNo[0] = (s16)leftNo;
				HuffTable[tableTop].ChNo[1] = (s16)leftNo;
				HuffTable[tableTop].LeafDepth = 1;
				HuffTable[leftNo].PaNo = (s16)tableTop;
				HuffTable[leftNo].Bit = 0;
				HuffTable[leftNo].PaDepth = 1;
			} else {
				tableTop--;
			}

			rootNo = tableTop;
			nodeNum = (u16)((rootNo - sHuffDataNum + 1) * 2 + 1);
			break;
		}

		HuffTable[tableTop].Freq = HuffTable[leftNo].Freq + HuffTable[rightNo].Freq;
		HuffTable[tableTop].ChNo[0] = (s16)leftNo;
		HuffTable[tableTop].ChNo[1] = (s16)rightNo;

		if (HuffTable[leftNo].LeafDepth > HuffTable[rightNo].LeafDepth) {
			HuffTable[tableTop].LeafDepth = (u16)(HuffTable[leftNo].LeafDepth + 1);
		} else {
			HuffTable[tableTop].LeafDepth = (u16)(HuffTable[rightNo].LeafDepth + 1);
		}

		HuffTable[leftNo].PaNo = HuffTable[rightNo].PaNo = (s16)tableTop;
		HuffTable[leftNo].Bit = 0;
		HuffTable[rightNo].Bit = 1;

		HuffUpdateParentDepth((u16)leftNo, (u16)rightNo);

		tableTop++;
		leftNo = rightNo = -1;
	}

	HuffMakeCode(rootNo, 0x00);
	(void)HuffCountHWord(rootNo);
	HuffMakeHuffTree(rootNo);

	HuffTree[0] = --HuffTreeTop;

	*(u32 *)dstp = size << 8 | MI_COMPRESSION_HUFFMAN | huffBitSize;
	HuffDstCount = 4;

	if (HuffDstCount + (HuffTreeTop + 1) * 2 >= size) {
		return 0;
	}

	for (i = 0; i < (u16)((HuffTreeTop + 1) * 2); i++) {
		dstp[HuffDstCount++] = ((u8 *)HuffTree)[i];
	}

	while (HuffDstCount & 0x3) {
		if (HuffDstCount & 0x1) {
			HuffTreeTop++;
			dstp[4]++;
		}

		dstp[HuffDstCount++] = 0;
	}

	for (i = 0; i < size; i++) {
		u8 val = srcp[i];
		if (huffBitSize == 8) {
			bitStream = (bitStream << HuffTable[val].PaDepth) | HuffTable[val].HuffCode;
			streamLength += HuffTable[val].PaDepth;

			if (HuffDstCount + (streamLength / 8) >= size) {
				return 0;
			}

			for (ii = 0; ii < streamLength / 8; ii++) {
				dstp[HuffDstCount++] = (u8)(bitStream >> (streamLength - (ii + 1) * 8));
			}
			streamLength %= 8;
		} else {
			for (ii = 0; ii < 2; ii++) {
				if (ii) {
					srcTmp = (u8)(val >> 4);
				} else {
					srcTmp = (u8)(val & 0x0F);
				}

				bitStream = (bitStream << HuffTable[srcTmp].PaDepth) | HuffTable[srcTmp].HuffCode;
				streamLength += HuffTable[srcTmp].PaDepth;

				if (HuffDstCount + (streamLength / 8) >= size) {
					return 0;
				}

				for (iii = 0; iii < streamLength / 8; iii++) {
					dstp[HuffDstCount++] = (u8)(bitStream >> (streamLength - (iii + 1) * 8));
				}

				streamLength %= 8;
			}
		}
	}

	if (streamLength != 0) {
		if (HuffDstCount + 1 >= size) {
			return 0;
		}

		dstp[HuffDstCount++] = (u8)(bitStream << (8 - streamLength));
	}

	while (HuffDstCount & 0x3) {
		dstp[HuffDstCount++] = 0;
	}

	for (i = 1 + (HuffTreeTop + 1) * 2 / 4; i < (HuffDstCount / 4) + 1; i++) {
		tmp = dstp[i * 4 + 0];

		dstp[i * 4 + 0] = dstp[i * 4 + 3];
		dstp[i * 4 + 3] = tmp;

		tmp = dstp[i * 4 + 1];

		dstp[i * 4 + 1] = dstp[i * 4 + 2];
		dstp[i * 4 + 2] = tmp;
	}

	return HuffDstCount;
}

static void HuffMakeHuffTree (u16 rootNo)
{
	s16 i;
	u16 tmp;
	s16 costHWord, tmpCostHWord;
	s16 costOffsetNeed, tmpCostOffsetNeed;
	s16 costMaxKey, costMaxRightFlag;
	u8 offsetNeedNum;
	u8 tmpKey, tmpRightFlag;

	for (i = 0; i < 256; i++) {
		HuffTree[i * 2] = HuffTree[i * 2 + 1] = 0;
	}

	for (i = 0; i < 256; i++) {
		HuffTreeCtrl[i] = HuffTreeCtrlInitData;
	}

	HuffTreeTop = 1;
	costOffsetNeed = 0;

	HuffTreeCtrl[0].leftOffsetNeed = 0;
	HuffTreeCtrl[0].rightNodeNo = rootNo;

	while (1) {
		offsetNeedNum = 0;
		for (i = 0; i < HuffTreeTop; i++) {
			if (HuffTreeCtrl[i].leftOffsetNeed) {
				offsetNeedNum++;
			}
			if (HuffTreeCtrl[i].rightOffsetNeed) {
				offsetNeedNum++;
			}
		}

		costHWord = -1;
		costMaxKey = -1;

		tmpKey = 0;
		tmpRightFlag = 0;

		for (i = 0; i < HuffTreeTop; i++) {
			tmpCostOffsetNeed = (u8)(HuffTreeTop - i);

			if (HuffTreeCtrl[i].leftOffsetNeed) {
				tmpCostHWord = (s16)HuffTable[HuffTreeCtrl[i].leftNodeNo].HWord;

				if ((tmpCostHWord + offsetNeedNum) > 64) {
					goto leftCostEvaluationEnd;
				}

				if (!HuffRemainingNodeCanSetOffset((u8)tmpCostHWord)) {
					goto leftCostEvaluationEnd;
				}

				if (tmpCostHWord > costHWord) {
					costMaxKey = i;
					costMaxRightFlag = 0;
				} else if ((tmpCostHWord == costHWord) && (tmpCostOffsetNeed > costOffsetNeed))   {
					costMaxKey = i;
					costMaxRightFlag = 0;
				}
			}
leftCostEvaluationEnd: {
			}

			if (HuffTreeCtrl[i].rightOffsetNeed) {
				tmpCostHWord = (s16)HuffTable[HuffTreeCtrl[i].rightNodeNo].HWord;

				if ((tmpCostHWord + offsetNeedNum) > 64) {
					goto rightCostEvaluationEnd;
				}

				if (!(HuffRemainingNodeCanSetOffset((u8)tmpCostHWord))) {
					goto rightCostEvaluationEnd;
				}

				if (tmpCostHWord > costHWord) {
					costMaxKey = i;
					costMaxRightFlag = 1;
				} else if ((tmpCostHWord == costHWord) && (tmpCostOffsetNeed > costOffsetNeed))   {
					costMaxKey = i;
					costMaxRightFlag = 1;
				}
			}
rightCostEvaluationEnd: {
			}
		}

		if (costMaxKey >= 0) {
			HuffMakeSubsetHuffTree((u8)costMaxKey, (u8)costMaxRightFlag);
			goto nextTreeMaking;
		} else {
			for (i = 0; i < HuffTreeTop; i++) {
				tmp = 0;
				tmpRightFlag = 0;

				if (HuffTreeCtrl[i].leftOffsetNeed) {
					tmp = HuffTable[HuffTreeCtrl[i].leftNodeNo].HWord;
				}

				if (HuffTreeCtrl[i].rightOffsetNeed) {
					if (HuffTable[HuffTreeCtrl[i].rightNodeNo].HWord > tmp) {
						tmpRightFlag = 1;
					}
				}

				if ((tmp != 0) || (tmpRightFlag)) {
					HuffSetOneNodeOffset((u8)i, tmpRightFlag);
					goto nextTreeMaking;
				}
			}
		}
		return;
nextTreeMaking: {
		}
	}
}

static void HuffMakeSubsetHuffTree (u16 huffTreeNo, u8 rightNodeFlag)
{
	u8 i;

	i = HuffTreeTop;
	HuffSetOneNodeOffset(huffTreeNo, rightNodeFlag);

	if (rightNodeFlag) {
		HuffTreeCtrl[huffTreeNo].rightOffsetNeed = 0;
	} else {
		HuffTreeCtrl[huffTreeNo].leftOffsetNeed = 0;
	}

	while (i < HuffTreeTop) {
		if (HuffTreeCtrl[i].leftOffsetNeed) {
			HuffSetOneNodeOffset(i, 0);
			HuffTreeCtrl[i].leftOffsetNeed = 0;
		}

		if (HuffTreeCtrl[i].rightOffsetNeed) {
			HuffSetOneNodeOffset(i, 1);
			HuffTreeCtrl[i].rightOffsetNeed = 0;
		}

		i++;
	}
}

static u8 HuffRemainingNodeCanSetOffset (u8 costHWord)
{
	u8 i;
	s16 capacity;

	capacity = (s16)(64 - costHWord);

	for (i = 0; i < HuffTreeTop; i++) {
		if (HuffTreeCtrl[i].leftOffsetNeed) {
			if ((HuffTreeTop - i) <= capacity) {
				capacity--;
			} else {
				return 0;
			}
		}

		if (HuffTreeCtrl[i].rightOffsetNeed) {
			if ((HuffTreeTop - i) <= capacity) {
				capacity--;
			} else {
				return 0;
			}
		}
	}

	return 1;
}

static void HuffSetOneNodeOffset (u16 huffTreeNo, u8 rightNodeFlag)
{
	u16 nodeNo;
	u8 offsetData = 0;

	if (rightNodeFlag) {
		nodeNo = HuffTreeCtrl[huffTreeNo].rightNodeNo;
		HuffTreeCtrl[huffTreeNo].rightOffsetNeed = 0;
	} else {
		nodeNo = HuffTreeCtrl[huffTreeNo].leftNodeNo;
		HuffTreeCtrl[huffTreeNo].leftOffsetNeed = 0;
	}

	if (HuffTable[HuffTable[nodeNo].ChNo[0]].LeafDepth == 0) {
		offsetData |= 0x80;
		HuffTree[HuffTreeTop * 2 + 0] = (u8)HuffTable[nodeNo].ChNo[0];
		HuffTreeCtrl[HuffTreeTop].leftNodeNo = (u8)HuffTable[nodeNo].ChNo[0];
		HuffTreeCtrl[HuffTreeTop].leftOffsetNeed = 0;
	} else {
		HuffTreeCtrl[HuffTreeTop].leftNodeNo = (u16)HuffTable[nodeNo].ChNo[0];
	}

	if (HuffTable[HuffTable[nodeNo].ChNo[1]].LeafDepth == 0) {
		offsetData |= 0x40;
		HuffTree[HuffTreeTop * 2 + 1] = (u8)HuffTable[nodeNo].ChNo[1];
		HuffTreeCtrl[HuffTreeTop].rightNodeNo = (u8)HuffTable[nodeNo].ChNo[1];
		HuffTreeCtrl[HuffTreeTop].rightOffsetNeed = 0;
	} else {
		HuffTreeCtrl[HuffTreeTop].rightNodeNo = (u16)HuffTable[nodeNo].ChNo[1];
	}

	offsetData |= (u8)(HuffTreeTop - huffTreeNo - 1);

	HuffTree[huffTreeNo * 2 + rightNodeFlag] = offsetData;
	HuffTreeTop++;
}

static void HuffUpdateParentDepth (u16 leftNo, u16 rightNo)
{
	HuffTable[leftNo].PaDepth++;
	HuffTable[rightNo].PaDepth++;

	if (HuffTable[leftNo].LeafDepth != 0) {
		HuffUpdateParentDepth((u16)HuffTable[leftNo].ChNo[0], (u16)HuffTable[leftNo].ChNo[1]);
	}

	if (HuffTable[rightNo].LeafDepth != 0) {
		HuffUpdateParentDepth((u16)HuffTable[rightNo].ChNo[0], (u16)HuffTable[rightNo].ChNo[1]);
	}
}

static void HuffMakeCode (u16 nodeNo, u32 paHuffCode)
{
	HuffTable[nodeNo].HuffCode = (paHuffCode << 1) | HuffTable[nodeNo].Bit;

	if (HuffTable[nodeNo].LeafDepth != 0) {
		HuffMakeCode((u16)HuffTable[nodeNo].ChNo[0], HuffTable[nodeNo].HuffCode);
		HuffMakeCode((u16)HuffTable[nodeNo].ChNo[1], HuffTable[nodeNo].HuffCode);
	}
}

static u8 HuffCountHWord (u16 nodeNo)
{
	u8 leftHWord, rightHWord;

	switch (HuffTable[nodeNo].LeafDepth) {
	case 0:
		return 0;
	case 1:
		leftHWord = rightHWord = 0;
		break;
	default:
		leftHWord = HuffCountHWord((u16)HuffTable[nodeNo].ChNo[0]);
		rightHWord = HuffCountHWord((u16)HuffTable[nodeNo].ChNo[1]);
		break;
	}

	HuffTable[nodeNo].HWord = (u16)(leftHWord + rightHWord + 1);
	return (u8)(leftHWord + rightHWord + 1);
}
