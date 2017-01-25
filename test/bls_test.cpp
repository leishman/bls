#include <bls.hpp>
#include <cybozu/test.hpp>
#include <cybozu/inttype.hpp>
#include <iostream>
#include <sstream>

template<class T>
void streamTest(const T& t)
{
	std::ostringstream oss;
	oss << t;
	std::istringstream iss(oss.str());
	T t2;
	iss >> t2;
	CYBOZU_TEST_EQUAL(t, t2);
}

CYBOZU_TEST_AUTO(bls)
{
	bls::init();
	bls::SecretKey sec;
	sec.init();
	streamTest(sec);
	bls::PublicKey pub;
	sec.getPublicKey(pub);
	streamTest(pub);
	for (int i = 0; i < 5; i++) {
		std::string m = "hello";
		m += char('0' + i);
		bls::Sign s;
		sec.sign(s, m);
		CYBOZU_TEST_ASSERT(s.verify(pub, m));
		CYBOZU_TEST_ASSERT(!s.verify(pub, m + "a"));
		streamTest(s);
	}
}

template<class T>
void testSet()
{
	/*
		mask value to be less than r if the value >= (1 << (192 + 62))
	*/
	const uint64_t fff = uint64_t(-1);
	const uint64_t one = uint64_t(1);
	const struct {
		uint64_t in;
		uint64_t expected;
	} tbl[] = {
		{ fff, (one << 61) - 1 }, // masked with (1 << 61) - 1
		{ one << 62, 0 }, // masked
		{ (one << 62) | (one << 61), (one << 61) }, // masked
		{ (one << 61) - 1, (one << 61) - 1 }, // same
	};
	T t1, t2;
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
		uint64_t v1[] = { fff, fff, fff, tbl[i].in };
		uint64_t v2[] = { fff, fff, fff, tbl[i].expected };
		t1.set(v1);
		t2.set(v2);
		CYBOZU_TEST_EQUAL(t1, t2);
	}
}

CYBOZU_TEST_AUTO(id)
{
	bls::Id id;
	CYBOZU_TEST_ASSERT(id.isZero());
	id = 5;
	CYBOZU_TEST_EQUAL(id, 5);
	{
		const uint64_t id1[] = { 1, 2, 3, 4 };
		id.set(id1);
		std::ostringstream os;
		os << id;
		CYBOZU_TEST_EQUAL(os.str(), "0x4000000000000000300000000000000020000000000000001");
	}
	testSet<bls::Id>();
}

CYBOZU_TEST_AUTO(SecretKey_set)
{
	testSet<bls::SecretKey>();
}

