/*
 *  Author: Matt McCutchen, https://mattmccutchen.net/bigint
 *  Copyright 2008/2010/2012 ZXing authors All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * The BigInteger library was included in the ZXing C++ library by Hartmut
 * Neubauer with the permission of Matt McCutchen because PDF417 uses
 * BigIntegers.
 */

#include "BigIntegerUtils.h"
#include "BigUnsignedInABase.h"

namespace bigInteger {

std::string bigUnsignedToString(const BigUnsigned &x) {
	return std::string(BigUnsignedInABase(x, 10));
}

std::string bigIntegerToString(const BigInteger &x) {
	return (x.getSign() == BigInteger::negative)
		? (std::string("-") + bigUnsignedToString(x.getMagnitude()))
		: (bigUnsignedToString(x.getMagnitude()));
}

BigUnsigned stringToBigUnsigned(const std::string &s) {
	return BigUnsigned(BigUnsignedInABase(s, 10));
}

BigInteger stringToBigInteger(const std::string &s) {
	// Recognize a sign followed by a BigUnsigned.
	return (s[0] == '-') ? BigInteger(stringToBigUnsigned(s.substr(1, s.length() - 1)), BigInteger::negative)
		: (s[0] == '+') ? BigInteger(stringToBigUnsigned(s.substr(1, s.length() - 1)))
		: BigInteger(stringToBigUnsigned(s));
}

#if _MSC_VER>=1300		//* 04.05.2012 hfn not for eMbedded c++ compiler
std::ostream &operator <<(std::ostream &os, const BigUnsigned &x) {
	BigUnsignedInABase::Base base;
	long osFlags = os.flags();
	if (osFlags & os.dec)
		base = 10;
	else if (osFlags & os.hex) {
		base = 16;
		if (osFlags & os.showbase)
			os << "0x";
	} else if (osFlags & os.oct) {
		base = 8;
		if (osFlags & os.showbase)
			os << '0';
	} else
		throw "std::ostream << BigUnsigned: Could not determine the desired base from output-stream flags";
	std::string s = std::string(BigUnsignedInABase(x, base));
	os << s;
	return os;
}

std::ostream &operator <<(std::ostream &os, const BigInteger &x) {
	if (x.getSign() == BigInteger::negative)
		os << '-';
	os << x.getMagnitude();
	return os;
}
#endif

}