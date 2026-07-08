/* Broker CA certificate (PEM), embedded in the firmware so the subscriber
 * can authenticate the broker without copying a certificate file onto the
 * board.
 * Replace the body below if the broker's CA changes; init_conn() parses
 * it whenever no CA file is supplied on the command line via -c.
 */

#  define TEST_CA_CRT_RSA_SHA256_PEM                                       \
    "-----BEGIN CERTIFICATE-----\r\n"                                      \
    "MIIDYTCCAkmgAwIBAgIUAk1VwJ7il2775wUnEgBZSvW77dUwDQYJKoZIhvcNAQEL\r\n" \
    "BQAwQDELMAkGA1UEBhMCUk8xEjAQBgNVBAgMCUJ1Y2hhcmVzdDEOMAwGA1UECgwF\r\n" \
    "TXlPcmcxDTALBgNVBAMMBE15Q0EwHhcNMjYwNjA5MDA0MjExWhcNMzYwNjA2MDA0\r\n" \
    "MjExWjBAMQswCQYDVQQGEwJSTzESMBAGA1UECAwJQnVjaGFyZXN0MQ4wDAYDVQQK\r\n" \
    "DAVNeU9yZzENMAsGA1UEAwwETXlDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCC\r\n" \
    "AQoCggEBAMGfLamBuio0mYqL8TDLqcxqCDhELmrmYxccz8/BAJ3xG0bbqAg0o2CM\r\n" \
    "2nUcDWeDTTOFTEAOVKhqqbu7jzQA+/XRQaptYAzt13N0pMUFlKZQ0QVefAH11mph\r\n" \
    "M2u0exWZObRQxUyfzzJG1UZQ4xGqXUekg+2LMSS09g/QAsCreu8O+xzAK9XUA6np\r\n" \
    "8lebaIqxFHabrjlbHgan5qKP0+EncMRN/5Lb11MDzfjyIusoFWPZF8rF2x9D+UFv\r\n" \
    "hZuL9Z40jz3cRlVUxuVtUq1tna2CCwWhwl5gwAQ/b0rkhe+5/b3cgSs3VbIdWHic\r\n" \
    "nwM4Yq4K/nAetYiOxtjKY5zLIvRopd0CAwEAAaNTMFEwHQYDVR0OBBYEFFR43w+x\r\n" \
    "ri1ssaAyJzQGDH/dP2e0MB8GA1UdIwQYMBaAFFR43w+xri1ssaAyJzQGDH/dP2e0\r\n" \
    "MA8GA1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQELBQADggEBADU/lBjTGb7RlgGq\r\n" \
    "HYCktoWZBNkJFPVTPrC1mwzJX/vSaz9QUpRA5dpprSl84QKvWNxO5fPEMmwK2Wrw\r\n" \
    "x6tdE2aWb5x+3W0302JnaUPNhDF26Z5+LK4CoRHjbT2J3FrDg2u6/3Si+RpNSHYh\r\n" \
    "0OCdApzCyzqWdmvb7VD+u+isSh9CEUcaBOCq/72GNqdIjnSsgMGNHp9Ld6+ctwJO\r\n" \
    "sQkj0M3MaciNSPZ3y0w/GqeWNQaK3Rk5pSaqrc9RQS42KtJuGOqMiBMG1Ib6b7D4\r\n" \
    "bJSEDBzhO1M0o9vwr9xYaCUfbQzuuYJ3al4weRl0E4B63o2OFUjMOILIO3LeuuRD\r\n" \
    "+HXdOE0=\r\n"                                                         \
    "-----END CERTIFICATE-----\r\n"
/* END FILE */
