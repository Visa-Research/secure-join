#include "Average_Test.h"

//#include "nlohmann/json.hpp"

using namespace secJoin;


void Average_concatColumns_Test()
{
    u64 n0 = 2345;
    Table t0;
    t0.mColumns.emplace_back("c0", TypeID::IntID, 11);
    t0.mColumns.emplace_back("c1", TypeID::IntID, 31);
    t0.mColumns.emplace_back("c2", TypeID::IntID, 5);
    t0.mIsActive.resize(n0);
    t0.resize(n0);

    auto sock = coproto::LocalAsyncSocket::makePair();

    Average avg;

    PRNG prng(oc::ZeroBlock);
    for (u64 i = 0; i < t0.mColumns.size(); ++i)
    {
        prng.get(t0[i].mCol.mData.data(), t0[i].mCol.mData.size());
        t0[i].mCol.mData.trim();
    }
    
    for (u64 i = 0; i < t0.rows(); ++i)
        t0.mIsActive[i] = 1;

    ColRef groupByCol = t0[0];
    std::vector<ColRef> avgCols = { t0[1], t0[2] };
    BinMatrix compressKeys, y;
    
    CorGenerator ole;
    ole.init(sock[0].fork(), prng, 1);

    avg.init(groupByCol, avgCols, ole, false);    
    avg.loadKeys(groupByCol, t0.mIsActive, compressKeys);
    avg.concatColumns(groupByCol, avgCols, t0.mIsActive, compressKeys, y);

    BinMatrix ones(n0, sizeof(oc::u64) * 8);
    for (oc::u64 i = 0; i < n0; i++)
        ones(i, 0) = 1;

    // Validation logic
    for (u64 i = 0; i < n0; ++i)
    {
        auto iter = oc::BitIterator(y.mData[i].data());
        
        // Checking the Average Columns
        for (u64 j = 0; j < avgCols.size(); ++j)
        {
            auto expIter = oc::BitIterator(avgCols[j].mCol.mData.data(i));
            for (u64 k = 0; k < avgCols[j].mCol.getBitCount(); ++k)
            {
                u8 exp = *expIter++;
                u8 act = *iter++;
                if (exp != act)
                    throw RTE_LOC;
            }

            auto rem = avgCols[j].mCol.getBitCount() % 8;
            if (rem)
                iter = iter + (8 - rem);
        }

        // Checking the ones column
        auto expIter = oc::BitIterator(ones.data(i));

        for (u64 k = 0; k < ones.mBitCount; ++k)
        {
            u8 exp = *expIter++;
            u8 act = *iter++;
            if (exp != act)
                throw RTE_LOC;
        }
        auto rem = ones.bitsPerEntry() % 8;
        if (rem)
            iter = iter + (8 - rem);

        // Checking the groupby column
        expIter = oc::BitIterator(groupByCol.mCol.mData.data(i));

        for (u64 k = 0; k < groupByCol.mCol.mBitCount; ++k)
        {
            u8 exp = *expIter++;
            u8 act = *iter++;
            if (exp != act)
                throw RTE_LOC;
        }
        rem = groupByCol.mCol.getBitCount() % 8;
        if (rem)
            iter = iter + (8 - rem);

        // Checking the compressKeys
        expIter = oc::BitIterator(compressKeys.data(i));

        for (u64 k = 0; k < compressKeys.bitsPerEntry(); ++k)
        {
            u8 exp = *expIter++;
            u8 act = *iter++;
            if (exp != act)
                throw RTE_LOC;
        }
        rem = compressKeys.bitsPerEntry() % 8;
        if (rem)
            iter = iter + (8 - rem);

        // Checking the active Flag
        u8 exp = t0.mIsActive[i];
        u8 act = *iter++;
        if (exp != act)
            throw RTE_LOC;

    }
}



