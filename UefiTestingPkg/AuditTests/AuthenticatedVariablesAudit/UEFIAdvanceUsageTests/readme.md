# UEFI Advanced Usage Tests

These tests focus on the limits of UEFI private authenticated variables

While useful, it is not expected (today) that these work correctly on all platforms.

|  Test Name        | Description                                                                                                           | Expectation                 | Implemented |
|:------------------|:----------------------------------------------------------------------------------------------------------------------|:----------------------------|-------------|
| DigestAlgorithms  | UEFI Supports AlgorithmDigests of SHA384, and SHA512                                                                  | SUCCESS (Currently FAIL)    | FALSE       |
| TrustAnchor       | UEFI Supports certificate chaining to a trust anchor certificate                                                      | SUCCESS (Currently FAIL)    | TRUE        |
| MultipleSigners   | UEFI does not make a statement this should work, but implementation seems to allow this if they are in the same chain | FAIL (Currently SUCCESS)    | TRUE        |
| LargeCertificate  | UEFI does not make a statement on what the largest certificate supported is, or what the behavior is if it fails      | ????                        | FALSE       |