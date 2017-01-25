#pragma once
/**
	@file
	@brief BLS threshold signature on BN curve
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#include <vector>
#include <string>
#include <iosfwd>
#include <stdint.h>

namespace bls {

namespace impl {

struct SecretKey;
struct PublicKey;
struct Sign;
struct Id;

} // bls::impl

/*
	BLS signature
	e : G2 x G1 -> Fp12
	Q in G2 ; fixed global parameter
	H : {str} -> G1
	s : secret key
	sQ ; public key
	s H(m) ; signature of m
	verify ; e(sQ, H(m)) = e(Q, s H(m))
*/

/*
	initialize this library
	call this once before using the other method
*/
void init();

class SecretKey;
class PublicKey;
class Sign;
class Id;

/*
	the value of secretKey and Id must be less than
	r = 0x2523648240000001ba344d8000000007ff9f800000000010a10000000000000d
	sizeof(uint64_t) * keySize = 32-byte
*/
const size_t keySize = 4;

typedef std::vector<SecretKey> SecretKeyVec;
typedef std::vector<PublicKey> PublicKeyVec;
typedef std::vector<Sign> SignVec;
typedef std::vector<Id> IdVec;

class Id {
	uint64_t self_[4]; // 256-bit
	friend class PublicKey;
	friend class SecretKey;
	template<class T, class G> friend struct WrapArray;
	impl::Id& getInner() { return *reinterpret_cast<impl::Id*>(self_); }
	const impl::Id& getInner() const { return *reinterpret_cast<const impl::Id*>(self_); }
public:
	Id(unsigned int id = 0);
	bool operator==(const Id& rhs) const;
	bool operator!=(const Id& rhs) const { return !(*this == rhs); }
	friend std::ostream& operator<<(std::ostream& os, const Id& id);
	friend std::istream& operator>>(std::istream& is, Id& id);
	bool isZero() const;
	/*
		set p[0, .., keySize)
		@note the value must be less than r
	*/
	void set(const uint64_t *p);

};

/*
	s ; secret key
*/
class SecretKey {
	uint64_t self_[4]; // 256-bit
	template<class T, class G> friend struct WrapArray;
	impl::SecretKey& getInner() { return *reinterpret_cast<impl::SecretKey*>(self_); }
	const impl::SecretKey& getInner() const { return *reinterpret_cast<const impl::SecretKey*>(self_); }
public:
	SecretKey() : self_() {}
	bool operator==(const SecretKey& rhs) const;
	bool operator!=(const SecretKey& rhs) const { return !(*this == rhs); }
	friend std::ostream& operator<<(std::ostream& os, const SecretKey& sec);
	friend std::istream& operator>>(std::istream& is, SecretKey& sec);
	/*
		initialize secretKey with random number and set id = 0
	*/
	void init();
	/*
		set secretKey with p[0, .., keySize) and set id = 0
		@note the value must be less than r
	*/
	void set(const uint64_t *p);
	void getPublicKey(PublicKey& pub) const;
	void sign(Sign& sign, const std::string& m) const;
	/*
		make Pop(Proof of Possesion)
		pop = prv.sign(pub)
	*/
	void getPop(Sign& pop) const;
	/*
		make [s_0, ..., s_{k-1}] to prepare k-out-of-n secret sharing
	*/
	void getMasterSecretKey(SecretKeyVec& msk, size_t k) const;
	/*
		set a secret key for id > 0 from msk
	*/
	void set(const SecretKeyVec& msk, const Id& id)
	{
		set(msk.data(), msk.size(), id);
	}
	/*
		recover secretKey from k secVec
	*/
	void recover(const SecretKeyVec& secVec, const IdVec& idVec);
	/*
		add secret key
	*/
	void add(const SecretKey& rhs);

	// the following methods are for C api
	/*
		the size of msk must be k
	*/
	void set(const SecretKey *msk, size_t k, const Id& id);
	void recover(const SecretKey *secVec, const Id *idVec, size_t n);
};

/*
	sQ ; public key
*/
class PublicKey {
	uint64_t self_[4 * 2 * 3]; // 256-bit x 2 x 3
	friend class SecretKey;
	friend class Sign;
	template<class T, class G> friend struct WrapArray;
	impl::PublicKey& getInner() { return *reinterpret_cast<impl::PublicKey*>(self_); }
	const impl::PublicKey& getInner() const { return *reinterpret_cast<const impl::PublicKey*>(self_); }
public:
	PublicKey() : self_() {}
	bool operator==(const PublicKey& rhs) const;
	bool operator!=(const PublicKey& rhs) const { return !(*this == rhs); }
	friend std::ostream& operator<<(std::ostream& os, const PublicKey& pub);
	friend std::istream& operator>>(std::istream& is, PublicKey& pub);
	/*
		set public for id from mpk
	*/
	void set(const PublicKeyVec& mpk, const Id& id)
	{
		set(mpk.data(), mpk.size(), id);
	}
	/*
		recover publicKey from k pubVec
	*/
	void recover(const PublicKeyVec& pubVec, const IdVec& idVec);
	/*
		add public key
	*/
	void add(const PublicKey& rhs);

	// the following methods are for C api
	void set(const PublicKey *mpk, size_t k, const Id& id);
	void recover(const PublicKey *pubVec, const Id *idVec, size_t n);
};

/*
	s H(m) ; sign
*/
class Sign {
	uint64_t self_[4 * 3]; // 256-bit x 3
	friend class SecretKey;
	template<class T, class G> friend struct WrapArray;
	impl::Sign& getInner() { return *reinterpret_cast<impl::Sign*>(self_); }
	const impl::Sign& getInner() const { return *reinterpret_cast<const impl::Sign*>(self_); }
public:
	Sign() : self_() {}
	bool operator==(const Sign& rhs) const;
	bool operator!=(const Sign& rhs) const { return !(*this == rhs); }
	friend std::ostream& operator<<(std::ostream& os, const Sign& s);
	friend std::istream& operator>>(std::istream& is, Sign& s);
	bool verify(const PublicKey& pub, const std::string& m) const;
	/*
		verify self(pop) with pub
	*/
	bool verify(const PublicKey& pub) const;

	/*
		verify self(pop) with message and pupkey vectors
	 */
	bool verifyAggregate(const std::vector<std::string> &messages, const std::vector<PublicKey> &pubKeys);
	/*
		recover sign from k signVec
	*/
	void recover(const SignVec& signVec, const IdVec& idVec);
	/*
		add signature
	*/
	void add(const Sign& rhs);

	// the following methods are for C api
	void recover(const Sign* signVec, const Id *idVec, size_t n);
};

/*
	make master public key [s_0 Q, ..., s_{k-1} Q] from msk
*/
inline void getMasterPublicKey(PublicKeyVec& mpk, const SecretKeyVec& msk)
{
	const size_t n = msk.size();
	mpk.resize(n);
	for (size_t i = 0; i < n; i++) {
		msk[i].getPublicKey(mpk[i]);
	}
}

/*
	make pop from msk and mpk
*/
inline void getPopVec(SignVec& popVec, const SecretKeyVec& msk)
{
	const size_t n = msk.size();
	popVec.resize(n);
	for (size_t i = 0; i < n; i++) {
		msk[i].getPop(popVec[i]);
	}
}

inline Sign operator+(const Sign& a, const Sign& b) { Sign r(a); r.add(b); return r; }
inline PublicKey operator+(const PublicKey& a, const PublicKey& b) { PublicKey r(a); r.add(b); return r; }
inline SecretKey operator+(const SecretKey& a, const SecretKey& b) { SecretKey r(a); r.add(b); return r; }

} //bls
