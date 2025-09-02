#include "LowMCPerm_Test.h"
#include "secure-join/Perm/LowMCPerm.h"
#include "secure-join/Util/Util.h"
#include "cryptoTools/Common/TestCollection.h"

using namespace secJoin;
template<size_t n>
auto& bytes(const std::bitset<n>&b)
{
	return *(std::array<u8, sizeof(b)>*)&b;
}

void LowMC_eval_test(const oc::CLP& cmd)
{
	u64 n = cmd.getOr("n", 10);
	auto sizeBlock = sizeof(LowMC2<>::block);
	oc::Matrix<u8> x(n, sizeBlock), y0(n, sizeBlock), y1(n, sizeBlock);

	//auto cir = LowMCPerm::mLowMcCir();
	static oc::BetaCircuit cir;
	if (cir.mGates.size() == 0)
	{

		LowMC2<>(false).to_enc_circuit(cir);
		cir.levelByAndDepth();
	}

	PRNG prng(oc::block(4523453, 0));
	LowMC2<> lowMc(false);
	LowMC2<>::keyblock key;
	for (u64 i = 0; i < key.size(); ++i)
		key[i] = prng.getBit();
	lowMc.set_key(key);
	std::vector<oc::Matrix<u8>> k(cir.mInputs.size() - 1);
	for (u64 i = 0; i < k.size(); ++i)
	{
		k[i].resize(n, sizeBlock);
		for (u64 j = 0; j < n; ++j)
			osuCrypto::copyBytes(k[i][j], bytes(lowMc.roundkeys[i]));
	}

	PRNG prng0(oc::block(0, 0));
	PRNG prng1(oc::block(0, 1));

	auto chls = coproto::LocalAsyncSocket::makePair();
	CorGenerator ole0, ole1;
	ole0.init(chls[0].fork(), prng0, 0, 1, 1 << 18, cmd.getOr("mock", 1));
	ole1.init(chls[1].fork(), prng1, 1, 1, 1 << 18, cmd.getOr("mock", 1));
	Gmw gmw0, gmw1;
	gmw0.init(n, cir, ole0);
	gmw1.init(n, cir, ole1);

	gmw0.setInput(0, x);
	gmw1.setZeroInput(0);

	for (u64 i = 0; i < k.size(); ++i)
	{
		gmw0.setInput(i + 1, k[i]);
		gmw1.setZeroInput(i + 1);
	}

	auto proto0 = gmw0.run(chls[0]);
	auto proto1 = gmw1.run(chls[1]);

	auto res = macoro::sync_wait(macoro::when_all_ready(
		std::move(proto0),
		std::move(proto1),
		ole0.start(), ole1.start()
	));
	std::get<0>(res).result();
	std::get<1>(res).result();

	gmw0.getOutput(0, y0);
	gmw1.getOutput(0, y1);

	for (u64 i = 0; i < n; ++i)
	{
		for (u64 j = 0; j < y0[i].size(); ++j)
			y0(i, j) ^= y1(i, j);

		LowMC2<>::block xx, zz;
		osuCrypto::copyBytes(bytes(xx), x[i]);
		osuCrypto::copyBytes(bytes(zz), y0[i]);
		auto yy = lowMc.encrypt(xx);
		if (yy != zz)
		{
			std::cout <<"\n" << yy << "\n" << zz << std::endl;
			throw RTE_LOC;
		}
	}

	if (cmd.isSet("v"))
	{
		std::cout << chls[0].bytesReceived() / 1000.0 << " " << chls[0].bytesSent() / 1000.0 << " kB " << std::endl;
	}
}

void LowMCPerm_perm_test(const oc::CLP& cmd)
{
	// User input
	u64 n = 10;    // total number of rows
	u64 rowSize = cmd.getOr("m", 63);

	oc::Matrix<u8> x(n, rowSize), yExp(n, rowSize);

	LowMCPermSender m1;
	LowMCPermReceiver m2;
	PRNG prng(oc::block(0, 0));

	auto chls = coproto::LocalAsyncSocket::makePair();
	CorGenerator ole0, ole1;


	// Initializing the vector x & permutation pi
	prng.get(x.data(), x.size());
	Perm pi(n, prng);


	ole0.init(chls[0].fork(), prng, 0, 1, 1 << 18, cmd.getOr("mock", 1));
	ole1.init(chls[1].fork(), prng, 1, 1, 1 << 18, cmd.getOr("mock", 1));
	pi.apply<u8>(x, yExp);

	m1.init(n, rowSize, ole0);
	m2.init(n, rowSize, ole1);

	PermCorSender send;
	PermCorReceiver recv;

	auto proto0 = m1.generate(pi, prng, chls[0], send);
	auto proto1 = m2.generate(prng, chls[1], recv);

	auto res = macoro::sync_wait(macoro::when_all_ready(
		std::move(proto0), std::move(proto1),
		ole0.start(), ole1.start()));

	std::get<0>(res).result();
	std::get<1>(res).result();

	validate(send, recv);
}
