#pragma once
#include "secure-join/Defines.h"
#include "secure-join/CorGenerator/Base.h"

#include "cryptoTools/Common/Aligned.h"
#include "cryptoTools/Common/BitVector.h"

#include <vector>
#include <memory>

#include "macoro/task.h"
#include "macoro/channel.h"
#include "macoro/macros.h"
#include "macoro/manual_reset_event.h"
#include "macoro/variant.h"

#include "libOTe/TwoChooseOne/Silent/SilentOtExtSender.h"
#include "libOTe/TwoChooseOne/Silent/SilentOtExtReceiver.h"

#include "Batch.h"
#include "Correlations.h"

namespace secJoin
{


    // a batch representing binary OLE correlation
    // a * b = [c]
    struct OleBatch : Batch
    {
        OleBatch(GenState* state, bool sender, oc::Socket&& s, PRNG&& p);

        // The "send" specific state
        struct SendBatch
        {
            // The OT Sender
            oc::SilentOtExtSender mSender;
            // The OT send messages
            ;

            // return the task that generate the Sender correlation.
            macoro::task<> sendTask(
                GenState* state,
                u64 batchIdx,
                u64 size,
                PRNG& prng,
                oc::Socket& sock,
                oc::AlignedUnVector<oc::block>& add,
                oc::AlignedUnVector<oc::block>& mult,
                macoro::async_manual_reset_event& corReady,
                BatchThreadState& threadState);

            // The routine that compresses the sender's OT messages
            // into OLEs. Basically, it just tasks the LSB of the OTs.
            void compressSender(
                block delta,
                span<oc::block> sendMsg,
                span<oc::block> add,
                span<oc::block> mult);

            void mock(u64 batchIdx,
                span<oc::block> add,
                span<oc::block> mult);

        };


        // The "specific" specific state
        struct RecvBatch
        {
            // The OT receiver
            oc::SilentOtExtReceiver mReceiver;

            // return the task that generate the Sender correlation.
            macoro::task<> recvTask(
                GenState* state,
                u64 batchIdx,
                u64 size,
                PRNG& prng,
                oc::Socket& sock,
                oc::AlignedUnVector<oc::block>& add,
                oc::AlignedUnVector<oc::block>& mult,
                macoro::async_manual_reset_event& corReady,
                BatchThreadState& threadState);

            // The routine that compresses the sender's OT messages
            // into OLEs. Basically, it just tasks the LSB of the OTs.
            void compressRecver(span<oc::block> recvMsg, span<oc::block> add, span<oc::block> mult);

            void mock(u64 batchIdx, span<oc::block> add, span<oc::block> mult);

        };

        macoro::variant<SendBatch, RecvBatch> mSendRecv;

        oc::AlignedUnVector<oc::block> mAdd, mMult;

        void getCor(Cor* c, u64 begin, u64 size) override;

        BaseRequest getBaseRequest() override;

        void setBase(BaseCor& sMsg) override;

        // Get the task associated with this batch.
        macoro::task<> getTask(BatchThreadState&) override;

        //void mock(u64 batchIdx) override;

        u64 getBatchTypeId() override
        {
            return mSendRecv.index();
        }

        void clear() override
        {
            mAdd = {};
            mMult = {};
        }
    };


}