void Average_getControlBits_Test(const oc::CLP& cmd)
{
    u64 n = 342;
    u64 keyBitCount = 21;
    auto mock = cmd.getOr("mock", 1);

    auto sock = coproto::LocalAsyncSocket::makePair();

    BinMatrix keys(n, keyBitCount), kk[2], cc[2];
    PRNG prng(oc::ZeroBlock);
    prng.get(keys.data(), keys.size());

    std::vector<u8> exp(n);
    for (u64 i = 1; i < n; ++i)
    {
        exp[i] = prng.getBit();
        if (exp[i])
        {
            memcpy(keys.data(i), keys.data(i - 1), keys.cols());
        }
    }

    share(keys, kk[0], kk[1], prng);

    CorGenerator ole0, ole1;
    ole0.init(sock[0].fork(), prng, 0, 1 << 16, mock);
    ole1.init(sock[1].fork(), prng, 1, 1 << 16, mock);

    Average avg[2];

    avg[0].mControlBitGmw.init(n, OmJoin::getControlBitsCircuit(keyBitCount), ole0);
    avg[1].mControlBitGmw.init(n, OmJoin::getControlBitsCircuit(keyBitCount), ole1);

    auto r = macoro::sync_wait(macoro::when_all_ready(
        ole0.start(),
        ole1.start(),
        avg[0].getControlBits(kk[0], sock[0], cc[0]),
        avg[1].getControlBits(kk[1], sock[1], cc[1])));

    std::get<0>(r).result();
    std::get<1>(r).result();

    auto c = reveal(cc[0], cc[1]);

    if (c.mData(0))
        throw RTE_LOC;
    for (u64 i = 1; i < n; ++i)
    {
        auto act = c(i);
        if (exp[i] != act)
            throw RTE_LOC;
    }
}

void Average_avg_Test(const oc::CLP& cmd)
{
    u64 nT = cmd.getOr("nT", 10);
    Table T;

    bool printSteps = cmd.isSet("print");
    bool mock = cmd.getOr("mock", 1);

    T.init(nT, { {
        {"L1", TypeID::IntID, 12},
        {"L2", TypeID::IntID, 16},
        {"L3", TypeID::IntID, 16}
    } });

    for (u64 i = 0; i < nT; ++i)
    {
        T.mColumns[0].mData.mData(i, 0) = i % 5;
        T.mColumns[1].mData.mData(i, 0) = i % 4;
        T.mColumns[1].mData.mData(i, 1) = i % 4;
        T.mColumns[2].mData.mData(i, 0) = i % 4;
        T.mColumns[2].mData.mData(i, 1) = i % 4;
    }

    auto sock = coproto::LocalAsyncSocket::makePair();

    PRNG prng0(oc::ZeroBlock);
    PRNG prng1(oc::OneBlock);

    std::array<Table, 2> Ts;
    share(T, Ts, prng0);

    Ts[0].mIsActive.resize(nT);
    Ts[1].mIsActive.resize(nT);
    for (u64 i = 0; i < nT; i++)
    {
        Ts[0].mIsActive[i] = 1;
        Ts[1].mIsActive[i] = 0;
    }
//    std::cout << "OG\n" << T << std::endl;
    auto exp = average(T[0], { T[1], T[2] });
//    std::cout << "exp\n" << exp << std::endl;

    CorGenerator ole0, ole1;
    ole0.init(sock[0].fork(), prng0, 0, 1 << 16, mock);
    ole1.init(sock[1].fork(), prng1, 1, 1 << 16, mock);

    for (auto remDummies : { false })
    {
        Average avg0, avg1;

        avg0.init(Ts[0][0], { Ts[0][1], Ts[0][2] }, ole0, remDummies, printSteps, mock);
        avg1.init(Ts[1][0], { Ts[1][1], Ts[1][2] }, ole1, remDummies, printSteps, mock);


        Perm p0(exp.rows(), prng0);
        Perm p1(exp.rows(), prng1);
        Perm pi = p0.composeSwap(p1);

        Table out[2];
        auto r = macoro::sync_wait(macoro::when_all_ready(
            ole0.start(), ole1.start(),
            avg0.avg(Ts[0][0], { Ts[0][1], Ts[0][2] }, out[0], prng0, sock[0], remDummies, p0),
            avg1.avg(Ts[1][0], { Ts[1][1], Ts[1][2] }, out[1], prng1, sock[1], remDummies, p1)
             )
        );

        std::get<0>(r).result();
        std::get<1>(r).result();
        std::get<2>(r).result();
        std::get<3>(r).result();

        Table res;

        res = reveal(out[0], out[1], remDummies);

        if (remDummies)
        {
            Table tmp = applyPerm(exp, pi);
            std::swap(exp, tmp);
        }

        if (res != exp)
        {
            std::cout << "remove dummies flag = " << remDummies << std::endl;
            std::cout << "exp \n" << exp << std::endl;
            std::cout << "act \n" << res << std::endl;
            std::cout << "ful \n" << reveal(out[0], out[1], false) << std::endl;
            throw RTE_LOC;
        }
    }

}

