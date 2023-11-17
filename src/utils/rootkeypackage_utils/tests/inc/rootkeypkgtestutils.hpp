#ifndef ROOTKEYPKGTESTUTILS_HPP
#define ROOTKEYPKGTESTUTILS_HPP

#include <aduc/c_utils.h>
#include <string>
#include <vector>
namespace aduc
{
namespace rootkeypkgtestutils
{
constexpr size_t NumTestRootKeys = 3;
enum class rootkeys
{
    rootkey1 = 0,
    rootkey2 = 1,
    rootkey3 = 2,
};

static std::string privgens[NumTestRootKeys] = {
    // Root 1
    "MIIEowIBAAKCAQEArSzp0Z8xFFs6f3SixM_iFni-HG5Gfdsfjm7fKNSnRHmC656o"
    "azWTpUgC0GCrzs8xYwIRoTyFbIQZAQf4cJpA2EKVIkWxPslHuXUmCGRFGtoOp-m_"
    "YEgZ2T36NG0dbFOuzPDGLnUw2EwyZrPZqLPKIwsWP6qHgI0FRWyR7FQoGfYqjjKs"
    "bS2Xt2oFkDt8121XKYrxLIWe3gYgUstwqCdzT82rAtzpLvMVD74Rte5qHfb8XD2u"
    "ntlmmnpE9jMyhzYOdNqsUYuGTy3NBAJNg5OgAVAINWrcoFJaSNjlBZuHYtk7414o"
    "2DSkbppI5mWil_l5u4rSYQWjIHWhzqd7cdJ6bwIDAQABAoIBACIAo5hpLXXVw9Kq"
    "0BrcxoOrCYkDjgvALp4E3wRhXMZxJWemK2OBjY_yZ7sKgDGHNSc_jL6f54K7HT26"
    "lullID5WNHoaPQca9l5Pxjv3lCoFjsMhflYlUg35wVrx4ckXVUcgL4mHsSOHMo4_"
    "2gjp5FKlJqUxkpGHHvXWr4A7tfQCsv8-Ue_t1wQpC6g1j7K0d9M3_LN8BF037h2x"
    "sWYsXa80zY9CT_knJtTXbAL2DI6xJH5CHX8KCf5t2gGdssveKqP2dHhR6PlItlW_"
    "nqBw8lJobhpPBEookgbSGdTbms2ODBg5TwO7GFudCYb1EnAyvdjdHXxJJgMzC4hL"
    "0-YhvYECgYEA1WPdvQfRISSPrHU7wpw9-r-kKsgM3F8op6nSejjtD87mhnaOIqlq"
    "Wk90dMLlv0l-pfslIWYQnRWFZJ3SJzEgwV6tuHQqZQs7H8n_r00sYnxvM1jaChR5"
    "PNUp8-xcXWlTQezUO5B053WW04HPSp-hLO2licbADHbN-nbtOCDgr78CgYEAz8FX"
    "2yN5xMIe3iI3WpL0lSiSgRdY2i9ckcYuYIN8M6-lFBMI16QwUcnzflgisto-0bxm"
    "EJS0jdO9uHzU0issAqO4wt6a9PT5SgY_AnqHpyGC1zHFSq8-LKnLGWKq_3UXA07X"
    "c0SGH1sawIOSGSiLli6upZm98XQUECi1zYC84VECgYA7vdXaklGuYboHolq0xWFk"
    "zjp7774KBGoxZo4SwdU808QeaRqqAZxQ5GXKOrZvs3fNqF5g115XXCsYXEb1yf9N"
    "o-Am__7OgzJuV_NJdOW0PPo0-e8xW0IGEffI3qeNT2uLzW4trufL1VQAqxsJ3V8W"
    "YQIzbH1f__Ly6FDJogrFqQKBgQClLam2d4w3HcgBAN9VygVgGjPxIyjnwEQvOoxg"
    "--liBsKPJVsgaFBqltBboaNk5BSSGOJlSHYNVU5AQi_LMbv6FUWu2eIF5EfXzQOH"
    "6vezr_chNVcRPCeIxy7Nbh9aoDO904-E-_RPNdYdPf6Iti3VKCR_Ua8tLAdPma6C"
    "R1rlIQKBgAlZgxM_Zi5-cMsoPGsGvzX5a_u1JMCdm3Auc5Q3KU9CEZQgPzv2Ta77"
    "W2bBGvpviUYDEzzTQvqio--BjB3plsgwFYOnWVLMXBEhCw8IyhymyrGk1Vn8_V-w"
    "ODtCHfNYJCtdxLN2P3m_yTecBbGJNbYFh53NzgaBl76Ssav0QEk9",

    // Root 2
    "MIIEpQIBAAKCAQEAyZtxZnkBrmZzFF7GNA9XYUFaBSl1MqcLEgqGW9PUyS48LK5r"
    "7PBXDCnF4e5u04VcKJ2PKPLHwRv9_ZGYQeeVJYVoBIomEOISXXXHKf90PwYKswFq"
    "XILzB9AaYUvftgp2mubfVj94Eiq5GKvFwr7yqY3Yqj6tSSWNi6eV-FJt4ypth4CI"
    "pyZPrrixnvI02gUOSF1Xwwk8PVgM2a_9FKWflFSYeN7p97ORkdXnA82XeYX9h2RH"
    "MomtZMCCtNpjWmioLgUXyQeS6Qq1Q2wfAPOZq-amHayq_Dxy5db9KbIe13AdbEv2"
    "BCoQlyL1fKSBQWiRGhkSxPwrKuseLp90j1xjhQIDAQABAoIBAQCb60D3rGw1cfxc"
    "a7PEPX0ptT4msdp28yOnr0YcLKbrdHuLXtYPKA1aVdA5nIpPwlr0-m3mkGUWn0x-"
    "2CQ2DCGYJCW_JQytj_n-GAGRJITF3SlXKagVphzJFRPh3alFg7AYuqdNb8av3iTK"
    "xMYsiDrqEM7ZU6H26TkgIdrlcvKTdTQ23c3-mAWYjfk_RysNACCSe2rtrjGNFRYm"
    "Sgt0uXVn_xJWHiMPGK_NrPea1TluN5Y0QXzV1LPEKlMG1c1-s9F30MA4cC3QfHot"
    "r96cD83YJRjWj_8U7TyMcqNahRQi8L0qBIKM5IRq3BmgWsGRgYtqcbbSoI0rAh2E"
    "6FlLLR4dAoGBAOnIFEbC8lJ-S-hgZzhGZUnIroI7tvxVr1UBzxLJNE0laVDaLbnj"
    "ymCMt-o3dwUQ3gx6aIlEU_6STPq5YUdEqc_5LJ1rSOUC3jL_QIHOGKFpLBvicp8u"
    "aqW_QZ2Gt2l9-ha5CnAw0NZDhB5-yxx87gI1TQcKS7-UpAK6lU_jy65rAoGBANzE"
    "jzhB7PveNIA-Z4nWMVqmqkQe2Y87QG7aKmVF2MgyDfEWq-3Ui3giK6dPLhvMbfTM"
    "mOb7WWOZ4wq3bfSzhqYA12gz2fj52GOwGcsXMe9bObGsv9veAC41ZDmnXxCGxwkV"
    "3pb8djUxcGZ3rX4Pg057G7sjDAXOMZu9EJeUD9HPAoGBAOOz_pPho5bX7uV6qG72"
    "mgdg0SCGOzfR2YHJzkB0-1082DRpHeqWRXL-_M_DkEi94hlzTMiOZeVp6FK5J1f8"
    "OA4am-sEKS7uOTCgz9reu7zTrKPIT25eDoA8JhPhuFmm22UwfEtEMNTRVClDxF-O"
    "C2DZO5pk29zRUWJbC72RmbzbAoGBAJ8RjbRWZB_ysld5H3dEexk7H1Gn_NBkO__j"
    "eEqyMgnFbyA4WrcpvzhXPqb1uV5URSHuzXkYwAaxGdNd6X84X5t63bp6KeNoek8R"
    "0vPviY5SZ9aqhy8v3WduFwIno6qvwfe85z5ZN_8J2VgrgTlkihLhR1DmZsJEGKCD"
    "cNIW3_MnAoGAfTIFiaOEo9hhIb0RJQc9ZXhcYwZq9uuS4BlhhLrOOGGJlADwJmg5"
    "JeSNt0TjKTG0AOJwu_O-uHkpErskagBdnwdf08WtrLsxWq__0WYAglbrZ12goG4_"
    "2iXoA9rS4bQ0ybxQMU9nLC6rHq4ktNNbcV61hjdRG94rrIRon6phuh0",

    // Root 3
    "MIIEpQIBAAKCAQEAwRy4kb2eXFXyTPVGewfBbZVNoOoLOLKdqxLvnFl7-9jwWwS4"
    "9GNZ8kpfO3nCU1qsoD198fLaTBvz9GVnsmrjpxLuA8im8Rq8O56kNs2XUDqftCFG"
    "CCSBqbDww4vcHuTdeQF3xqzL2ezIe4hDqF3r3Uu7VkzNmLhIccW03SEB7xDYH7ta"
    "t_6nAE_CuToqQh3V8gTQulRckONhAGG3CK_5DqHpk_GU8AFmWO8um2bPyTGW9pQG"
    "cQj5TieLpIWZmS6r8_NyF056B7b9PZ10ZozkLTdUVvEy3dMeQXLVJBn3ZpccZB6M"
    "8UaD1UMW7HXEn0saro506cMWdx06g6EMRv3GbwIDAQABAoIBADAHoLA-5SA6ECWr"
    "63e2GhnTRJd9HoVfh2-BKi9M4lVlQ4KfzhCalUh5zu3P_tEUgFLqeGSw7jJ30Vk7"
    "z2rEwxJO1vwB7-OPrrl1X4px3-yIaCg5Dl4AFm_KHlfEdV8JtTvfbZbwa1MsXUC8"
    "R3ecxIkpyNJSD-CoFPyXG8DT8NMwgjwQnfY2Zr3VO4x21JtXJgeX3F47Ve5ChTQ3"
    "Af87PDqsiJC6PLwInKXfhZm2zXWowY_3-5IrT3JL_mrCFhv5xihQWo6mjalyR-5J"
    "3jBb4KGkAB57AtVwjB1NYm2lA2yVww0GejT1cq1GPMcyuOd5OPlncPfDH7DkP8uw"
    "A1SQI8ECgYEA8_9-Qc1e7G5_yt_RTki3w2tPcmqvkAO_OKuBlAvyg0w9ftFLi8YF"
    "EQYLTYVc87U3IYfWezqMgI6tW9_cu4WVIUObGlfobSQmlDO3wIcnIt1GdrMIX7AY"
    "AE3r-ITK4_phw4ySEDL-ICnAl6hqaiY_7pkZXbRVf7B9Y9UIuAWIyfkCgYEAypx0"
    "udJ-m0fO8PmZl6cK9XWXVw2_fvDlR_NvZiv2gsvdzR6zpS2ryOCdNnEfzC1Cx3eC"
    "4A06zBZujaTdHzDQWhg2oAI5UOzc19kOZ2IZCKeBRCB_1B0n1m5B80DlZHAlxdt7"
    "iZFe0pItl55DxBrnGvR4IbZmf6g4K96oLPubbacCgYEAiI3m1WDgzSWSc7ILa-qa"
    "nc3C6t_2XX0bWdXycS62jPDwQbdUtmceksZ1MO-AdAxpTGS_jrvXwmMXdqG04WYD"
    "blhtx7KHK_3dcXf4UNHS_1ojg27zMspUxGbXt4BqJGkOqehUHqjLPKjhVn80_y9k"
    "_F3GqoCwkQPvSR4DASpnwdECgYEAnb5R4prPl3XA2Dx8KGYVUiXLPiul-97xo6MU"
    "CCgSNKMkfJ56nw9_v1WhENHiP8S9SS7y5h1muZ59VCoXPkFy9bIEVW9l0GuZRTPo"
    "0vS9KM_BBJmI7EwGyBvvnMPZ1Oi7f9_xvpk_ihHlDTZa7ENFyuaq4RRxmNIPaZhg"
    "tyTtTeECgYEA2q4b3_saP_gNUVWZADzuY4YgbDsvhYxYvPboxB0HQULqr6-A63p_"
    "QslCL1dPkqQXNjcon4gzIguwrfSFD1vFhKqR1WLfdJ6RIZNXn-YXaiqJ0DjbT5Fj"
    "004n2BU6UABWLTf8L8OwkcvdFeWT2Yvz3fT38VDbQOdspyDODvtRP2Y",
};

static std::string pubs[NumTestRootKeys] = {
    // Root 1 pub
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEArSzp0Z8xFFs6f3SixM_i"
    "Fni-HG5Gfdsfjm7fKNSnRHmC656oazWTpUgC0GCrzs8xYwIRoTyFbIQZAQf4cJpA"
    "2EKVIkWxPslHuXUmCGRFGtoOp-m_YEgZ2T36NG0dbFOuzPDGLnUw2EwyZrPZqLPK"
    "IwsWP6qHgI0FRWyR7FQoGfYqjjKsbS2Xt2oFkDt8121XKYrxLIWe3gYgUstwqCdz"
    "T82rAtzpLvMVD74Rte5qHfb8XD2untlmmnpE9jMyhzYOdNqsUYuGTy3NBAJNg5Og"
    "AVAINWrcoFJaSNjlBZuHYtk7414o2DSkbppI5mWil_l5u4rSYQWjIHWhzqd7cdJ6"
    "bwIDAQAB",

    // Root 2 pub
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAyZtxZnkBrmZzFF7GNA9X"
    "YUFaBSl1MqcLEgqGW9PUyS48LK5r7PBXDCnF4e5u04VcKJ2PKPLHwRv9_ZGYQeeV"
    "JYVoBIomEOISXXXHKf90PwYKswFqXILzB9AaYUvftgp2mubfVj94Eiq5GKvFwr7y"
    "qY3Yqj6tSSWNi6eV-FJt4ypth4CIpyZPrrixnvI02gUOSF1Xwwk8PVgM2a_9FKWf"
    "lFSYeN7p97ORkdXnA82XeYX9h2RHMomtZMCCtNpjWmioLgUXyQeS6Qq1Q2wfAPOZ"
    "q-amHayq_Dxy5db9KbIe13AdbEv2BCoQlyL1fKSBQWiRGhkSxPwrKuseLp90j1xj"
    "hQIDAQAB",

    // Root 3 pub
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAwRy4kb2eXFXyTPVGewfB"
    "bZVNoOoLOLKdqxLvnFl7-9jwWwS49GNZ8kpfO3nCU1qsoD198fLaTBvz9GVnsmrj"
    "pxLuA8im8Rq8O56kNs2XUDqftCFGCCSBqbDww4vcHuTdeQF3xqzL2ezIe4hDqF3r"
    "3Uu7VkzNmLhIccW03SEB7xDYH7tat_6nAE_CuToqQh3V8gTQulRckONhAGG3CK_5"
    "DqHpk_GU8AFmWO8um2bPyTGW9pQGcQj5TieLpIWZmS6r8_NyF056B7b9PZ10Zozk"
    "LTdUVvEy3dMeQXLVJBn3ZpccZB6M8UaD1UMW7HXEn0saro506cMWdx06g6EMRv3G"
    "bwIDAQAB",
};

static std::string pubhashies[NumTestRootKeys] = {
    // SHA256 of root 1 pub
    "KHN0ZGluKT0gNzVmZWU5N2U1MGQxZjY4MWRmNTdmNWM1Njg4OWFhOGIyNjY1NDQ0"
    "MDYyZTE3NzAxOTJlNzE5ZjMzMWM3ZjY0Ngo=",

    // SHA256 of root 2 pub
    "KHN0ZGluKT0gOTBkYjJmZGM5Nzc2ODRjNjY4MmRkOWI2NmEyYzdjOGNhMzNiNWY5"
    "YTA3OTE4YjQwMzBjMTJhMjA5M2RkYzJhOAo=",

    // SHA256 of root 3 pub
    "KHN0ZGluKT0gNTJhNDRlMjVkODM3YzNjOGMzZWIwZTg0MWI3OWE0MzhjNGYzZDc0"
    "NWIwMTRhMjE0MmNjMjc4NTFlODAxYzM0Ngo=",
};

static std::string modulus[NumTestRootKeys] = {
    // Root 1 modulus
    "00:ad:2c:e9:d1:9f:31:14:5b:3a:7f:74:a2:c4:cf:"
    "e2:16:78:be:1c:6e:46:7d:db:1f:8e:6e:df:28:d4:"
    "a7:44:79:82:eb:9e:a8:6b:35:93:a5:48:02:d0:60:"
    "ab:ce:cf:31:63:02:11:a1:3c:85:6c:84:19:01:07:"
    "f8:70:9a:40:d8:42:95:22:45:b1:3e:c9:47:b9:75:"
    "26:08:64:45:1a:da:0e:a7:e9:bf:60:48:19:d9:3d:"
    "fa:34:6d:1d:6c:53:ae:cc:f0:c6:2e:75:30:d8:4c:"
    "32:66:b3:d9:a8:b3:ca:23:0b:16:3f:aa:87:80:8d:"
    "05:45:6c:91:ec:54:28:19:f6:2a:8e:32:ac:6d:2d:"
    "97:b7:6a:05:90:3b:7c:d7:6d:57:29:8a:f1:2c:85:"
    "9e:de:06:20:52:cb:70:a8:27:73:4f:cd:ab:02:dc:"
    "e9:2e:f3:15:0f:be:11:b5:ee:6a:1d:f6:fc:5c:3d:"
    "ae:9e:d9:66:9a:7a:44:f6:33:32:87:36:0e:74:da:"
    "ac:51:8b:86:4f:2d:cd:04:02:4d:83:93:a0:01:50:"
    "08:35:6a:dc:a0:52:5a:48:d8:e5:05:9b:87:62:d9:"
    "3b:e3:5e:28:d8:34:a4:6e:9a:48:e6:65:a2:97:f9:"
    "79:bb:8a:d2:61:05:a3:20:75:a1:ce:a7:7b:71:d2:"
    "7a:6f",

    // Root 2 modulus
    "00:c9:9b:71:66:79:01:ae:66:73:14:5e:c6:34:0f:"
    "57:61:41:5a:05:29:75:32:a7:0b:12:0a:86:5b:d3:"
    "d4:c9:2e:3c:2c:ae:6b:ec:f0:57:0c:29:c5:e1:ee:"
    "6e:d3:85:5c:28:9d:8f:28:f2:c7:c1:1b:fd:fd:91:"
    "98:41:e7:95:25:85:68:04:8a:26:10:e2:12:5d:75:"
    "c7:29:ff:74:3f:06:0a:b3:01:6a:5c:82:f3:07:d0:"
    "1a:61:4b:df:b6:0a:76:9a:e6:df:56:3f:78:12:2a:"
    "b9:18:ab:c5:c2:be:f2:a9:8d:d8:aa:3e:ad:49:25:"
    "8d:8b:a7:95:f8:52:6d:e3:2a:6d:87:80:88:a7:26:"
    "4f:ae:b8:b1:9e:f2:34:da:05:0e:48:5d:57:c3:09:"
    "3c:3d:58:0c:d9:af:fd:14:a5:9f:94:54:98:78:de:"
    "e9:f7:b3:91:91:d5:e7:03:cd:97:79:85:fd:87:64:"
    "47:32:89:ad:64:c0:82:b4:da:63:5a:68:a8:2e:05:"
    "17:c9:07:92:e9:0a:b5:43:6c:1f:00:f3:99:ab:e6:"
    "a6:1d:ac:aa:fc:3c:72:e5:d6:fd:29:b2:1e:d7:70:"
    "1d:6c:4b:f6:04:2a:10:97:22:f5:7c:a4:81:41:68:"
    "91:1a:19:12:c4:fc:2b:2a:eb:1e:2e:9f:74:8f:5c:"
    "63:85",

    // Root 3 modulus
    "00:c1:1c:b8:91:bd:9e:5c:55:f2:4c:f5:46:7b:07:"
    "c1:6d:95:4d:a0:ea:0b:38:b2:9d:ab:12:ef:9c:59:"
    "7b:fb:d8:f0:5b:04:b8:f4:63:59:f2:4a:5f:3b:79:"
    "c2:53:5a:ac:a0:3d:7d:f1:f2:da:4c:1b:f3:f4:65:"
    "67:b2:6a:e3:a7:12:ee:03:c8:a6:f1:1a:bc:3b:9e:"
    "a4:36:cd:97:50:3a:9f:b4:21:46:08:24:81:a9:b0:"
    "f0:c3:8b:dc:1e:e4:dd:79:01:77:c6:ac:cb:d9:ec:"
    "c8:7b:88:43:a8:5d:eb:dd:4b:bb:56:4c:cd:98:b8:"
    "48:71:c5:b4:dd:21:01:ef:10:d8:1f:bb:5a:b7:fe:"
    "a7:00:4f:c2:b9:3a:2a:42:1d:d5:f2:04:d0:ba:54:"
    "5c:90:e3:61:00:61:b7:08:af:f9:0e:a1:e9:93:f1:"
    "94:f0:01:66:58:ef:2e:9b:66:cf:c9:31:96:f6:94:"
    "06:71:08:f9:4e:27:8b:a4:85:99:99:2e:ab:f3:f3:"
    "72:17:4e:7a:07:b6:fd:3d:9d:74:66:8c:e4:2d:37:"
    "54:56:f1:32:dd:d3:1e:41:72:d5:24:19:f7:66:97:"
    "1c:64:1e:8c:f1:46:83:d5:43:16:ec:75:c4:9f:4b:"
    "1a:ae:8e:74:e9:c3:16:77:1d:3a:83:a1:0c:46:fd:"
    "c6:6f",
};

static size_t exponent[NumTestRootKeys] = {
    65537, // 0x10001
    65537, // 0x10001
    65537, // 0x10001
};

static std::string siggies[NumTestRootKeys] = {
    // protected signed with root1
    "UPLul7v7l2jTAGzrQDSQJZgTSk+ZzbskxXY8lHNGEYILmcMsgvB2MnRJis7NFFiH"
    "QHyE4b6YdThgvgKLWwTLaLc7isQIluLMQt6AC3Dh0wHgplH+B80LwbHS83Tg1EiS"
    "mdH2IkF2syFAT0UG/g2jqreLaiG79iAsNWI0+2/7bAvMOc/DCdYrjIuOVVtUEjOp"
    "u0rxPiGPsFOC8M8YV8H23xF8l9sj4R82zkbYonvtrJZtQ/1xyhXdWdy/qVsjSUMI"
    "TyuIuzCx8tJrKvYy5yLLI4N/xJs48KFoyKDEbMvL8saKO3oxTja1nTPI9UOfTRHa"
    "zRorMmj/ACeDA21Ovws6VA", // removed 2 '=' from the end

    // protected signed with root2
    "qUtUfzf1k3QyIx70SObEB30b1LcIGu38kvFGEu5J9eTVcUgE+QSJ0R/LndIk/q1P"
    "Bqlk3KjbqWQKn6HERYR4r+/vG8PmHb0VEzA9mC5IO/4so2NxiGbJSVTHrznEw4EG"
    "D+Qb5L6RGcl5BBK2v3czWbi3Tv5+wu9v6Nzj1F4+ZYCYgMd4PJP0czfFN8//d2Zf"
    "EarxekrjbIi3pQ3V8qE6Abo7r4nEU65nK8ksB2rfXOcPYHCaOdNM+m1viXhf4Bay"
    "GG4HCLj8WqxLs/hFp196ki0GztKblvC9CnuPBk5YOnbmlT2j9I6FtXuzEhefQ0K3"
    "rut4jR3k5CtdoachHCrLAw", // removed 2 '=' from the end

    // protected signed with root3
    "DEdx66Dcilyk+/S9RUdz2Ag+1wuMoChVDrNFM4+Lt5WlZJhdcpaWms3KbsMVG6V4"
    "N7WFgMBdh8QLmqs8tirE4O4LEh1DR/V1/xJA41P3FQLk2wTpFGruDtcOxsaav36i"
    "7psZ75FBudXHUaWBVeIIRRNU8/Fq0dTucSdvwW2ZQLjgWqJPP+9tIrrpHpzI1Jkd"
    "JwImTU/k8anxAdZ/RB1TpCOA7F9REHQBBcvpn5wLo95x5iu5chSWoJP0zsZJgHZ+"
    "J4sJTX26RKF3NZMznW4DDY1keguTKmaCElVblTTW1LGvOplG+a05/ivAj4ySYC9J"
    "vQV+sDyuyQwfjBFn2j36MQ", // removed 2 '=' from the end
};

class TestRSAPrivateKey
{
    aduc::rootkeypkgtestutils::rootkeys m_rootkey;

public:
    TestRSAPrivateKey(aduc::rootkeypkgtestutils::rootkeys rootkey) : m_rootkey{ rootkey }
    {
    }

