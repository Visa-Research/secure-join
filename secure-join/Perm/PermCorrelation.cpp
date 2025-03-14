#include "PermCorrelation.h"


namespace secJoin
{
	macoro::task<> PermCorReceiver::validate(coproto::Socket& sock)
	{
		co_await sock.send(mA);
		co_await sock.send(mB);
	}

	macoro::task<> PermCorSender::validate(coproto::Socket& sock)
	{
		//assert(hasSetup());
		auto A = oc::Matrix<oc::block>{};
		auto B = oc::Matrix<oc::block>{};

		mPerm.validate();

		A.resize(mDelta.rows(), mDelta.cols());
		B.resize(mDelta.rows(), mDelta.cols());
		co_await sock.recv(A);
		co_await sock.recv(B);

		for (u64 i = 0; i < mPerm.size(); ++i)
		{
			for (u64 j = 0; j < A.cols(); ++j)
			{
				if ((B(i, j) ^ mDelta(i, j)) != A(mPerm[i], j))
					throw RTE_LOC;
			}
		}

	}

	namespace {
		void xorShare(
			oc::MatrixView<const oc::block> v1_,
			u64 byteOffset,
			oc::MatrixView<const oc::u8> v2,
			oc::MatrixView<oc::u8>& s)
		{
			auto v1 = matrixCast<const u8>(v1_);
			auto r = s.rows();
			auto c = s.cols();
			auto d = s.data();
			auto d2 = v2.data();
			for (oc::u64 i = 0; i < r; ++i)
			{
				auto d1 = v1.data(i) + byteOffset;
				for (oc::u64 j = 0; j < c; ++j)
					*d++ = *d1++ ^ *d2++;
			}
		}

		void xorShare(oc::MatrixView<const u8> v1,
			oc::MatrixView<const oc::u8> v2,
			oc::MatrixView<oc::u8>& s)
		{
			// Checking the dimensions
			if (v1.rows() != v2.rows())
				throw RTE_LOC;
			if (v1.cols() != v2.cols())
				throw RTE_LOC;
			for (oc::u64 i = 0; i < v1.size(); ++i)
				s(i) = v1(i) ^ v2(i);
		}

	}

	template <>
	macoro::task<> PermCorSender::apply<u8>(
		PermOp op,
		oc::MatrixView<u8> sout,
		coproto::Socket& chl)
	{
		auto xEncrypted = oc::Matrix<u8>();
		auto xPermuted = oc::Matrix<u8>();
		auto delta = Perm{};

		if (size() == 0)
			throw std::runtime_error("not initalized" LOCATION);

		if (size() != sout.rows())
			throw std::runtime_error("output rows does not match init(). " LOCATION);

		if (corSize() < sout.cols())
			throw std::runtime_error("insufficient correlated randomness");

		xPermuted.resize(size(), sout.cols());
		xEncrypted.resize(size(), sout.cols());

		co_await chl.recv(xEncrypted);

		if (op == PermOp::Regular)
		{
			mPerm.apply<u8>(xEncrypted, xPermuted, op);
			xorShare(mDelta, mByteOffset, xPermuted, sout);
		}
		else
		{
			xorShare(mDelta, mByteOffset, xEncrypted, xEncrypted);
			mPerm.apply<u8>(xEncrypted, sout, op);
		}

		mByteOffset += sout.cols();
	}



	// If AltMod receiver only wants to call apply
	// when it also has inputs
	// this will internally call setup for it
	template <>
	macoro::task<> PermCorSender::apply<u8>(
		PermOp op,
		oc::MatrixView<const u8> in,
		oc::MatrixView<u8> sout,
		coproto::Socket& chl)
	{
		auto xPermuted = oc::Matrix<u8>();

		if (size() == 0)
			throw std::runtime_error("not initalized" LOCATION);

		xPermuted.resize(in.rows(), in.cols());

		co_await apply(op, sout, chl);

		mPerm.apply<u8>(in, xPermuted, op);
		xorShare(xPermuted, sout, sout);
	}

	// If AltMod sender only wants to call apply
	// this will internally call setup for it
	template <>
	macoro::task<> PermCorReceiver::apply<u8>(
		PermOp op,
		oc::MatrixView<const u8> input,
		oc::MatrixView<u8> sout,
		coproto::Socket& chl)
	{
		auto xEncrypted = oc::Matrix<u8>();
		auto delta = Perm{};

		if (input.rows() != size() ||
			sout.rows() != size() ||
			input.cols() != sout.cols())
			throw std::runtime_error("input output size mismatch. " LOCATION);

		if (size() == 0)
			throw std::runtime_error("init has not been called. " LOCATION);

		if (corSize() < sout.cols())
			throw std::runtime_error("insufficient correlated randomness");

		// co_await apply(input, sout, chl));
		xEncrypted.resize(input.rows(), input.cols());

		if (op == PermOp::Regular)
		{
			xorShare(mA, mByteOffset, input, xEncrypted);
			co_await chl.send(std::move(xEncrypted));
			for (u64 i = 0; i < size(); ++i)
			{
				std::copy(
					(u8*)mB.data(i) + mByteOffset,
					(u8*)mB.data(i) + mByteOffset + sout.cols(),
					sout.data(i)
				);
				//m emcpy(sout.data(i), (u8*)mB.data(i) + mByteOffset, sout.cols());
			}
		}
		else
		{
			xorShare(mB, mByteOffset, input, xEncrypted);
			co_await chl.send(std::move(xEncrypted));
			for (u64 i = 0; i < size(); ++i)
			{
				std::copy(
					(u8*)mA.data(i) + mByteOffset,
					(u8*)mA.data(i) + mByteOffset + sout.cols(),
					sout.data(i)
				);
				//m emcpy(sout.data(i), (u8*)mA.data(i) + mByteOffset, sout.cols());
			}
		}

		mByteOffset += sout.cols();
	}
}