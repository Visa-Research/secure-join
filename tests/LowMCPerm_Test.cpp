#include "LowMCPerm_Test.h"
#include "util.h"
using namespace secJoin;

void LocMC_eval_test(const oc::CLP& cmd)
{
    u64 n = cmd.getOr("n", 100);

    oc::Matrix<u8> x(n, 16);// , x2Perm(n, rowSize);
    std::vector<oc::Matrix<u8>> k(13);// (n, LowMC2<>::getBlockSize());// , x2Perm(n, rowSize);

    for (u64 i = 0; i < k.size(); ++i)
    {
        k[i].resize(n, 16);
    }

    //auto cir = LowMCPerm::mLowMcCir();
    static oc::BetaCircuit cir;
    if (cir.mGates.size() == 0)
    {

        LowMC2<10,128,128,20>(false).to_enc_circuit(cir, true);
        cir.levelByAndDepth();
    }
    oc::PRNG prng0(oc::block(0, 0));
    oc::PRNG prng1(oc::block(0, 1));

    auto chls = coproto::LocalAsyncSocket::makePair();
    OleGenerator ole0, ole1;
    ole0.fakeInit(OleGenerator::Role::Sender);
    ole1.fakeInit(OleGenerator::Role::Receiver);
    Gmw gmw0, gmw1;
    gmw0.init(n, cir, ole0);
    gmw1.init(n, cir, ole1);

    gmw0.setInput(0, x);
    gmw0.setInput(1, x);
    gmw1.setZeroInput(0);
    gmw1.setZeroInput(1);

    for (u64 i = 0; i < k.size(); ++i)
    {
        gmw0.setInput(i + 2, k[i]);
        gmw1.setZeroInput(i + 2);
    }

    auto proto0 = gmw0.run(chls[0]);
    auto proto1 = gmw1.run(chls[1]);

    auto res = macoro::sync_wait(macoro::when_all_ready(std::move(proto0), std::move(proto1)));

    std::get<0>(res).result();
    std::get<1>(res).result();
    if (cmd.isSet("v"))
    {
        std::cout << chls[0].bytesReceived() / 1000.0 << " " << chls[0].bytesSent() / 1000.0 << " kB " << std::endl;
    }
}

void LowMCPerm_perm_test(const oc::CLP& cmd)
{
    // User input
    u64 n = 10;    // total number of rows
    u64 rowSize = cmd.getOr("m",63);      

    oc::Matrix<u8> x(n, rowSize), yExp(n,rowSize);

    LowMCPerm m1, m2;
    oc::PRNG prng(oc::block(0, 0));

    auto chls = coproto::LocalAsyncSocket::makePair();
    OleGenerator ole0, ole1;
    ole0.fakeInit(OleGenerator::Role::Sender);
    ole1.fakeInit(OleGenerator::Role::Receiver);


    // Initializing the vector x & permutation pi
    prng.get(x.data(), x.size());
    Perm pi(n,prng);

    for(auto invPerm : {false,true})
    {

        pi.apply<u8>(x, yExp, invPerm);

        std::array<oc::Matrix<u8>, 2> sout;
        sout[0].resize(n, rowSize);
        sout[1].resize(n, rowSize);

        auto proto0 = m1.apply<u8>(x, sout[0], prng, chls[0], ole0);
        auto proto1 = m2.apply<u8>(pi, sout[1], prng, chls[1], invPerm, ole1);

        auto res = macoro::sync_wait(macoro::when_all_ready(std::move(proto0), std::move(proto1)));

        std::get<0>(res).result();
        std::get<1>(res).result();

        auto yAct = reveal(sout[0], sout[1]);
        if(eq(yExp, yAct) == false)
            throw RTE_LOC;
    }
}

void LowMCPerm_secret_shared_input_perm_test()
{
    // User input
    u64 n = 1454;    // total number of rows
    u64 rowSize = 66;
    

    oc::Matrix<u8> x(n, rowSize), yExp(n,rowSize);

    LowMCPerm m1, m2;
    oc::PRNG prng(oc::block(0,0));

    auto chls = coproto::LocalAsyncSocket::makePair();

    Perm pi(n,prng);

    OleGenerator ole0, ole1;
    ole0.fakeInit(OleGenerator::Role::Sender);
    ole1.fakeInit(OleGenerator::Role::Receiver);

    // Initializing the vector x & permutation pi
    prng.get(x.data(), x.size());
    std::array<oc::Matrix<u8>, 2> xShares = share(x,prng);


    for(auto invPerm : {false,true})
    {
        pi.apply<u8>(x, yExp, invPerm);
        std::array<oc::Matrix<u8>, 2> sout;
        sout[0].resize(n, rowSize);
        sout[1].resize(n, rowSize);

        auto proto0 = m1.apply<u8>(xShares[0], sout[0], prng, chls[0], ole0);
        auto proto1 = m2.apply<u8>(pi, xShares[1], sout[1], prng, chls[1], invPerm, ole1);

        auto res = macoro::sync_wait(macoro::when_all_ready(std::move(proto0), std::move(proto1)));

        std::get<0>(res).result();
        std::get<1>(res).result();

        auto yAct = reveal(sout[0], sout[1]);
        if(!eq(yAct, yExp))
            throw RTE_LOC;
    }
}