void Average_avg_BigKey_Test(const oc::CLP& cmd)
{
    u64 nT = cmd.getOr("nT", 10);
    Table T;

    bool printSteps = cmd.isSet("print");
    bool mock = cmd.getOr("mock", 1);

    T.init(nT, { {
        {"L1", TypeID::IntID, 100},
        {"L2", TypeID::IntID, 16},
        {"L3", TypeID::IntID, 16}
    } });

    for (u64 i = 0; i < nT; ++i)
    {
        T.mColumns[0].mData.mData(i, 0) = i % 5;
        T.mColumns[1].mData.mData(i, 0) = i % 4;
        T.mColumns[1].mData.mData(i, 1) = i % 4;
        T.mColumns[2].mData.mData(i, 0) = i % 4;
        T.mColumns[2].mData.mData(i, 1) = i % 4;
    }

    auto sock = coproto::LocalAsyncSocket::makePair();

    PRNG prng0(oc::ZeroBlock);
    PRNG prng1(oc::OneBlock);

    std::array<Table, 2> Ts;
    share(T, Ts, prng0);

    Ts[0].mIsActive.resize(nT);
    Ts[1].mIsActive.resize(nT);
    for (u64 i = 0; i < nT; i++)
    {
        Ts[0].mIsActive[i] = 1;
        Ts[1].mIsActive[i] = 0;
    }

    auto exp = average(T[0], { T[1], T[2] });

    CorGenerator ole0, ole1;
    ole0.init(sock[0].fork(), prng0, 0, 1 << 16, mock);
    ole1.init(sock[1].fork(), prng1, 1, 1 << 16, mock);

    for (auto remDummies : { false })
    {
        Average avg0, avg1; 

        avg0.init(Ts[0][0], { Ts[0][1], Ts[0][2] }, ole0, remDummies, printSteps, mock);
        avg1.init(Ts[1][0], { Ts[1][1], Ts[1][2] }, ole1, remDummies, printSteps, mock);


        Perm p0(exp.rows(), prng0);
        Perm p1(exp.rows(), prng1);
        Perm pi = p0.composeSwap(p1);

        Table out[2];
        auto r = macoro::sync_wait(macoro::when_all_ready(
            ole0.start(), ole1.start(),
            avg0.avg(Ts[0][0], { Ts[0][1], Ts[0][2] }, out[0], prng0, sock[0], remDummies, p0),
            avg1.avg(Ts[1][0], { Ts[1][1], Ts[1][2] }, out[1], prng1, sock[1], remDummies, p1)
            
             )
        );

        std::get<0>(r).result();
        std::get<1>(r).result();
        std::get<2>(r).result();
        std::get<3>(r).result();

        Table res;

        res = reveal(out[0], out[1], remDummies);

        if (remDummies)
        {
            Table tmp = applyPerm(exp, pi);
            std::swap(exp, tmp);
        }

        if (res != exp)
        {
            std::cout << "remove dummies flag = " << remDummies << std::endl;
            std::cout << "exp \n" << exp << std::endl;
            std::cout << "act \n" << res << std::endl;
            std::cout << "ful \n" << reveal(out[0], out[1], false) << std::endl;
            throw RTE_LOC;
        }
    }

}

