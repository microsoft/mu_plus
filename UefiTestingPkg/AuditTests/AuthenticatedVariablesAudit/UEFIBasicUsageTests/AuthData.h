#ifndef AUTH_DATA_H_
#define AUTH_DATA_H_


#include <Uefi/UefiBaseType.h>


extern UINT8 m2048VariableKeyLengthSupportMockVar[1729];
extern UINT8 m2048VariableKeyLengthSupportMockVarExpected[73];

extern UINT8 m2048VariableKeyLengthSupportMockVarEmpty[1657];
extern UINT8 m3072VariableKeyLengthSupportMockVar[1985];
extern UINT8 m3072VariableKeyLengthSupportMockVarExpected[73];

extern UINT8 m3072VariableKeyLengthSupportMockVarEmpty[1913];
extern UINT8 m4096VariableKeyLengthSupportMockVar[2241];
extern UINT8 m4096VariableKeyLengthSupportMockVarExpected[73];

extern UINT8 m4096VariableKeyLengthSupportMockVarEmpty[2169];
extern UINT8 m1AdditionalCertificatesMockVar[3212];
extern UINT8 m1AdditionalCertificatesMockVarExpected[112];

extern UINT8 m1AdditionalCertificatesMockVarEmpty[3101];
extern UINT8 m2AdditionalCertificatesMockVar[4916];
extern UINT8 m2AdditionalCertificatesMockVarExpected[112];

extern UINT8 m2AdditionalCertificatesMockVarEmpty[4805];
extern UINT8 m3AdditionalCertificatesMockVar[6619];
extern UINT8 m3AdditionalCertificatesMockVarExpected[112];

extern UINT8 m3AdditionalCertificatesMockVarEmpty[6508];
extern UINT8 mPreventUpdateInitVariableMockVar[2197];
extern UINT8 mPreventUpdateInitVariableMockVarExpected[68];

extern UINT8 mPreventUpdateInitVariableMockVarEmpty[2130];
extern UINT8 mPreventUpdateInvalidVariableMockVar[1691];
extern UINT8 mPreventUpdateInvalidVariableMockVarExpected[68];

extern UINT8 mPreventUpdateInvalidVariableMockVarEmpty[1624];
extern UINT8 mPreventRollbackPastVariableMockVar[2211];
extern UINT8 mPreventRollbackPastVariableMockVarExpected[76];

extern UINT8 mPreventRollbackPastVariableMockVarEmpty[2136];
extern UINT8 mPreventRollbackFutureVariableMockVar[2211];
extern UINT8 mPreventRollbackFutureVariableMockVarExpected[76];

extern UINT8 mPreventRollbackFutureVariableMockVarEmpty[2136];
#endif AUTH_DATA_H_
