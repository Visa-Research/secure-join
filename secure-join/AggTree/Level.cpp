
#include "Level.h"

namespace secJoin
{

    void PLevelNew::reveal(std::array<AggTreeLevel, 2>& tvs0, std::array<AggTreeLevel, 2>& tvs1)
    {
        PLevelNew l0, l1;
        l0.reveal(tvs0[0], tvs1[0]);
        l1.reveal(tvs0[1], tvs1[1]);

        perfectUnshuffle(l0, l1);
    }


    void PLevelNew::reveal(AggTreeLevel& tvs0, AggTreeLevel& tvs1)
    {
        AggTreeLevel ll;

        auto revealOne = [](auto& dst, auto& src0, auto& src1)
        {
            dst.resize(src0.numEntries(), src0.bitsPerEntry());
            for (u64 i = 0; i < dst.size(); ++i)
                dst(i) = src0(i) ^ src1(i);
        };

        revealOne(ll.mPreBit, tvs0.mPreBit, tvs1.mPreBit);
        revealOne(ll.mPreVal, tvs0.mPreVal, tvs1.mPreVal);
        revealOne(ll.mSufBit, tvs0.mSufBit, tvs1.mSufBit);
        revealOne(ll.mSufVal, tvs0.mSufVal, tvs1.mSufVal);

        ll.mPreVal.transpose(mPreVal);
        ll.mPreBit.transpose(mPreBit);
        ll.mSufVal.transpose(mSufVal);
        ll.mSufBit.transpose(mSufBit);
    }


    void PLevelNew::perfectUnshuffle(PLevelNew& l0, PLevelNew& l1)
    {
        auto preSize = l0.mPreVal.numEntries() + l1.mPreVal.numEntries();
        mPreVal.resize(preSize, l0.mPreVal.bitsPerEntry());
        mPreBit.resize(preSize, 1);
        auto sufSize = l0.mSufVal.numEntries() + l1.mSufVal.numEntries();
        mSufVal.resize(sufSize, l0.mSufVal.bitsPerEntry());
        mSufBit.resize(sufSize, 1);

        if (preSize & 1)
            throw RTE_LOC;
        if (sufSize & 1)
            throw RTE_LOC;

        for (u64 j = 0; j < preSize; j += 2)
        {
            for (u64 k = 0; k < mPreVal.bytesPerEnrty(); ++k)
            {
                mPreVal(j + 0, k) = l0.mPreBit(j / 2, k);
                mPreVal(j + 1, k) = l1.mPreBit(j / 2, k);
            }

            mPreBit(j + 0) = l0.mPreBit(j / 2);
            mPreBit(j + 1) = l1.mPreBit(j / 2);
        }

        for (u64 j = 0; j < sufSize; j += 2)
        {
            for (u64 k = 0; k < mSufVal.bytesPerEnrty(); ++k)
            {
                mSufVal(j + 0, k) = l0.mSufVal(j / 2, k);
                mSufVal(j + 1, k) = l1.mSufVal(j / 2, k);
            }

            mSufBit(j + 0) = l0.mSufBit(j / 2);
            mSufBit(j + 1) = l1.mSufBit(j / 2);
        }
    }


    void PLevel::reveal(std::array<AggTreeLevel, 2>& tvs0, std::array<AggTreeLevel, 2>& tvs1)
    {
        PLevel l0, l1;
        l0.reveal(tvs0[0], tvs1[0]);
        l1.reveal(tvs0[1], tvs1[1]);

        perfectUnshuffle(l0, l1);
    }


    auto revealOne(
        std::vector<oc::BitVector>& dst,
        TBinMatrix& src0,
        TBinMatrix& src1)
    {
        //BinMatrix temp(src0.numEntries(), src0.bitsPerEntry());
        //dst.resize(src0.numEntries(), src0.bitsPerEntry());

        TBinMatrix v(src0.numEntries(), src0.bitsPerEntry());
        for (i64 i = 0; i < v.size(); ++i)
            v(i) = src0(i) ^ src1(i);

        auto vv = v.transpose();

        dst.resize(src0.numEntries());
        for (u64 i = 0; i < dst.size(); ++i)
        {
            dst[i] = oc::BitVector(vv.data(i), vv.bitsPerEntry());
        }

    }

    auto revealOne(
        oc::BitVector& dst,
        TBinMatrix& src0,
        TBinMatrix& src1)
    {
        assert(src0.bitsPerEntry() < 2);
        assert(src1.bitsPerEntry() < 2);
        dst.resize(src0.numEntries());

        for (u64 i = 0; i < src0.bytesPerRow(); ++i)
        {
            dst.getSpan<u8>()[i] = src0(i) ^ src1(i);
        }
    };


    void PLevel::reveal(AggTreeLevel& tvs0, AggTreeLevel& tvs1)
    {
        revealOne(mPreBit, tvs0.mPreBit, tvs1.mPreBit);
        revealOne(mPreVal, tvs0.mPreVal, tvs1.mPreVal);
        revealOne(mSufBit, tvs0.mSufBit, tvs1.mSufBit);
        revealOne(mSufVal, tvs0.mSufVal, tvs1.mSufVal);
    }


    void PLevel::perfectUnshuffle(PLevel& l0, PLevel& l1)
    {

        auto preSize = l0.mPreVal.size() + l1.mPreVal.size();
        mPreBit.resize(preSize);
        mPreVal.resize(preSize);
        for (u64 i = 0; i < preSize; i += 2)
        {
            mPreBit[i + 0] = l0.mPreBit[i / 2];
            mPreVal[i + 0] = l0.mPreVal[i / 2];

            if (i + 1 < preSize)
            {
                mPreBit[i + 1] = l1.mPreBit[i / 2];
                mPreVal[i + 1] = l1.mPreVal[i / 2];
            }
        }

        auto sufSize = l0.mSufVal.size() + l1.mSufVal.size();
        mSufBit.resize(sufSize);
        mSufVal.resize(sufSize);
        for (u64 i = 0; i < sufSize; i += 2)
        {
            mSufBit[i + 0] = l0.mSufBit[i / 2];
            mSufVal[i + 0] = l0.mSufVal[i / 2];
            if (i + 1 < sufSize)
            {

                mSufBit[i + 1] = l1.mSufBit[i / 2];
                mSufVal[i + 1] = l1.mSufVal[i / 2];
            }
        }
    }
}