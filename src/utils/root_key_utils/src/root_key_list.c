/**
 * @file root_key_utility.c
 * @brief Implements a static list of root keys used to verify keys and messages within crypto_lib and it's callers.
 * these have exponents and parameters that have been extracted from their certs
 * to ease the computation process but remain the same as the ones issued by the Authority
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "root_key_list.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

//
// Hardcoded Root Keychain Definitions
//

// clang-format off
static const RSARootKey HardcodedRSARootKeyList[] =
{
    {
        // kid
        "ADU.200702.R",
        // N
        "ANVCLq8RVKNQZYeiTVu6GvupMt_pmV"
        "8FRcivvTUdiegnJ1ijqO7FxR5P95Km"
        "EgZ9PX2wB_Ysf95tKvW8SbwV7_CByz"
        "-ITycdiHEoYAi2GdLSOdAFHzx2hnG7"
        "WVi8sYh7q1YovzFzRDIQ_T3Tllz_Tl"
        "yza_-LhJuLgLhJ0H361kBYdk3Acid1"
        "y5ovm7SfDyXxHMUbC1owfS-476cmWF"
        "Ov1R1VAVEN6RuiDz_X6R0gQabmFAqu"
        "_vIcKtbkBHv2FH7sD5eD-lj6gTYhua"
        "Mr-tlhCxqU98G-f0AUSsn6NX_vZnAA"
        "sf3b12ENO1h0Z5SJdXaWfJGH0o4Rl-"
        "57h2yaL0XYZT9ScJgqy8gEY_XJR89w"
        "9O1kp3SlI4-27fcc07AcZFcSWqmBhB"
        "-g51AZlrSCsaxI4-EygstAH6zEWbwQ"
        "NFGC-SiNqB6b9XlFdbLcmhFDCL5hzJ"
        "rEy3c2_4PdqHFPUY4Oe036eZiNvvyC"
        "fkBIqRIBqNl-86Ub8fuQdz5AhxjJq9n"
        "3eQ",
        // E
        65537
    },

    {
        // kid
        "ADU.200703.R",
        // N
        "ALnCgCTJLJFVSo7VSxXa2jEndHE2tl"
        "XJ3ZJXueNLJoN2tr-IWF6B1gWgNLZG"
        "Y-VTNX1kfl3IxyfTUYe-E87B6jyTg1"
        "59PObVhjMDs1BD4TkgM0WmPhQq9t0i"
        "KjOgBwmD8c4im8_ZBHA-ntON1MgkTM"
        "i3_H0Cd0jrwVwjaFUpIkPK8HA3h4xY"
        "ks5PwS8tCRkY38yWRfJfqtTflaqat1"
        "bTpkCaYW5Ui-mwvQEdRja72iWx4knI"
        "mhb7ornJ3lQbtwem2NMBIp9tMfwq40"
        "zZolhLbmP86FMSRixkFR5MyUUQEMHh"
        "8cPxQ0jHd1IYiDUbje0G96_0vRz6BT"
        "ZGb2jlFTlXu1lGNIwBSM1VBHKtgk8e"
        "N6dF72i3mBM_g7BvkZT5JE9ovtcpBI"
        "cLSX7Chg0yD-Zjw7JWZE1EnIGRUTE6"
        "CXcDrRipAMxl0D5b7X9yC-PX1EzOK_"
        "HaSHOxNTdg47feP_5HHWNXvCcX4ER-"
        "0aIkAgEx3qevpkFuVCqv6JyVtmzaO4"
        "vRmw",
        // E
        65537
    },

#ifdef BUILD_WITH_TEST_KEYS
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
        65537
    },
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
        65537
    },
#endif
};
// clang-format on

/**
 * @brief
 *
 * @return the current list of hardcoded keys
 */
const RSARootKey* RootKeyList_GetHardcodedRsaRootKeys(void)
{
    return HardcodedRSARootKeyList;
}
