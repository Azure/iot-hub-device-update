#include "rootkeypkgtestutils.hpp"
#include <parson.h>
#include <string>

/*
{
    "protected": {
        "version": 3,
        "published": 1680544698,
        "disabledRootKeys": [
            "ADU.200702.R.T"
        ],
        "disabledSigningKeys": [
            {
                "alg": "SHA256",
                "hash": "abc123"
            }
        ],
        "rootKeys": {
            "ADU.200702.R.T": {
                "keyType": "RSA",
                "n": "ucKAJMkskVVKjtVLFdraMSd0cTa2Vcndkle540smg3a2v4hYXoHWBaA0tkZj5VM1fWR-XcjHJ9NRh74TzsHqPJODXn085tWGMwOzUEPhOSAzRaY-FCr23SIqM6AHCYPxziKbz9kEcD6e043UyCRMyLf8fQJ3SOvBXCNoVSkiQ8rwcDeHjFiSzk_BLy0JGRjfzJZF8l-q1N-Vqpq3VtOmQJphblSL6bC9AR1GNrvaJbHiSciaFvuiucneVBu3B6bY0wEin20x_CrjTNmiWEtuY_zoUxJGLGQVHkzJRRAQweHxw_FDSMd3UhiINRuN7Qb3r_S9HPoFNkZvaOUVOVe7WUY0jAFIzVUEcq2CTx43p0XvaLeYEz-DsG-RlPkkT2i-1ykEhwtJfsKGDTIP5mPDslZkTUScgZFRMToJdwOtGKkAzGXQPlvtf3IL49fUTM4r8dpIc7E1N2Djt94__kcdY1e8JxfgRH7RoiQCATHep6-mQW5UKq_onJW2bNo7i9Gb",
                "e": 65537
            },
            "ADU.200703.R.T": {
                "keyType": "RSA",
                "n": "2WisuSoVDzKsz02BmP2bulWJzwrDH4hBIgKaz4Ithol_LYOSpk0knonvCiEB5Zb9kKMUAlOdKluvO2nGKp95kqZzm77thqjUbe5bZyFOCqPlPH-0nUHhg_oHXvX_5Oz3l-7KhMG0bUWzQ72nkmUHViexAPBpY-u4zZTRr8MONbGtMInrVI7SJTbToZ1zzM-b04o8wxlNfNJspjY2P_82mmJXZKlRlGdWuLYLoeXhKosfSu9MP1aLjC-puEmYmZ-dsoMg3_DHhluC-7VN2r8dAm3e3cTKuL3bNvGguTwTnccMEw1VxLMUsnpsDtxFHjebwpRVvs76JJsW3fllYZZ2T1l5WWxQbWDCdui7dDvnAfEww7Juxw4dKdXnlSorBGa5-QxZ0OnfKQTYbQweA_GehkKPPvku9mzK-n0PxfsaQMLS1-JfXgiVNQ4erhu_625iKFwWKlfuqOuZWiJMkFTK-NBmpDKAaBtxdH_5Xgc3ZA7SMymyVfw-9UmWv-ooK1F9",
                "e": 65537
            }
        }
    },
    "signatures": [
        {
            "alg": "RS256",
            "sig": ""
        },
        {
            "alg": "RS256",
            "sig": ""
        }
    ]
}

*/

/**
 * @brief Gets the public key modulus.
 * @return std::string The modulus as base64 url encoding.
 */
std::string TestRSAKeyPair::GetPubKeyModulus() const
{
    return "";
}

/**
 * @brief Gets the public key exponent.
 * @return size_t The exponent.
 */
size_t TestRSAKeyPair::GetPubKeyExponent() const
{
    return 0;
}

/**
 * @brief Gets the SHA256 hash of the public key.
 * @return std::string The SHA256 hash as base64.
 */
std::string TestRSAKeyPair::getSha256HashOfPublicKey() const
{
    return "";
}

/**
 * @brief Signs the protected properties json obj string.
 * @param data the snippet of serialized json that is the value of protected property.
 * @return std::string The signature as base64 url encoding.
 */
std::string TestRSAKeyPair::SignData(const std::string& data) const
{
    return "";
}

/**
 * @brief Verifies a signature
 * @param signature The base64 url encoded signature to verify.
 * @return bool true when signature is good.
 */
bool TestRSAKeyPair::VerifySignature(const std::string& signature) const
{
    return false;
}

/**
 * @brief Gets the path to minimal rootkey package json with correct structure
 * @return std::string the path to the test json file.
 */
std::string get_minimal_rootkey_package_json_path()
{
    std::string path{ ADUC_TEST_DATA_FOLDER };
    path += "/rootkeypackage_utils/rootkeypackage_minimal.json";
    return path;
}

/**
 * @brief Gets a json value of minimal rootkey package template json file that
 * has empty arrays for following properties:
 * disabledRootKeys, disabledSigningKeys, rootKeys, signatures
 * @return JSON_Value* The json value, or nullptr on error.
 */
JSON_Value* get_minimal_rootkeypkg_json_value()
{
    return json_parse_file(get_minimal_rootkey_package_json_path().c_str());
}

/**
 * @brief Adds a rootkey to the rootKeys array of the rootkey package json.
 * @param root_value The JSON root value
 * @param kid The kid string of the rootkey
 * @param rsa_n_modulus_base64url The RSA modulus of the public key, encoded as base64 url format.
 * @return bool true on success.
 */
bool add_test_rootkeypkg_rootkey(JSON_Value* root_value, const std::string& kid, const std::string& rsa_n_modulus_base64url)
{
    return true;
}

/**
 * @brief Adds a kid of disabled rootkey to the disabledRootKeys array of the rootkey package json.
 * @param kid The kid string of the disabled rootkey
 * @return bool true on success.
 */
bool add_test_rootkeypkg_disabledRootKey(JSON_Value* root_value, const std::string& kid)
{
    return true;
}

/**
 * @brief Adds a kid of disabled signing key to the disabledSigningKeys array of the rootkey package json.
 * @param sha256_hash The sha256 hash of the public key of the disabled signing key.
 * @return bool true on success.
 */
bool add_test_rootkeypkg_disabledSigningKey(JSON_Value* root_value, const std::string& sha256_hash)
{
    return true;
}

/**
 * @brief Adds a signature of the rootkey package protected element to the signatures property.
 * @param rs256_signature The RS256 signature of the rootkey package's protected property.
 * @return bool true on success.
 */
bool add_test_rootkeypkg_signature(JSON_Value* root_value, const std::string& rs256_signature)
{
    return true;
}

