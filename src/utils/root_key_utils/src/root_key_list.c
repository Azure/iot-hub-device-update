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
#if EMBED_TEST_ROOT_KEYS == 1
    {
        "ADU.200702.R.T",
        "ALnCgCTJLJFVSo7VSxXa2jEndHE2tlXJ3ZJXueNLJoN2tr-IWF6B1gWgNLZGY-VT"
        "NX1kfl3IxyfTUYe-E87B6jyTg159PObVhjMDs1BD4TkgM0WmPhQq9t0iKjOgBwmD"
        "8c4im8_ZBHA-ntON1MgkTMi3_H0Cd0jrwVwjaFUpIkPK8HA3h4xYks5PwS8tCRkY"
        "38yWRfJfqtTflaqat1bTpkCaYW5Ui-mwvQEdRja72iWx4knImhb7ornJ3lQbtwem"
        "2NMBIp9tMfwq40zZolhLbmP86FMSRixkFR5MyUUQEMHh8cPxQ0jHd1IYiDUbje0G"
        "96_0vRz6BTZGb2jlFTlXu1lGNIwBSM1VBHKtgk8eN6dF72i3mBM_g7BvkZT5JE9o"
        "vtcpBIcLSX7Chg0yD-Zjw7JWZE1EnIGRUTE6CXcDrRipAMxl0D5b7X9yC-PX1EzO"
        "K_HaSHOxNTdg47feP_5HHWNXvCcX4ER-0aIkAgEx3qevpkFuVCqv6JyVtmzaO4vR"
        "mw",
        65537
    },
    {

        "ADU.200703.R.T",

        "ANlorLkqFQ8yrM9NgZj9m7pVic8Kwx-IQSICms-CLYaJfy2DkqZNJJ6J7wohAeWW"
        "_ZCjFAJTnSpbrztpxiqfeZKmc5u-7Yao1G3uW2chTgqj5Tx_tJ1B4YP6B171_-Ts"
        "95fuyoTBtG1Fs0O9p5JlB1YnsQDwaWPruM2U0a_DDjWxrTCJ61SO0iU206Gdc8zP"
        "m9OKPMMZTXzSbKY2Nj__NppiV2SpUZRnVri2C6Hl4SqLH0rvTD9Wi4wvqbhJmJmf"
        "nbKDIN_wx4Zbgvu1Tdq_HQJt3t3Eyri92zbxoLk8E53HDBMNVcSzFLJ6bA7cRR43"
        "m8KUVb7O-iSbFt35ZWGWdk9ZeVlsUG1gwnbou3Q75wHxMMOybscOHSnV55UqKwRm"
        "ufkMWdDp3ykE2G0MHgPxnoZCjz75LvZsyvp9D8X7GkDC0tfiX14IlTUOHq4bv-tu"
        "YihcFipX7qjrmVoiTJBUyvjQZqQygGgbcXR_-V4HN2QO0jMpslX8PvVJlr_qKCtR"
        "fQ",

        65537
    }
#else
    {
        "ADU.200702.R",
        "ANVCLq8RVKNQZYeiTVu6GvupMt_pmV8FRcivvTUdiegnJ1ijqO7FxR5P95KmEgZ9"
        "PX2wB_Ysf95tKvW8SbwV7_CByz-ITycdiHEoYAi2GdLSOdAFHzx2hnG7WVi8sYh7"
        "q1YovzFzRDIQ_T3Tllz_Tlyza_-LhJuLgLhJ0H361kBYdk3Acid1y5ovm7SfDyXx"
        "HMUbC1owfS-476cmWFOv1R1VAVEN6RuiDz_X6R0gQabmFAqu_vIcKtbkBHv2FH7s"
        "D5eD-lj6gTYhuaMr-tlhCxqU98G-f0AUSsn6NX_vZnAAsf3b12ENO1h0Z5SJdXaW"
        "fJGH0o4Rl-57h2yaL0XYZT9ScJgqy8gEY_XJR89w9O1kp3SlI4-27fcc07AcZFcS"
        "WqmBhB-g51AZlrSCsaxI4-EygstAH6zEWbwQNFGC-SiNqB6b9XlFdbLcmhFDCL5h"
        "zJrEy3c2_4PdqHFPUY4Oe036eZiNvvyCfkBIqRIBqNl-86Ub8fuQdz5AhxjJq9n3"
        "eQ",
        65537
    },
    {

        "ADU.200703.R",
        "ALKjsnQW-rsg-VJ25ic-gEHG_s8w-ciW9VkKqoHnUYOKxPUXOi8q5lfUcc6KPe-a"
        "VXY-meLCrkzuLbh49aJOKPKcTjllvOzkDeXjOKhZqwikG7T0oFKjOLNGIRPMPGgG"
        "3v4AppJu3kxHENYcnCT1zXDh9Wp8aBMd4cX2qE8hn4Z8RMWKmRzF0wabWnGdCRzD"
        "ZDFqxReVHV0q8VXHZtTo9dmpW4yibGJgBTfXMrBzy_dLNickIYw4CrgY_vUVYDWL"
        "Ne8eD4imE417fe-z57DJphxwe8zyKYuH972dtohvrHP_cvLvSCeWcoYGolzjfc6w"
        "nuXC2U7E8394B0tliEUMEeWWVjSILRYOWULS99ntHe3JN3dEfuOENp9YE-9v5MNE"
        "1HcGis9byIAcophlCzXcc8hp0F7oJUOe9tirBa9RKSNVQFgQ6rjizV15zOzftFuY"
        "x_rj0mwmzi4sVuDPje79kxIvAEmNHII4VqZdeURKGvPcFhCzwS0nEf4bmAXko2Ax"
        "mQ",
        65537
    }
#endif
};
// clang-format on

/**
 * @brief Returns the hardcoded array of hardcoded RSARootKeys
 *
 * @return the current list of hardcoded keys
 */
const RSARootKey* RootKeyList_GetHardcodedRsaRootKeys(void)
{
    return HardcodedRSARootKeyList;
}

/**
 * @brief Returns the number of hardcoded RSARootKeys
 *
 * @return the current size of the list of hardcoded root keys
 */
size_t RootKeyList_numHardcodedKeys(void)
{
    return ARRAY_SIZE(HardcodedRSARootKeyList);
}