void Average_avg_csv_Test(const oc::CLP& cmd)
{
    std::string rootPath(SEC_JOIN_ROOT_DIRECTORY);
    std::string visaCsvPath = rootPath + "/tests/tables/visa.csv";
    std::string bankCsvPath = rootPath + "/tests/tables/bank.csv";
    std::string visaMetaDataPath = rootPath + "/tests/tables/visa_meta.txt";
    std::string clientMetaDataPath = rootPath + "/tests/tables/bank_meta.txt";
    std::string joinCsvPath = rootPath + "/tests/tables/joindata.csv";
    std::string joinMetaPath = rootPath + "/tests/tables/joindata_meta.txt";
    std::vector<oc::i64> opInfo{ 2, 0, 3, 4, 1, 0, 4, 5, 1, 0, 2, 1, 4 };
    bool printSteps = cmd.isSet("print");
    bool mock = cmd.getOr("mock", 1);

    std::vector<oc::u64> joinCols, selectCols, groupByCols, avgCols;
    u64 startIndex = 0;
    parseColsArray(joinCols, selectCols, groupByCols, avgCols, opInfo, startIndex, printSteps);
    updateSelectCols(selectCols, groupByCols, avgCols, printSteps);

    oc::u64 lRowCount = 0, rRowCount = 0, lColCount = 0, rColCount = 0;
    bool isBin;

    std::vector<ColumnInfo> lColInfo, rColInfo;
    getFileInfo(visaMetaDataPath, lColInfo, lRowCount, lColCount, isBin);
    getFileInfo(clientMetaDataPath, rColInfo, rRowCount, rColCount, isBin);

    Table L, R;

    L.init(lRowCount, lColInfo);
    R.init(rRowCount, rColInfo);

    populateTable(L, visaCsvPath, lRowCount, isBin);
    populateTable(R, bankCsvPath, rRowCount, isBin);

    // Get Select Col Refs
    std::vector<secJoin::ColRef> selectColRefs = getSelectColRef(selectCols, L, R);

    // if (printSteps)
    // {
    //     std::cout << "L\n" << L << std::endl;
    //     std::cout << "R\n" << R << std::endl;
    // }

    PRNG prng(oc::ZeroBlock);
    std::array<Table, 2> Ls, Rs;
    share(L, Ls, prng);
    share(R, Rs, prng);

    CorGenerator ole0, ole1;
    PRNG prng0(oc::ZeroBlock);
    PRNG prng1(oc::OneBlock);
    auto sock = coproto::LocalAsyncSocket::makePair();
    ole0.init(sock[0].fork(), prng0, 0, 1 << 16, mock);
    ole1.init(sock[1].fork(), prng1, 1, 1 << 16, mock);

    for (auto remDummies : { false, true })
    {
        OmJoin join0, join1;

        join0.mInsecurePrint = printSteps;
        join1.mInsecurePrint = printSteps;

        join0.mInsecureMockSubroutines = mock;
        join1.mInsecureMockSubroutines = mock;

        u64 lJoinColIndex = joinCols[0];
        u64 rJoinColIndex = getRColIndex(joinCols[1], lColCount, rColCount);

        throw RTE_LOC;//not impl.
        // auto joinExp = join(L[lJoinColIndex], R[rJoinColIndex], selectColRefs);
        // auto query0 = JoinQuery(Ls[0][0], Rs[0][1], { Ls[0][0], Rs[0][2], Ls[0][1] });
        // auto query1 = JoinQuery(Ls[1][0], Rs[1][1], { Ls[1][0], Rs[1][2], Ls[1][1] });

        // join0.init(query0, ole0, remDummies);
        // join1.init(query1, ole1, remDummies);


        // std::vector<secJoin::ColRef> lSelectColRefs = getSelectColRef(selectCols, Ls[0], Rs[0]);
        // std::vector<secJoin::ColRef> rSelectColRefs = getSelectColRef(selectCols, Ls[1], Rs[1]);

        // auto r = macoro::sync_wait(macoro::when_all_ready(
        //     join0.join(Ls[0][lJoinColIndex], Rs[0][rJoinColIndex], lSelectColRefs,
        //         tempOut[0], prng0, ole0, sock[0], remDummies, p2),
        //     join1.join(Ls[1][lJoinColIndex], Rs[1][rJoinColIndex], rSelectColRefs,
        //         tempOut[1], prng1, ole1, sock[1], remDummies, p3)
        // ));
        // std::get<0>(r).result();
        // std::get<1>(r).result();

        // auto res = reveal(tempOut[0], tempOut[1]);

        // if (remDummies)
        // {
        //     Table tmp = applyPerm(joinExp, pi1);
        //     std::swap(joinExp, tmp);
        // }

        // if (res != joinExp)
        // {
        //     std::cout << "exp \n" << joinExp << std::endl;
        //     std::cout << "act \n" << res << std::endl;
        //     // std::cout << "ful \n" << reveal(out[0], out[1], false) << std::endl;
        //     throw RTE_LOC;
        // }


        // // Create a new mapping and store the new mapping in the cState
        // std::unordered_map<oc::u64, oc::u64> map;
        // createNewMapping(map, selectCols);

        // std::vector<secJoin::ColRef> avgColRefs = getColRefFromMapping(map, avgCols, joinExp);
        // // Assuming we have only one groupby column
        // oc::u64 groupByColIndex = getMapVal(map, groupByCols[0]);
        // auto avgExp = average(joinExp[groupByColIndex], avgColRefs);


        // Perm p0(avgExp.rows(), prng0);
        // Perm p1(avgExp.rows(), prng1);
        // Perm pi = p0.composeSwap(p1);

        // Average avg1, avg2;

        // avg1.mInsecurePrint = printSteps;
        // avg2.mInsecurePrint = printSteps;

        // avg1.mInsecureMockSubroutines = mock;
        // avg2.mInsecureMockSubroutines = mock;

        // std::vector<secJoin::ColRef> lAvgColRefs = getColRefFromMapping(map, avgCols, tempOut[0]);
        // std::vector<secJoin::ColRef> rAvgColRefs = getColRefFromMapping(map, avgCols, tempOut[1]);

        // auto r1 = macoro::sync_wait(macoro::when_all_ready(
        //     avg1.avg(tempOut[0][groupByColIndex], lAvgColRefs, out[0],
        //         prng0, ole0, sock[0], remDummies, p0),
        //     avg2.avg(tempOut[1][groupByColIndex], rAvgColRefs, out[1],
        //         prng1, ole1, sock[1], remDummies, p1)
        // ));
        // std::get<1>(r1).result();
        // std::get<0>(r1).result();

        // auto res1 = reveal(out[0], out[1]);

        // if (remDummies)
        // {
        //     Table tmp = applyPerm(avgExp, pi);
        //     std::swap(avgExp, tmp);
        // }


        // if (res1 != avgExp)
        // {
        //     std::cout << "exp \n" << avgExp << std::endl;
        //     std::cout << "act \n" << res1 << std::endl;
        //     // std::cout << "ful \n" << reveal(out[0], out[1], false) << std::endl;
        //     throw RTE_LOC;
        // }

        // // if (cmd.isSet("timing"))
        // //     std::cout << timer << std::endl;
    }
}