    ~TestRSAPrivateKey()
    {
    }

    TestRSAPrivateKey(const TestRSAPrivateKey&) = delete;
    TestRSAPrivateKey& operator=(const TestRSAPrivateKey&) = delete;
    TestRSAPrivateKey(TestRSAPrivateKey&&) = delete;
    TestRSAPrivateKey& operator=(TestRSAPrivateKey&&) = delete;

    std::string SignData(const std::string& data) const
    {
        UNREFERENCED_PARAMETER(data);
        // TODO: Use OpenSSL to actually create the signature using private key and data.
        // For now, return hard-coded signatuers for now per root key.
        auto index = static_cast<typename std::underlying_type<aduc::rootkeypkgtestutils::rootkeys>::type>(m_rootkey);
        return siggies[index];
    }
};

class TestRSAPublicKey
{
    aduc::rootkeypkgtestutils::rootkeys m_rootkey;

public:
    TestRSAPublicKey(aduc::rootkeypkgtestutils::rootkeys rootkey) : m_rootkey(rootkey)
    {
    }

    ~TestRSAPublicKey()
    {
    }

    TestRSAPublicKey(const TestRSAPublicKey&) = delete;
    TestRSAPublicKey& operator=(const TestRSAPublicKey&) = delete;
    TestRSAPublicKey(TestRSAPublicKey&&) = delete;
    TestRSAPublicKey& operator=(TestRSAPublicKey&&) = delete;

