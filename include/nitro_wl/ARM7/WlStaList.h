#ifndef	__WLSTALIST_H_
#define	__WLSTALIST_H_

typedef struct {
	u16			state;
	u16			aid;

	u16			macAdrs[3];
	u16			rssi;

	u16			capaInfo;
	u16			authSeed;

	u16			supRateSet;
	u16			rsv;

	u16			lastSeqCtrl;
	u16			frameCount;

	u16			lifeTime;
	u16			maxLifeTime;

} WlStaElement;

typedef	struct {
	u16	rssi;
	u16	channel;
	u16	capaInfo;
	u16	bssid[3];
	u16	ssidLength;
	u8	ssid[32];
	u16	beaconInterval;
	u16	lifeTime;
} WlApElement;

#endif	// __WLSTALIST_H_


