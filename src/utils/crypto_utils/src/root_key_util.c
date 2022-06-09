/**
 * @file root_key_utility.c
 * @brief Implements a static list of root keys used to verify keys and messages within crypto_lib and it's callers.
 * these have exponents and parameters that have been extracted from their certs
 * to ease the computation process but remain the same as the ones issued by the Authority
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "crypto_lib.h"
#include "root_key_util.h"
//
// Root Key Type Definitions
//

typedef struct tagRSARootKey
{
    const char* kid;
    const char* N;
    const char* e;
} RSARootKey;

//
// Root Keychain Definitions
//

// clang-format off
static const RSARootKey RSARootKeyList[] =
{
    {
        // kid
        "ADU.200702.R",
        // N
        "00d5422eaf1154a3506587a24d5bba"
        "1afba932dfe9995f0545c8afbd351d"
        "89e8272758a3a8eec5c51e4ff792a6"
        "12067d3d7db007f62c7fde6d2af5bc"
        "49bc15eff081cb3f884f271d887128"
        "6008b619d2d239d0051f3c768671bb"
        "5958bcb1887bab5628bf3173443210"
        "fd3dd3965cff4e5cb36bff8b849b8b"
        "80b849d07dfad64058764dc0722775"
        "cb9a2f9bb49f0f25f11cc51b0b5a30"
        "7d2fb8efa7265853afd51d5501510d"
        "e91ba20f3fd7e91d2041a6e6140aae"
        "fef21c2ad6e4047bf6147eec0f9783"
        "fa58fa813621b9a32bfad9610b1a94"
        "f7c1be7f40144ac9fa357fef667000"
        "b1fddbd7610d3b5874679489757696"
        "7c9187d28e1197ee7b876c9a2f45d8"
        "653f5270982acbc80463f5c947cf70"
        "f4ed64a774a5238fb6edf71cd3b01c"
        "6457125aa981841fa0e7501996b482"
        "b1ac48e3e13282cb401facc459bc10"
        "345182f9288da81e9bf5794575b2dc"
        "9a114308be61cc9ac4cb7736ff83dd"
        "a8714f518e0e7b4dfa79988dbefc82"
        "7e4048a91201a8d97ef3a51bf1fb90"
        "773e408718c9abd9f779",
        // E
        "010001"
    },

    {
        // kid
        "ADU.200703.R",
        // N
        "00b2a3b27416fabb20f95276e6273e"
        "8041c6fecf30f9c896f5590aaa81e7"
        "51838ac4f5173a2f2ae657d471ce8a"
        "3def9a55763e99e2c2ae4cee2db878"
        "f5a24e28f29c4e3965bcece40de5e3"
        "38a859ab08a41bb4f4a052a338b346"
        "2113cc3c6806defe00a6926ede4c47"
        "10d61c9c24f5cd70e1f56a7c68131d"
        "e1c5f6a84f219f867c44c58a991cc5"
        "d3069b5a719d091cc364316ac51795"
        "1d5d2af155c766d4e8f5d9a95b8ca2"
        "6c62600537d732b073cbf74b362724"
        "218c380ab818fef51560358b35ef1e"
        "0f88a6138d7b7defb3e7b0c9a61c70"
        "7bccf2298b87f7bd9db6886fac73ff"
        "72f2ef482796728606a25ce37dceb0"
        "9ee5c2d94ec4f37f78074b6588450c"
        "11e5965634882d160e5942d2f7d9ed"
        "1dedc93777447ee384369f5813ef6f"
        "e4c344d477068acf5bc8801ca29865"
        "0b35dc73c869d05ee825439ef6d8ab"
        "05af51292355405810eab8e2cd5d79"
        "ccecdfb45b98c7fae3d26c26ce2e2c"
        "56e0cf8deefd93122f00498d1c8238"
        "56a65d79444a1af3dc1610b3c12d27"
        "11fe1b9805e4a3603199",
        // E
        "010001"
    },

#ifdef BUILD_WITH_TEST_KEYS
    {
        // kid
        "ADU.200703.R.T",
        // N
        "00d968acb92a150f32accf4d8198fd"
        "9bba5589cf0ac31f884122029acf82"
        "2d86897f2d8392a64d249e89ef0a21"
        "01e596fd90a31402539d2a5baf3b69"
        "c62a9f7992a6739bbeed86a8d46dee"
        "5b67214e0aa3e53c7fb49d41e183fa"
        "075ef5ffe4ecf797eeca84c1b46d45"
        "b343bda79265075627b100f06963eb"
        "b8cd94d1afc30e35b1ad3089eb548e"
        "d22536d3a19d73cccf9bd38a3cc319"
        "4d7cd26ca636363fff369a625764a9"
        "51946756b8b60ba1e5e12a8b1f4aef"
        "4c3f568b8c2fa9b84998999f9db283"
        "20dff0c7865b82fbb54ddabf1d026d"
        "deddc4cab8bddb36f1a0b93c139dc7"
        "0c130d55c4b314b27a6c0edc451e37"
        "9bc29455becefa249b16ddf9656196"
        "764f5979596c506d60c276e8bb743b"
        "e701f130c3b26ec70e1d29d5e7952a"
        "2b0466b9f90c59d0e9df2904d86d0c"
        "1e03f19e86428f3ef92ef66ccafa7d"
        "0fc5fb1a40c2d2d7e25f5e0895350e"
        "1eae1bbfeb6e62285c162a57eea8eb"
        "995a224c9054caf8d066a43280681b"
        "71747ff95e0737640ed23329b255fc"
        "3ef54996bfea282b517d",
        // E
        "010001"
    },

    {
        // kid
        "ADU.200702.R.T",
        // N
        "00b9c28024c92c91554a8ed54b15da"
        "da3127747136b655c9dd9257b9e34b"
        "268376b6bf88585e81d605a034b646"
        "63e553357d647e5dc8c727d35187be"
        "13cec1ea3c93835e7d3ce6d5863303"
        "b35043e139203345a63e142af6dd22"
        "2a33a0070983f1ce229bcfd904703e"
        "9ed38dd4c8244cc8b7fc7d027748eb"
        "c15c236855292243caf07037878c58"
        "92ce4fc12f2d091918dfcc9645f25f"
        "aad4df95aa9ab756d3a6409a616e54"
        "8be9b0bd011d4636bbda25b1e249c8"
        "9a16fba2b9c9de541bb707a6d8d301"
        "229f6d31fc2ae34cd9a2584b6e63fc"
        "e85312462c64151e4cc9451010c1e1"
        "f1c3f14348c777521888351b8ded06"
        "f7aff4bd1cfa0536466f68e5153957"
        "bb5946348c0148cd550472ad824f1e"
        "37a745ef68b798133f83b06f9194f9"
        "244f68bed72904870b497ec2860d32"
        "0fe663c3b256644d449c819151313a"
        "097703ad18a900cc65d03e5bed7f72"
        "0be3d7d44cce2bf1da4873b1353760"
        "e3b7de3ffe471d6357bc2717e0447e"
        "d1a224020131dea7afa6416e542aaf"
        "e89c95b66cda3b8bd19b",
        // E
        "010001"
    },
#endif
};
// clang-format on

/**
 * @brief Helper function that returns a CryptoKeyHandle associated with the kid
 * @details The caller must free the returned Key with the FreeCryptoKeyHandle() function
 * @param kid the key identifier associated with the key
 * @returns the CryptoKeyHandle on success, null on failure
 */
CryptoKeyHandle GetKeyForKid(const char* kid)
{
    //
    // Iterate through the RSA Root Keys
    //
    const unsigned numberKeys = (sizeof(RSARootKeyList) / sizeof(RSARootKey));
    for (unsigned i = 0; i < numberKeys; ++i)
    {
        if (strcmp(RSARootKeyList[i].kid, kid) == 0)
        {
            return RSAKey_ObjFromStrings(RSARootKeyList[i].N, RSARootKeyList[i].e);
        }
    }

    return NULL;
}