    // returns hex-encoded with colon delimiter
    std::string GetModulus() const
    {
        // TODO: Use OpenSSL to get the modulus from public key
        // For now, return hard-coded ones per pre-canned root key.
        auto index = static_cast<typename std::underlying_type<aduc::rootkeypkgtestutils::rootkeys>::type>(m_rootkey);
        return std::string{ modulus[index] };
    }

    size_t GetExponent() const
    {
        // TODO: Use OpenSSL to get the exponent from public key
        // For now, return hard-coded ones per pre-canned root key.
        auto index = static_cast<typename std::underlying_type<aduc::rootkeypkgtestutils::rootkeys>::type>(m_rootkey);
        return exponent[index];
    }

    // returns base64 of sha256 of pubkey
    std::string getSha256HashOfPublicKey() const
    {
        // TODO: Calculate the hash of the dynamically-generated public key.
        // For now, return hard-coded ones.
        auto index = static_cast<typename std::underlying_type<aduc::rootkeypkgtestutils::rootkeys>::type>(m_rootkey);
        return pubhashies[index];
    }

    bool VerifySignature(const std::string& signature) const
    {
        // TODO: use OpenSSL to calculate signature
        // For now, compare with hard-coded signatures
        auto index = static_cast<typename std::underlying_type<aduc::rootkeypkgtestutils::rootkeys>::type>(m_rootkey);
        return siggies[index] == signature;
    }
};

class TestRSAKeyPair
{
    TestRSAPrivateKey* privateKey;
    TestRSAPublicKey* publicKey;

    aduc::rootkeypkgtestutils::rootkeys m_rootkey;

public:
    TestRSAKeyPair(aduc::rootkeypkgtestutils::rootkeys rootkey) : m_rootkey(rootkey)
    {
        // TODO: generate private/public key pairs.
        // for now, rootkey is specified.
        privateKey = new TestRSAPrivateKey{ rootkey };
        publicKey = new TestRSAPublicKey{ rootkey };
    }

    ~TestRSAKeyPair()
    {
        delete privateKey;
        delete publicKey;
    }

    TestRSAKeyPair(const TestRSAKeyPair&) = delete;
    TestRSAKeyPair& operator=(const TestRSAKeyPair&) = delete;
    TestRSAKeyPair(TestRSAKeyPair&&) = delete;
    TestRSAKeyPair& operator=(TestRSAKeyPair&&) = delete;

    const TestRSAPublicKey& GetPublicKey() const
    {
        return *publicKey;
    }

    const TestRSAPrivateKey& GetPrivateKey() const
    {
        return *privateKey;
    }
};

} // namespace rootkeypkgtestutils
} // namespace aduc

#endif // ROOTKEYPKGTESTUTILS_HPP
