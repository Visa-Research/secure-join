#include "AdditivePerm_Test.h"

using namespace secJoin;

void AdditivePerm_setup_test()
{

}

void AdditivePerm_xor_test()
{
    // User input
    u64 n = 500;    // total number of rows
    u64 rowSize = 11;

    oc::Matrix<u8> x(n,rowSize),yExp(n,rowSize);
    oc::PRNG prng(oc::block(0,0));
    auto chls = coproto::LocalAsyncSocket::makePair();

    Perm mPerm(n, prng);
    prng.get(x.data(), x.size());

    // Secret Sharing s
    std::array<std::vector<u32>, 2> sShares = xorShare(mPerm.mPerm, prng);
    std::array<oc::Matrix<u8>, 2> yShare;

    AdditivePerm 
        vecPerm1(sShares[0], prng, 0),
        vecPerm2(sShares[1], prng, 1);

    OleGenerator ole0, ole1;
    ole0.fakeInit(OleGenerator::Role::Sender);
    ole1.fakeInit(OleGenerator::Role::Receiver);

    oc::block kk;
    kk = prng.get();

    // Setuping up the OT Keys
    std::vector<oc::block> rk(vecPerm1.mPi.mDlpnPerm.mSender.mPrf.KeySize);
    std::vector<std::array<oc::block, 2>> sk(vecPerm1.mPi.mDlpnPerm.mSender.mPrf.KeySize);
    for (u64 i = 0; i < vecPerm1.mPi.mDlpnPerm.mSender.mPrf.KeySize; ++i)
    {
        sk[i][0] = oc::block(i, 0);
        sk[i][1] = oc::block(i, 1);
        rk[i] = oc::block(i, *oc::BitIterator((u8*)&kk, i));
    }
    vecPerm2.setupDlpnSender(kk,rk);
    vecPerm1.setupDlpnReceiver(sk);

    vecPerm1.setupDlpnSender(kk,rk);
    vecPerm2.setupDlpnReceiver(sk);

    auto proto0 = vecPerm1.setup(chls[0], ole0, prng);
    auto proto1 = vecPerm2.setup(chls[1], ole1, prng);

    auto res = macoro::sync_wait(macoro::when_all_ready(std::move(proto0), std::move(proto1)));

    std::get<0>(res).result();
    std::get<1>(res).result();

    if(vecPerm1.mRho != vecPerm2.mRho)
        throw RTE_LOC;
    
    auto pi = vecPerm1.mPi.mPerm.compose(vecPerm2.mPi.mPerm);
    Perm rhoExp = pi.apply(mPerm.mPerm);
    if(rhoExp != vecPerm1.mRho)
        throw RTE_LOC;
    // Secret Sharing x
    std::array<oc::Matrix<u8>, 2> xShares = share(x,prng);
    yShare[0].resize(x.rows(), x.cols());
    yShare[1].resize(x.rows(), x.cols());

    proto0 = vecPerm1.apply<u8>(xShares[0], yShare[0], prng, chls[0], ole0);
    proto1 = vecPerm2.apply<u8>(xShares[1], yShare[1], prng, chls[1], ole1);

    auto res1 = macoro::sync_wait(macoro::when_all_ready(std::move(proto0), std::move(proto1)));

    std::get<0>(res1).result();
    std::get<1>(res1).result();

    auto yAct = reveal(yShare[0], yShare[1]);
    mPerm.apply<u8>(x,yExp);

    if(!eq(yAct, yExp))
        throw RTE_LOC;
    

    auto res2 = macoro::sync_wait(macoro::when_all_ready(
        vecPerm1.apply<u8>(xShares[0], yShare[0], prng, chls[0], ole0, true),
        vecPerm2.apply<u8>(xShares[1], yShare[1], prng, chls[1], ole1, true)
    ));
    std::get<0>(res2).result();
    std::get<1>(res2).result();



    yAct = reveal(yShare[0], yShare[1]);
    mPerm.apply<u8>(x, yExp, true);

    if (!eq(yAct, yExp))
        throw RTE_LOC;
}


void AdditivePerm_add_test()
{

    //// User input
    //u64 n = 500;    // total number of rows
    //u64 rowSize = 11;

    //oc::Matrix<u32> x(n, rowSize), yExp(n, rowSize);
    //oc::PRNG prng(oc::block(0, 0));
    //auto chls = coproto::LocalAsyncSocket::makePair();

    //Perm mPerm(n, prng);
    //prng.get(x.data(), x.size());

    //// Secret Sharing s
    //std::array<std::vector<u32>, 2> sShares;
    //share(mPerm.mPerm, sShares[0], sShares[1], prng);
    //std::array<oc::Matrix<u32>, 2> yShare;

    //AdditivePerm
    //    vecPerm1(sShares[0], prng, 0, AdditivePerm::Type::Add),
    //    vecPerm2(sShares[1], prng, 1, AdditivePerm::Type::Add);

    //OleGenerator ole0, ole1;
    //ole0.fakeInit(OleGenerator::Role::Sender);
    //ole1.fakeInit(OleGenerator::Role::Receiver);

    //auto proto0 = vecPerm1.setup(chls[0], ole0, prng);
    //auto proto1 = vecPerm2.setup(chls[1], ole1, prng);

    //auto res = macoro::sync_wait(macoro::when_all_ready(std::move(proto0), std::move(proto1)));

    //std::get<0>(res).result();
    //std::get<1>(res).result();

    //if (vecPerm1.mRho != vecPerm2.mRho)
    //    throw RTE_LOC;

    //auto pi = vecPerm1.mPi.mPerm.compose(vecPerm2.mPi.mPerm);
    //Perm rhoExp = pi.apply(mPerm.mPerm);
    //if (rhoExp != vecPerm1.mRho)
    //    throw RTE_LOC;
    //// Secret Sharing x
    //std::array<oc::Matrix<u32>, 2> xShares = share(x, prng);

    //proto0 = vecPerm1.apply(xShares[0], yShare[0], prng, chls[0], ole0);
    //proto1 = vecPerm2.apply(xShares[1], yShare[1], prng, chls[1], ole1);

    //auto res1 = macoro::sync_wait(macoro::when_all_ready(std::move(proto0), std::move(proto1)));

    //std::get<0>(res1).result();
    //std::get<1>(res1).result();

    //auto yAct = reveal(yShare[0], yShare[1]);
    //mPerm.apply<u32>(x, yExp);

    //if (!eq(yAct, yExp))
    //    throw RTE_LOC;


    //auto res2 = macoro::sync_wait(macoro::when_all_ready(
    //    vecPerm1.apply(xShares[0], yShare[0], prng, chls[0], ole0, true),
    //    vecPerm2.apply(xShares[1], yShare[1], prng, chls[1], ole1, true)
    //));
    //std::get<0>(res2).result();
    //std::get<1>(res2).result();



    //yAct = reveal(yShare[0], yShare[1]);
    //mPerm.apply<u32>(x, yExp, true);

    //if (!eq(yAct, yExp))
    //    throw RTE_LOC;
}
