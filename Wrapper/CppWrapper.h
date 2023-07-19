#pragma once
#include <string>
#include <vector>
#include "secure-join/Defines.h"

namespace secJoin
{
        struct State;

        void testApi(std::string& str);
        State* initState(std::string& csvPath, std::string& visaMetaDataPath, std::string& clientMetaDataPath,
                std::string& visaJoinCols, std::string& clientJoinCols, std::string& selectVisaCols,
                std::string& selectClientCols, bool isUnique);

        std::vector<u8> runJoin(State* stateAddress, std::vector<u8>& buff);
        void releaseState(State* memoryAddress);
        bool isProtocolReady(State* stateAddress);
        void getOtherShare(State* stateAddress, bool isUnique);
        void getJoinTable(State* stateAddress, std::string csvPath, std::string metaDataPath, bool isUnique);

}


