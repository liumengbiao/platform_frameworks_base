/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __AAH_TX_PACKET_H__
#define __AAH_TX_PACKET_H__

#include <utils/LinearTransform.h>
#include <utils/RefBase.h>
#include <utils/Timers.h>
#include <utils/Vector.h>

namespace android {

class TRTPPacket : public RefBase {
  protected:
    enum TRTPHeaderType {
        kHeaderTypeAudio = 1,
        kHeaderTypeVideo = 2,
        kHeaderTypeSubpicture = 3,
        kHeaderTypeControl = 4,
        kHeaderTypeMetaData = 5,
    };

    TRTPPacket(TRTPHeaderType headerType)
        : mIsPacked(false)
        , mVersion(2)
        , mPadding(false)
        , mExtension(false)
        , mCsrcCount(0)
        , mPayloadType(100)
        , mSeqNumber(0)
        , mPTSValid(false)
        , mPTS(0)
        , mEpoch(0)
        , mProgramID(0)
        , mSubstreamID(0)
        , mClockTranformValid(false)
        , mTRTPVersion(1)
        , mTRTPLength(0)
        , mTRTPHeaderType(headerType)
        , mPacket(NULL)
        , mPacketLen(0) { }

  public:
    virtual ~TRTPPacket();

    void setSeqNumber(uint16_t val);
    uint16_t getSeqNumber() const;

    void setPTS(int64_t val);
    int64_t getPTS() const;

    void setEpoch(uint32_t val);
    void setProgramID(uint8_t val);
    void setSubstreamID(uint16_t val);
    void setClockTransform(const LinearTransform& trans);

    uint8_t* getPacket() const;
    int getPacketLen() const;

    void setExpireTime(nsecs_t val);
    nsecs_t getExpireTime() const;

    virtual bool pack() = 0;
    bool isPacked() const { return mIsPacked; }

    // mask for the number of bits in a TRTP epoch
    static const uint32_t kTRTPEpochMask = (1 << 22) - 1;
    static const int kTRTPEpochShift = 10;

    static const uint32_t kCNC_RetryRequestID     = 'Treq';
    static const uint32_t kCNC_FastStartRequestID = 'Tfst';
    static const uint32_t kCNC_NakRetryRequestID  = 'Tnak';
    static const uint32_t kCNC_JoinGroupID        = 'Tjgp';
    static const uint32_t kCNC_LeaveGroupID       = 'Tlgp';
    static const uint32_t kCNC_NakJoinGroupID     = 'Tngp';

    static void writeU8(uint8_t*& buf, uint8_t val);
    static void writeU16(uint8_t*& buf, uint16_t val);
    static void writeU32(uint8_t*& buf, uint32_t val);
    static void writeU64(uint8_t*& buf, uint64_t val);
    static void writeFloat(uint8_t*& buf, float val);

  protected:
    static const int kRTPHeaderLen = 12;
    virtual int TRTPHeaderLen() const;

    void writeTRTPHeader(uint8_t*& buf,
                         bool isFirstFragment,
                         int totalPacketLen);

    bool mIsPacked;

    uint8_t mVersion;
    bool mPadding;
    bool mExtension;
    uint8_t mCsrcCount;
    uint8_t mPayloadType;
    uint16_t mSeqNumber;
    bool mPTSValid;
    int64_t  mPTS;
    uint32_t mEpoch;
    uint8_t mProgramID;
    uint16_t mSubstreamID;
    LinearTransform mClockTranform;
    bool mClockTranformValid;
    uint8_t mTRTPVersion;
    uint32_t mTRTPLength;
    TRTPHeaderType mTRTPHeaderType;

    uint8_t* mPacket;
    int mPacketLen;

    nsecs_t mExpireTime;
};

class TRTPAudioPacket : public TRTPPacket {
  public:
    TRTPAudioPacket()
        : TRTPPacket(kHeaderTypeAudio)
        , mCodecType(kCodecInvalid)
        , mRandomAccessPoint(false)
        , mDropable(false)
        , mDiscontinuity(false)
        , mEndOfStream(false)
        , mVolume(0)
        , mAccessUnitData(NULL)
        , mAccessUnitLen(0)
        , mAuxData(NULL)
        , mAuxDataLen(0) { }

    enum TRTPAudioCodecType {
        kCodecInvalid = 0,
        kCodecPCMBigEndian = 1,
        kCodecPCMLittleEndian = 2,
        kCodecMPEG1Audio = 3,
        kCodecAACAudio = 4,
    };

    void setCodecType(TRTPAudioCodecType val);
    void setRandomAccessPoint(bool val);
    void setDropable(bool val);
    void setDiscontinuity(bool val);
    void setEndOfStream(bool val);
    void setVolume(uint8_t val);
    void setAccessUnitData(const void* data, size_t len);
    void setAuxData(const void* data, size_t len);

    virtual bool pack();

  protected:
    virtual int TRTPHeaderLen() const;

  private:
    TRTPAudioCodecType mCodecType;
    bool mRandomAccessPoint;
    bool mDropable;
    bool mDiscontinuity;
    bool mEndOfStream;
    uint8_t mVolume;
    const void* mAccessUnitData;
    size_t mAccessUnitLen;
    const void* mAuxData;
    size_t mAuxDataLen;
};

class TRTPControlPacket : public TRTPPacket {
  public:
    TRTPControlPacket()
        : TRTPPacket(kHeaderTypeControl)
        , mCommandID(kCommandNop) {}

    enum TRTPCommandID {
        kCommandNop   = 1,
        kCommandFlush = 2,
        kCommandEOS   = 3,
        kCommandAPU   = 4,
    };

    void setCommandID(TRTPCommandID val);

    virtual bool pack();

  protected:
    explicit TRTPControlPacket(TRTPCommandID commandID)
        : TRTPPacket(kHeaderTypeControl)
        , mCommandID(commandID) {}

    TRTPCommandID mCommandID;
};

class TRTPActiveProgramUpdatePacket : public TRTPControlPacket {
  public:
    TRTPActiveProgramUpdatePacket()
        : TRTPControlPacket(kCommandAPU)
        , mProgramIDCnt(0) {}

    virtual bool pack();
    bool pushProgramID(uint8_t id);

  private:
    static const uint8_t kMaxProgramIDs = 31;

    uint8_t mProgramIDCnt;
    uint8_t mProgramIDs[kMaxProgramIDs];
};

enum TRTPMetaDataTypeID {
    kMetaDataNone   = 0,
    kMetaDataBeat   = 1,
};

class TRTPMetaDataBlock {
  public:
    TRTPMetaDataBlock(TRTPMetaDataTypeID typeId, uint16_t item_length)
            : mTypeId(typeId)
            , mItemLength(item_length) {}
    void writeBlockHead(uint8_t*& buf) const;
    virtual void write(uint8_t*& buf) const = 0;
    virtual ~TRTPMetaDataBlock() {}

    TRTPMetaDataTypeID mTypeId;
    uint16_t mItemLength;
};

// TRTPMetaDataPacket contains multiple TRTPMetaDataBlocks of different types
class TRTPMetaDataPacket : public TRTPPacket {
  public:
    TRTPMetaDataPacket()
        : TRTPPacket(kHeaderTypeMetaData) {}

    void append(TRTPMetaDataBlock* block);

    virtual bool pack();

    virtual ~TRTPMetaDataPacket();

  protected:

    Vector<TRTPMetaDataBlock*> mBlocks;
};


}  // namespace android

#endif  // __AAH_TX_PLAYER_H__
