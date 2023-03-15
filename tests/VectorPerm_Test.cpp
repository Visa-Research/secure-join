#include "VectorPerm_Test.h"


using namespace secJoin;

void VectorPerm_basic_test()
{
    // User input
    u64 n = 5;    // total number of rows
    u64 rowSize = 1;

    oc::Matrix<u8> x(n,rowSize),yExp(n,rowSize);
    oc::PRNG prng(oc::block(0,0));
    auto chls = coproto::LocalAsyncSocket::makePair();

    // Generating a random s
    Perm mPerm(n, prng);
    prng.get(x.data(), x.size());

    
    // Secret Sharing s
    std::array<std::vector<u64>, 2> sShares = share(mPerm.mPerm, prng);
    std::array<oc::Matrix<u8>, 2> yShare;

    VectorPerm 
        vecPerm1(sShares[0], prng, 0),
        vecPerm2(sShares[1], prng, 1);

    auto proto0 = vecPerm1.setup(chls[0]);
    auto proto1 = vecPerm2.setup(chls[1]);

    auto res = macoro::sync_wait(macoro::when_all_ready(std::move(proto0), std::move(proto1)));

    try{
        std::get<0>(res).result();
    }
    catch(std::exception &ex)
    {
        std::cout << ex.what() << std::endl;
        std::get<1>(res).result();
        throw ex;
    }
    std::get<1>(res).result();

    // Checking if both the parties have same rho
    if(vecPerm1.mRho != vecPerm2.mRho)
        throw RTE_LOC;
    
    // Creating pi from pi1 & pi2
    auto pi = vecPerm1.mPi.mPerm.compose(vecPerm2.mPi.mPerm);
    Perm rhoExp = pi.apply(mPerm.mPerm);

    // Checking if pi1 & pi2 are applied correctly
    if(rhoExp != vecPerm1.mRho)
        throw RTE_LOC;
    
    // Secret Sharing x
    std::array<oc::Matrix<u8>, 2> xShares = share(x,prng);

    proto0 = vecPerm1.apply(xShares[0], yShare[0], prng, chls[0]);
    proto1 = vecPerm2.apply(xShares[1], yShare[1], prng, chls[1]);

    auto res1 = macoro::sync_wait(macoro::when_all_ready(std::move(proto0), std::move(proto1)));

    std::get<0>(res1).result();
    std::get<1>(res1).result();

    auto yAct = reconstruct_from_shares(yShare[0], yShare[1]);

    // Getting x permutated under s
    mPerm.apply<u8>(x,yExp);

    if(!eq(yAct, yExp))
        throw RTE_LOC;

}
