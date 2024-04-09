#pragma once
#include "secure-join/Defines.h"
#include "secure-join/CorGenerator/Base.h"

#include <vector>
#include <memory>
#include <numeric>

#include "macoro/task.h"

#include "Correlations.h"
#include "Batch.h"
#include "Request.h"

namespace secJoin
{

    struct GenState;
    struct RequestState;


    struct GenState : std::enable_shared_from_this<GenState>, oc::TimerAdapter
    {
        GenState() = delete;
        GenState(u64 partyIdx, PRNG&& prng, oc::Socket s, u64 numConcurrent, u64 batchSize, bool mock)
            : mPrng(std::move(prng))
            , mSock(std::move(s))
            , mNumConcurrent(numConcurrent)
            , mBatchSize(batchSize)
            , mMock(mock)
            , mPartyIdx(partyIdx)
        {
            if (batchSize < (1ull << 12))
                throw std::runtime_error("too small of batch size." LOCATION);

            if (batchSize > (1ull << 26))
                throw std::runtime_error("too large of batch size." LOCATION);

            if (numConcurrent == 0)
                throw std::runtime_error("numConcurrent can not be zero." LOCATION);

            if (numConcurrent > (1ull << 10))
                throw std::runtime_error("numConcurrent too large." LOCATION);
        }

        GenState(const GenState&) = delete;
        GenState(GenState&&) = delete;


        oc::SoftSpokenShOtSender<> mSendBase;
        oc::SoftSpokenShOtReceiver<> mRecvBase;


        macoro::thread_pool* mPool = nullptr;

        // next request index
        u64 mReqIndex = 0;

        u64 mNumOle = 0;
        u64 mNumOt = 0;

        struct ReqInfo
        {
            CorType mType;
            u64 mRole;
            u64 mSize;

            bool operator!=(const ReqInfo& r)const
            {
                return
                    mType != r.mType ||
                    mRole != r.mRole ||
                    mSize != r.mSize;
            }
            bool operator==(const ReqInfo& r) const
            {
                return !(*this != r);
            }
        };
        std::vector<ReqInfo> mReqs;
        //std::shared_ptr<Session> mSession;

        // all of the requests. These are broken down into mBatches.
        //std::vector<std::shared_ptr<RequestState>> mRequests;

        // randomness source
        PRNG mPrng;

        // the base socket that each subprotocol is forked from.
        coproto::Socket mSock;

        // the number of concurrent correlations that should be generated.
        u64 mNumConcurrent = 0;

        // the size that a batch of OT/OLEs should be generated in.
        u64 mBatchSize = 0;

        // enable additional debugging checks.
        bool mDebug = false;

        // true if we should just fake the correlation generation
        bool mMock = false;

        // used to determine which party should go first when its ambiguous.
        u64 mPartyIdx = -1;

        std::atomic<u64> mBatchStartIdx = 0;

        // returns a task that constructs the base OTs and assigns them to mBatches.
        macoro::task<> start();

        std::array<std::shared_ptr<Batch>, 2> mOtBatch;
        std::array<std::shared_ptr<Batch>, 2> mOleBatch;
        std::vector<std::shared_ptr<Batch>> mBatches;


        void set(SendBase& b);
        void set(RecvBase& b);


        void startBatch(Batch* b)
        {
            u64 idx = mBatchStartIdx;
            //assert(b->mIndex < mBatches.size());
            //assert(mBatches[b->mIndex].get() == b);

            while (b->mIndex >= idx)
            {
                bool s = mBatchStartIdx.compare_exchange_strong(idx, idx + 1);
                if (s)
                {
                    mBatches[idx]->mStart.set();
                    ++idx;
                }
            }
        }

        void abort()
        {
            u64 idx = mBatchStartIdx;
            while (idx < mBatches.size())
            {

                bool s = mBatchStartIdx.compare_exchange_strong(idx, idx + 1);
                if (s)
                {
                    mBatches[idx]->mAbort = true;
                    mBatches[idx]->mStart.set();
                    ++idx;
                }
            }
        }
    };

    struct CorGenerator
    {
        std::shared_ptr<GenState> mGenState;

        std::vector<std::weak_ptr<GenState>> mStarted;

        void init(
            coproto::Socket&& sock,
            PRNG& prng,
            u64 partyIdx,
            u64 numConcurrent,
            u64 batchSize,
            bool mock)
        {
            mGenState = std::make_shared<GenState>(partyIdx, prng.fork(), std::move(sock), numConcurrent, batchSize, mock);
        }

        Request<OtRecv> recvOtRequest(u64 n) {
            if (initialized() == false)
                throw std::runtime_error(LOCATION);
            return Request<OtRecv>{request(CorType::Ot, 0, oc::roundUpTo(n, 128))};
        }
        Request<OtSend> sendOtRequest(u64 n) {
            if (initialized() == false)
                throw std::runtime_error(LOCATION);
            return Request<OtSend>{request(CorType::Ot, 1, oc::roundUpTo(n, 128))};
        }
        Request<BinOle> binOleRequest(u64 n) {
            if (initialized() == false)
                throw std::runtime_error(LOCATION);
            return Request<BinOle>{request(CorType::Ole, mGenState->mPartyIdx, oc::roundUpTo(n, 128))};
        }

        void setBaseOts(SendBase& sb, RecvBase& rb)
        {
            mGenState->set(sb);
            mGenState->set(rb);
        }

        bool initialized()const
        {
            return mGenState.get();
        }

        u64 partyIdx() const
        {
            if (!mGenState)
                throw RTE_LOC;
            return mGenState->mPartyIdx;
        }

        macoro::task<> start()
        {
            mStarted.push_back(mGenState);
            return std::exchange(mGenState, nullptr)->start();
        }

        void startBatch(Batch* b)
        {
            mGenState->startBatch(b);
        }

        void abort()
        {
            for (auto& s : mStarted)
                if (auto genState = s.lock())
                    genState->abort();
        }


    private:

        std::shared_ptr<RequestState> request(CorType, u64 role, u64 size);

    };


    using OtRecvRequest = Request<OtRecv>;
    using OtSendRequest = Request<OtSend>;
    using BinOleRequest = Request<BinOle>;

}