CYBOZU_TEST_AUTO(k_of_n)
{
	const std::string m = "abc";
	const int n = 5;
	const int k = 3;
	bls::SecretKey sec0;
	sec0.init();
	bls::Sign s0;
	sec0.sign(s0, m);
	bls::PublicKey pub0;
	sec0.getPublicKey(pub0);
	CYBOZU_TEST_ASSERT(s0.verify(pub0, m));

	bls::SecretKeyVec msk;
	sec0.getMasterSecretKey(msk, k);

	bls::SecretKeyVec allPrvVec(n);
	bls::IdVec allIdVec(n);
	for (int i = 0; i < n; i++) {
		int id = i + 1;
		allPrvVec[i].set(msk, id);
		allIdVec[i] = id;

		bls::SecretKey p;
		p.set(msk.data(), k, id);
		CYBOZU_TEST_EQUAL(allPrvVec[i], p);
	}

	bls::SignVec allSignVec(n);
	for (int i = 0; i < n; i++) {
		CYBOZU_TEST_ASSERT(allPrvVec[i] != sec0);
		allPrvVec[i].sign(allSignVec[i], m);
		bls::PublicKey pub;
		allPrvVec[i].getPublicKey(pub);
		CYBOZU_TEST_ASSERT(pub != pub0);
		CYBOZU_TEST_ASSERT(allSignVec[i].verify(pub, m));
	}

	/*
		3-out-of-n
		can recover
	*/
	bls::SecretKeyVec secVec(3);
	bls::IdVec idVec(3);
	for (int a = 0; a < n; a++) {
		secVec[0] = allPrvVec[a];
		idVec[0] = allIdVec[a];
		for (int b = a + 1; b < n; b++) {
			secVec[1] = allPrvVec[b];
			idVec[1] = allIdVec[b];
			for (int c = b + 1; c < n; c++) {
				secVec[2] = allPrvVec[c];
				idVec[2] = allIdVec[c];
				bls::SecretKey sec;
				sec.recover(secVec, idVec);
				CYBOZU_TEST_EQUAL(sec, sec0);
				bls::SecretKey sec2;
				sec2.recover(secVec.data(), idVec.data(), secVec.size());
				CYBOZU_TEST_EQUAL(sec, sec2);
			}
		}
	}
	{
		secVec[0] = allPrvVec[0];
		secVec[1] = allPrvVec[1];
		secVec[2] = allPrvVec[0]; // same of secVec[0]
		idVec[0] = allIdVec[0];
		idVec[1] = allIdVec[1];
		idVec[2] = allIdVec[0];
		bls::SecretKey sec;
		CYBOZU_TEST_EXCEPTION_MESSAGE(sec.recover(secVec, idVec), std::exception, "same id");
	}
	{
		/*
			n-out-of-n
			can recover
		*/
		bls::SecretKey sec;
		sec.recover(allPrvVec, allIdVec);
		CYBOZU_TEST_EQUAL(sec, sec0);
	}
	/*
		2-out-of-n
		can't recover
	*/
	secVec.resize(2);
	idVec.resize(2);
	for (int a = 0; a < n; a++) {
		secVec[0] = allPrvVec[a];
		idVec[0] = allIdVec[a];
		for (int b = a + 1; b < n; b++) {
			secVec[1] = allPrvVec[b];
			idVec[1] = allIdVec[b];
			bls::SecretKey sec;
			sec.recover(secVec, idVec);
			CYBOZU_TEST_ASSERT(sec != sec0);
		}
	}
	/*
		3-out-of-n
		can recover
	*/
	bls::SignVec signVec(3);
	idVec.resize(3);
	for (int a = 0; a < n; a++) {
		signVec[0] = allSignVec[a];
		idVec[0] = allIdVec[a];
		for (int b = a + 1; b < n; b++) {
			signVec[1] = allSignVec[b];
			idVec[1] = allIdVec[b];
			for (int c = b + 1; c < n; c++) {
				signVec[2] = allSignVec[c];
				idVec[2] = allIdVec[c];
				bls::Sign s;
				s.recover(signVec, idVec);
				CYBOZU_TEST_EQUAL(s, s0);
			}
		}
	}
	{
		/*
			n-out-of-n
			can recover
		*/
		bls::Sign s;
		s.recover(allSignVec, allIdVec);
		CYBOZU_TEST_EQUAL(s, s0);
	}
	/*
		2-out-of-n
		can't recover
	*/
	signVec.resize(2);
	idVec.resize(2);
	for (int a = 0; a < n; a++) {
		signVec[0] = allSignVec[a];
		idVec[0] = allIdVec[a];
		for (int b = a + 1; b < n; b++) {
			signVec[1] = allSignVec[b];
			idVec[1] = allIdVec[b];
			bls::Sign s;
			s.recover(signVec, idVec);
			CYBOZU_TEST_ASSERT(s != s0);
		}
	}
	// share and recover publicKey
	{
		bls::PublicKeyVec pubVec(k);
		idVec.resize(k);
		// select [0, k) publicKey
		for (int i = 0; i < k; i++) {
			allPrvVec[i].getPublicKey(pubVec[i]);
			idVec[i] = allIdVec[i];
		}
		bls::PublicKey pub;
		pub.recover(pubVec, idVec);
		CYBOZU_TEST_EQUAL(pub, pub0);
		bls::PublicKey pub2;
		pub2.recover(pubVec.data(), idVec.data(), pubVec.size());
		CYBOZU_TEST_EQUAL(pub, pub2);
	}
}

