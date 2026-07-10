#ifndef	__WLFRAME_H_
#define	__WLFRAME_H_

typedef	struct {
	u16			frameId;
	u8			rsv1[4];
	u16			length;

	u16			status;
	u16			rsvm1;
	u16			rsvm2;
//	u8			rsv2[6];
	u8			rate;
	u8			rssi;
//	u8			rsv3[4];
	u32			rsvm3;

	u8			rsv4[4];
	u16			destAdrs[3];
	u16			srcAdrs[3];
	u8			rsv5[8];

	u16*		datap;
} WlTxFrame;

typedef	struct {
	u16			frameId;
	u8			rsv1[4];
	u16			length;

	u16			status;
	u16			rsvm2;
	u16			timeStamp;
	u8			rate;
	u8			rssi;
	u32			rsvm3;

	u8			rsv4[4];
	u16			destAdrs[3];
	u16			srcAdrs[3];
	u8			rsv5[8];

	u8			data[4];
} WlRxFrame;

typedef	struct {
	u8			rsv1[6];
	u16			length;

	u8			rsv2[6];
	u8			rate;
	u8			rsv3[5];

	u8			data[4];
} WlTestFrame;

typedef	struct {
	u8			rsv1[6];
	u16			length;

	u8			rsv2[6];
	u8			rate;
	u8			rssi;
	u8			rsv3[4];

	u8			rsv4[4];
	u16			destAdrs[3];
	u16			srcAdrs[3];
	u8			rsv5[6];
	u16			seqCtrl;

	u16			txop;
	u16			bitmap;
	u16*		datap;
} WlTxMpFrame;

typedef	struct {
	u8			rsv1[6];
	u16			length;

	u16			txKeySts;
	u16			ackTimeStamp;
	u16			timeStamp;
	u8			rate;
	u8			rssi;
	u8			rsv3[4];

	u8			rsv4[4];
	u16			destAdrs[3];
	u16			srcAdrs[3];
	u8			rsv5[6];
	u16			seqCtrl;

	u16			txop;
	u16			bitmap;
	u16			data[1];
} WlRxMpFrame;

typedef	struct {
	u8			rsv1[6];
	u16			length;

	u16			txKeySts;
	u16			rsv3;
	u16			timeStamp;
	u8			rate;
	u8			rssi;
	u8			rsv4[4];

	u8			rsv5[4];
	u16			destAdrs[3];
	u16			srcAdrs[3];
	u8			rsv6[6];
	u16			seqCtrl;

	u16			tmptt;
	u16			bitmap;
} WlRxMpAckFrame;

typedef	struct {
	u16			length;
	u8			rate;
	u8			rssi;
	u16			aid;
	u16			noResponse;
	u8			cdata[4];
} WlMpKeyData;

typedef	struct {
	u16			bitmap;
	u16			errBitmap;
	u16			count;
	u16			length;
	u16			txCount;

	WlMpKeyData	data[1];
} WlMpKey;

typedef	struct {
	struct {
		u32		success;
		u32		failed;
		u32		retry;
		u32		ackErr;

		u32		unicast;
		u32		multicast;
		u32		wep;
		u32		beacon;
	} tx;

	struct {
		u32		rts;
		u32		fragment;
		u32		unicast;
		u32		multicast;
		u32		wep;
		u32		beacon;

		u32		fcsErr;
		u32		duplicateErr;
		u32		mpDuplicateErr;
		u32		icvErr;
		u32		fcErr;
		u32		lengthErr;
		u32		plcpErr;
		u32		bufOvfErr;
		u32		pathErr;
		u32		rateErr;

		u32		fcsOk;
	} rx;

	struct {
		u32		txMp;
		u32		txKey;
		u32		txNull;
		u32		rxMp;
		u32		rxMpAck;
		u32		keyResponseErr[15];
	} multiPoll;
} WlCounter;

#endif	// __WLFRAME_H_

