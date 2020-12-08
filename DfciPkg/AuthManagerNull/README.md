# AuthManagerNull

## Purposes

Do not use in production!

### FrontPage during device bringup

This driver can be a stand in for IdentityAndAuthManager, which requires RngLib, to allow FrontPage development if
RngLib is not yet functional.

### Unit Testing

With further development, this "Null" driver could be an effective stub for IdentityAndAuthManager, allowing detailed
unit testing of DFCI.