CYBOZU_TEST_AUTO(aggregate_sigs) {

	// generate messages
	const std::string m1 = "abc";
	const std::string m2 = "def";
	const std::string m3 = "hijklmnopqrs";

	std::vector<std::string> msgVec;
	msgVec.push_back(m1);
	msgVec.push_back(m2);
	msgVec.push_back(m3);


	// generate secrets
	bls::SecretKey sec1;
	bls::SecretKey sec2;
	bls::SecretKey sec3;

	sec1.init();
	sec2.init();
	sec3.init();

	// std::cout << "SECRETS" << std::endl;
	// std::cout << sec1 << std::endl;
	// std::cout << sec2 << std::endl;
	// std::cout << sec3 << std::endl;

	// get public keys
	bls::PublicKeyVec pubVec;
	bls::PublicKey pub1;
	bls::PublicKey pub2;
	bls::PublicKey pub3;

	sec1.getPublicKey(pub1);
	sec2.getPublicKey(pub2);
	sec2.getPublicKey(pub3);

	pubVec.push_back(pub1);
	pubVec.push_back(pub2);
	pubVec.push_back(pub3);


	// generate signatures
	bls::Sign s1;
	bls::Sign s2;
	bls::Sign s3;

	sec1.sign(s1, m1);
	sec2.sign(s2, m2);
	sec2.sign(s3, m3);

	// calculate aggregate signature
	bls::Sign sAgg;

	sAgg = s1;
	// std::cout << "AGGREGATE SIG" << sAgg << std::endl;
	// std::cout << "SIG 1" << s1 << std::endl;

	sAgg.add(s2);
	// std::cout << "AGGREGATE SIG" << sAgg << std::endl;
	// std::cout << "SIG 2" << s2 << std::endl;

	sAgg.add(s3);
	// std::cout << "AGGREGATE SIG" << sAgg << std::endl;
	// std::cout << "SAgg " << sAgg << std::endl;
	// std::cout << "s1 " << s1 << std::endl;

	CYBOZU_TEST_ASSERT(s1.verify(pub1, m1));
	CYBOZU_TEST_ASSERT(s2.verify(pub2, m2));
	CYBOZU_TEST_ASSERT(s3.verify(pub3, m3));

	CYBOZU_TEST_ASSERT(sAgg.verifyAggregate(msgVec, pubVec));
}

CYBOZU_TEST_AUTO(pop)
{
	const size_t k = 3;
	const size_t n = 6;
	const std::string m = "pop test";
	bls::SecretKey sec0;
	sec0.init();
	bls::PublicKey pub0;
	sec0.getPublicKey(pub0);
	bls::Sign s0;
	sec0.sign(s0, m);
	CYBOZU_TEST_ASSERT(s0.verify(pub0, m));

	bls::SecretKeyVec msk;
	sec0.getMasterSecretKey(msk, k);

	bls::PublicKeyVec mpk;
	bls::getMasterPublicKey(mpk, msk);
	bls::SignVec  popVec;
	bls::getPopVec(popVec, msk);

	for (size_t i = 0; i < popVec.size(); i++) {
		CYBOZU_TEST_ASSERT(popVec[i].verify(mpk[i]));
	}

	const int idTbl[n] = {
		3, 5, 193, 22, 15
	};
	bls::SecretKeyVec secVec(n);
	bls::PublicKeyVec pubVec(n);
	bls::SignVec sVec(n);
	for (size_t i = 0; i < n; i++) {
		int id = idTbl[i];
		secVec[i].set(msk, id);
		secVec[i].getPublicKey(pubVec[i]);
		bls::PublicKey pub;
		pub.set(mpk, id);
		CYBOZU_TEST_EQUAL(pubVec[i], pub);

		bls::Sign pop;
		secVec[i].getPop(pop);
		CYBOZU_TEST_ASSERT(pop.verify(pubVec[i]));

		secVec[i].sign(sVec[i], m);
		CYBOZU_TEST_ASSERT(sVec[i].verify(pubVec[i], m));
	}
	secVec.resize(k);
	sVec.resize(k);
	bls::IdVec idVec(k);
	for (size_t i = 0; i < k; i++) {
		idVec[i] = idTbl[i];
	}
	bls::SecretKey sec;
	sec.recover(secVec, idVec);
	CYBOZU_TEST_EQUAL(sec, sec0);
	bls::Sign s;
	s.recover(sVec, idVec);
	CYBOZU_TEST_EQUAL(s, s0);
	bls::Sign s2;
	s2.recover(sVec.data(), idVec.data(), sVec.size());
	CYBOZU_TEST_EQUAL(s, s2);
}

CYBOZU_TEST_AUTO(add)
{
	bls::SecretKey sec1, sec2;
	sec1.init();
	sec2.init();
	CYBOZU_TEST_ASSERT(sec1 != sec2);

	bls::PublicKey pub1, pub2;
	sec1.getPublicKey(pub1);
	sec2.getPublicKey(pub2);

	const std::string m = "doremi";
	bls::Sign s1, s2;
	sec1.sign(s1, m);
	sec2.sign(s2, m);
	CYBOZU_TEST_ASSERT((s1 + s2).verify(pub1 + pub2, m));
}
