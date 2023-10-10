#ifndef AUTH_DATA_H_
#define AUTH_DATA_H_

#include <Uefi/UefiBaseType.h>

extern UINT8  mSHA384DigestAlgorithmsSupportMockVar[2214];
extern UINT8  mSHA384DigestAlgorithmsSupportMockVarExpected[32];
extern UINT8  mSHA384DigestAlgorithmsSupportMockVarEmpty[2183];

extern UINT8  mSHA512DigestAlgorithmsSupportMockVar[2214];
extern UINT8  mSHA512DigestAlgorithmsSupportMockVarExpected[32];
extern UINT8  mSHA512DigestAlgorithmsSupportMockVarEmpty[2183];

extern UINT8  mSigner1TrustAnchorSupportMockVar[4593];
extern UINT8  mSigner1TrustAnchorSupportMockVarExpected[72];
extern UINT8  mSigner1TrustAnchorSupportMockVarEmpty[4522];

extern UINT8  mSigner2TrustAnchorSupportMockVar[3659];
extern UINT8  mSigner2TrustAnchorSupportMockVarExpected[68];
extern UINT8  mSigner2TrustAnchorSupportMockVarEmpty[3592];

extern UINT8  mSigner1MultipleSignersSupportMockVar[4315];
extern UINT8  mSigner1MultipleSignersSupportMockVarExpected[60];
extern UINT8  mSigner1MultipleSignersSupportMockVarEmpty[4256];

extern UINT8  mSigner2MultipleSignersSupportMockVar[4309];
extern UINT8  mSigner2MultipleSignersSupportMockVarExpected[60];
extern UINT8  mSigner2MultipleSignersSupportMockVarEmpty[4250];
#endif // AUTH_DATA_H_
