/**
 * @file adu_core_json_ut.cpp
 * @brief Unit tests for adu_core_json.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <catch2/catch.hpp>
using Catch::Matchers::Equals;

#include <map>
#include <memory>
#include <sstream>
#include <string>

#include "aduc/adu_core_export_helpers.h" // ADUC_FileEntityArray_Free
#include "aduc/adu_core_exports.h" // ADUC_FileEntity
#include "aduc/adu_core_json.h"
#include "aduc/hash_utils.h"
#include "aduc/parser_utils.h"
#include <aduc/calloc_wrapper.hpp>

using ADUC::StringUtils::cstr_wrapper;

TEST_CASE("ADUCITF_StateToString: Valid")
{
    // clang-format off
    const std::map<ADUCITF_State, std::string> files{
        { ADUCITF_State_Idle, "Idle" },
        { ADUCITF_State_DownloadStarted, "DownloadStarted" },
        { ADUCITF_State_DownloadSucceeded, "DownloadSucceeded" },
        { ADUCITF_State_InstallStarted, "InstallStarted" },
        { ADUCITF_State_InstallSucceeded, "InstallSucceeded" },
        { ADUCITF_State_ApplyStarted, "ApplyStarted" },
        { ADUCITF_State_Failed, "Failed" }
    };
    // clang-format on
    for (const auto& val : files)
    {
        CHECK(ADUCITF_StateToString(val.first) == val.second);
    }
}

TEST_CASE("ADUCITF_StateToString: Invalid")
{
    CHECK_THAT(ADUCITF_StateToString((ADUCITF_State)(65535)), Equals("<Unknown>"));
}

TEST_CASE("ADUC_Json_GetRoot: Valid")
{
    CHECK(ADUC_Json_GetRoot(R"({ })") != nullptr);

    CHECK(ADUC_Json_GetRoot(R"({ "a": "42" })") != nullptr);
}

TEST_CASE("ADUC_Json_GetRoot: Invalid")
{
    SECTION("Invalid JSON returns nullptr")
    {
        CHECK(ADUC_Json_GetRoot(R"(Not JSON)") == nullptr);
    }

    SECTION("Invalid root object returns nullptr")
    {
        CHECK(ADUC_Json_GetRoot(R"(["a"])") == nullptr);
    }
}

/*
 * Example Manifest with Signature:
 {
  "updateManifest": "{
    "manifestVersion":"1.0",
    "updateId":{
        "provider":"AduTest",
        "name":"ADU content team",
        "version":"2020.611.534.16"
    },
    "updateType":null,
    "installedCriteria":null,
    "files":{
        "00000":{
            "fileName":"setup.exe",
            "sizeInBytes":76,
            "hashes":{
                "sha256":"IhIIxBJpLfazQOk/PVi6SzR7BM0jf4HDqw+6gdZ3vp8="
            }
        }
    },
    "createdDateTime":"2020-06-12T00:38:13.9350278"
  }",
  "fileUrls":{
      "00000": "<URL>"
  }
  "updateManifestSignature": "ey...mw"
}
*/

TEST_CASE("ADUC_Json_GetRootAndValidateManifest: Valid Signature Value")
{
    std::stringstream manifest_json_strm;
    // clang-format off
    manifest_json_strm
        << R"({)"
        << R"("updateManifest":)"
        << R"("{)"
        <<     R"(\"manifestVersion\":\"2.0\",)"
        <<     R"(\"updateId\":{)"
        <<         R"(\"provider\":\"adu\",)"
        <<         R"(\"name\":\"test\",)"
        <<         R"(\"version\":\"2.0.0.0\")"
        <<     R"(},)"
        <<     R"(\"updateType\":\"SWUpdate\",)"
        <<     R"(\"installedCriteria\":\"1.2.3.4\",)"
        <<     R"(\"files\":{)"
        <<         R"(\"00000\":{)"
        <<             R"(\"fileName\":\"setup.exe\",)"
        <<             R"(\"sizeInBytes\":76,)"
        <<             R"(\"hashes\":{)"
        <<                 R"(\"sha256\":\"aGc0x9DB3CFgRwPYgX2kQsN0oUsS4/Zn4kP//f3QyLc=\")"
        <<             R"(})"
        <<         R"(})"
        <<     R"(},)"
        <<     R"(\"createdDateTime\":\"2020-10-02T22:15:56.8012002Z\")"
        << R"(}")"
        << R"(,)"
        << R"("updateManifestSignature":")"
        << R"(eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9pSlNVekkxTmlJc0ltdHBaQ0)"
        << R"(k2SWtGRVZTNHlNREEzTURJdVVpNVVJbjAuZXlKcmRIa2lPaUpTVTBFaUxDSnVJam9p)"
        << R"(ZGtwYWJGRjFjM1J3T1RsUFRpOXJXWEJ4TlVveFRtdG1hMGxDTjNrdldVbzJNbWRrVj)"
        << R"(JWRFlteE5XWGt3WjNRd1VVVjZXVGR4TjI1UFJHMDJTbXQxYldoTVpuTTNaa1JCVTA4)"
        << R"(elJIaG5SVlE0Y1dkWmQyNVdhMFpsTVVkVVowZEVXR28xVTFGQ2VIQkxVRWxQTlVRcl)"
        << R"(Z6TXdSSEpKTmxGSk4xRm1hVFpRWmxaT2FsVlVTRkI2U0VaNWFFMVRUVnByYWxreGVF)"
        << R"(eFBNMDFSWjBobmRXUjFNV1pzU0hGREszUmlhbms1WTI0emVtcDBNM2hUVWpKbU9URk)"
        << R"(RVa0p5VWxRelJ6Vm1WbFkxVjA1U1EwOWlWVU51YUdNeWFHaHlSRU15V0dreFFWaHli)"
        << R"(azlRZDNWRFJFbEZkSFIzYjBzd1QyNVFjV3BLVHk5SVVqRjJMelJPY3l0S1YwOU1VRT)"
        << R"(U0ZW1aRlNqSnBXVTlSWkdwbFpteEJPRVoyVFhWMGVYVndaVXRwUTJsclYzY3lOWFpw)"
        << R"(YjAxUldWSXdiMVozUjFwMU16aG1PSFJvTVVoRlZFSjJkVVZWTDFkeGVIQllXbVJQUm)"
        << R"(5wa09VTkhabVZsTW5FMGIzcFFNQzlQVUUxRWQxQlRNRzVZWm0xYVVHUlhhWHBwVDNW)"
        << R"(RlUwSndVRko1TjFwcFJYSjVSbVpNWjJnemNVWkVXQ3RYZDB3eFdXWmxaRzU1V1N0cF)"
        << R"(VWWjNiaXRpVTNWalVHeFRlRXdyUmxJNVNuZFFaRGhNZUU1bmFHUnZZbTFCTjA1MmNX)"
        << R"(SkNaakp6Wld0dE1HUkpTa0pRTm5wc2MxQmxURU5ZVW1VMU1IbE9SVk5rYUhWRlFuVn)"
        << R"(hNV1k0Y1ZZek1VVklSVmx6V0dRMGMwWm5WMVZ2Wm5kM1dIaFBTM0FpTENKbElqb2lR)"
        << R"(VkZCUWlJc0ltRnNaeUk2SWxKVE1qVTJJaXdpYTJsa0lqb2lRVVJWTGpJd01EY3dNaT)"
        << R"(VTTGxOVUluMC5RS0ktckVJSXBIZTRPZVd1dkN4cFNYam1wbC05czNlN1d2ZGRPcFNn)"
        << R"(R2dJaUN5T1hPYThjSFdlbnpzSFVMTUZBZEF0WVBnM2ZVTnl3QUNJOEI2U044UGhUc3)"
        << R"(E5TFg4QjR2c0l2QlZ2S2FrTU1ucjRuZXRlVHN5V0NJbEhBeG4ydThzSlQ3amVzMmtF)"
        << R"(Q2ZrNnpBeS16cElSSXhYaF8yVEl5Y2RNLVU0UmJYVDhwSGpPUTVfZTRnSFRCZjRlOD)"
        << R"(RaWTdsUEw0RGtTa3haR1dnaERhOTFGVjlXbjRtc050eDdhMEJ3NEtTMURVd1c2eXE0)"
        << R"(c2diTDFncVl6VG04bENaSDJOb1pDdjZZUUd4YVE5UkJ1ek5Eb0VMb0RXXzB4S3V1Vz)"
        << R"(Faa3UxSDN6c3lZNlNJMVp0bkFkdDAxcmxIck5fTTlKdTBZM3EyNzJiRWxWSnVnN3F6)"
        << R"(cGF5WXFucllOMTBHRWhIUW9KTkRxZnlPSHNLRG9FbTNOR1pKY2FSWVl4eHY2SVZxTF)"
        << R"(JDaFZidnJ3ekdkTXdmME95aE52THFGWlhMS1RWMmhPX2t6MXZWaVNwRi1PVXVWbW5u)"
        << R"(RC1ZMXFQb28xSTBJeVFYallxaGNlME01UkhiQ1drVk1Qbk5jZHFMQzZlTWhiTjJGT1)"
        << R"(pHSUxSOVRqcDIyMmxucGFTbnVmMHlVbDF4MiJ9.eyJzaGEyNTYiOiJjTGRjZnBBRnN)"
        << R"(yYjA3ak5zVVFzQ0hDZDBmb0J3RU5remQybTB1UWZMdGc4PSJ9.ghH76UBj3uPgpfYP)"
        << R"(3Px24QUB0ZmAu9BthyL90CwkAb7TRM6yjCMxU7V7P2YERa_ATnjKygTefSzxpzc1c7)"
        << R"(B86X7DyLNInp3TvRktPf6PWGMbZ_O2l7XeUNfhhJwUTsFok405AK6rWBSt49MpnKbZ)"
        << R"(yEss-dVEFddun1Zr71InV2gA_BtdhkP3qS7y16zp3SpLi1eqyxgAL5hGddam7jt0pL)"
        << R"(XW-ds60n3zBivFJFzhbJPqanmw6VaWtgwT7c5Z437A2xMBSdfEjSmZoZgIqgE08Pub)"
        << R"(n0afZZFptE4RnuSPhqqIfusuVb4uIzr5qdZUCG2qUAN4osM6raMr5m5JF4fjvG7lCE)"
        << R"(FYhtfMW7qsCEmuaX2UNWeUlDEvSUHzupysWgBwIK8a-0AoUPh56Wm_m7N-Ppj3mqhk)"
        << R"(xY6qK4CcucML5VevRhUr8onQHmtp8Iwv5y2THnEWh6cSnFp9qSwexDPItdk8iwclw6)"
        << R"(SsmWpv3LpsuAlMHQhrfaqERBthyp-zdvr1)"
        << R"("})";
    // clang-format on

    INFO(manifest_json_strm.str());

    const JSON_Value* downloadJsonValue{ ADUC_Json_GetRoot(manifest_json_strm.str().c_str()) };

    CHECK(downloadJsonValue != nullptr);

    CHECK(ADUC_Json_ValidateManifest(downloadJsonValue));
}

TEST_CASE("ADUC_Json_GetRootAndValidateManifest: Invalid Signature Value")
{
    // Note: The signature has been edited to be invalid
    std::stringstream manifest_json_strm;
    // clang-format off
    manifest_json_strm
        << R"({)"
        << R"("updateManifest":)"
        << R"("{)"
        <<     R"(\"manifestVersion\":\"2.0\",)"
        <<     R"(\"updateId\":{)"
        <<         R"(\"provider\":\"adu\",)"
        <<         R"(\"name\":\"test\",)"
        <<         R"(\"version\":\"2.0.0.0\")"
        <<     R"(},)"
        <<     R"(\"updateType\":\"SWUpdate\",)"
        <<     R"(\"installedCriteria\":\"1.2.3.4\",)"
        <<     R"(\"files\":{)"
        <<         R"(\"00000\":{)"
        <<             R"(\"fileName\":\"setup.exe\",)"
        <<             R"(\"sizeInBytes\":76,)"
        <<             R"(\"hashes\":{)"
        <<                 R"(\"sha256\":\"fSBQtjHq+MYBf3zLcISd/7rLi9lWu/khv/2yAhzZkxU=\")"
        <<             R"(})"
        <<         R"(})"
        <<     R"(},)"
        <<     R"(\"createdDateTime\":\"2020-08-25T23:37:21.3102772\")"
        << R"(}")"
        << R"(,)"
        << R"("updateManifestSignature":")"
        << R"(eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9pSlNVekkxTmlJc0ltdHBaQ0)"
        << R"(k2SWtGRVZTNHlNREEzTURJdVVpSjkuZXlKcmRIa2lPaUpTVTBFaUxDSnVJam9pY2to)"
        << R"(V1FrVkdTMUl4ZG5Ob1p5dEJhRWxuTDFORVVVOHplRFJyYWpORFZWUTNaa2R1U21oQm)"
        << R"(JYVkVhSFpJWm1velowaDZhVEJVTWtsQmNVTXhlREpDUTFka1QyODFkamgwZFcxeFVt)"
        << R"(b3ZibGx3WnprM2FtcFFRMHQxWTJSUE5tMHpOMlJqVDIxaE5EWm9OMDh3YTBod2Qwd3)"
        << R"(pibFZJUjBWeVNqVkVRUzloY0ZsdWQwVmxjMlY0VkdwVU9GTndMeXRpVkhGWFJXMTZa)"
        << R"(MFF6TjNCbVpFdGhjV3AwU0V4SFZtbFpkMVpJVUhwMFFtRmlkM2RxYUVGMmVubFNXUz)"
        << R"(k1T1U5bWJYcEVabGh0Y2xreGNtOHZLekpvUlhGRmVXdDFhbmRSUlZscmFHcEtZU3RD)"
        << R"(TkRjMkt6QnRkVWQ1VjBrMVpVbDJMMjlzZERKU1pWaDRUV0k1VFd4c1dFNTViMUF6WV)"
        << R"(U1TFNVcHBZbHBOY3pkMVMyTnBkMnQ1YVZWSllWbGpUV3B6T1drdlVrVjVLMnhOT1ha)"
        << R"(SlduRnlabkJEVlZoMU0zUnVNVXRuWXpKUmN5OVVaRGgwVGxSRFIxWTJkM1JXWVhGcF)"
        << R"(NYQlVaRlEwVW5KRFpFMXZUelZUVG1WbVprUjVZekpzUXpkMU9EVXJiMjFVYTJOcVVH)"
        << R"(cHRObVpoY0dSSmVVWXljV1Z0ZGxOQ1JHWkNOMk5oYWpWRVNVa3lOVmQzTlVWS1kyRj)"
        << R"(JabmxRTlRSdGNVNVJVVE5IWTAxUllqSmtaMmhwWTJ4d2FsbHZLelF6V21kWlEyUkhk)"
        << R"(R0ZhWkRKRlpreGFkMGd6VVdjeWNrUnNabXN2YVdFd0x6RjVjV2xyTDFoYU1XNXpXbF)"
        << R"(JwTUVKak5VTndUMDFGY1daT1NrWlJhek5DVjI5Qk1EVnlRMW9pTENKbElqb2lRVkZC)"
        << R"(UWlJc0ltRnNaeUk2SWxKVE1qVTJJaXdpYTJsa0lqb2lRVVJWTGpJd01EY3dNaTVTTG)"
        << R"(xNaWZRLmlTVGdBRUJYc2Q3QUFOa1FNa2FHLUZBVjZRT0dVRXV4dUhnMllmU3VXaHRZ)"
        << R"(WHFicE0takk1UlZMS2VzU0xDZWhLLWxSQzl4Ni1fTGV5eE5oMURPRmMtRmE2b0NFR3)"
        << R"(dVajh6aU9GX0FUNnM2RU9tY2txUHJ4dXZDV3R5WWtrRFJGNzRkdGFLMWpOQTdTZFhy)"
        << R"(Wnp2V0NzTXFPVU1OejBnQ29WUjBDczEyNTRrRk1SbVJQVmZFY2pnVDdqNGxDcHlEdV)"
        << R"(dncjlTZW5TZXFnS0xZeGphYUcwc1JoOWNkaTJkS3J3Z2FOYXFBYkhtQ3JyaHhTUENU)"
        << R"(QnpXTUV4WnJMWXp1ZEVvZnlZSGlWVlJoU0pwajBPUTE4ZWN1NERQWFYxVGN0MXkzaz)"
        << R"(dMTGlvN244aXpLdXEybTNUeEY5dlBkcWI5TlA2U2M5LW15YXB0cGJGcEhlRmtVTC1G)"
        << R"(NXl0bF9VQkZLcHdOOUNMNHdwNnlaLWpkWE5hZ3JtVV9xTDFDeVh3MW9tTkNnVG1KRj)"
        << R"(NHZDNseXFLSEhEZXJEcy1NUnBtS2p3U3dwWkNRSkdEUmNSb3ZXeUwxMnZqdzNMQkpN)"
        << R"(aG1VeHNFZEJhWlA1d0dkc2ZEOGxkS1lGVkZFY1owb3JNTnJVa1NNQWw2cEl4dGVmRV)"
        << R"(hpeTVscW1pUHpxX0xKMWVSSXJxWTBfIn0.eyJzaGEyNTYiOiI3Mk9BRTJmME5iVDAr)"
        << R"(VEw5MzdvNzB4bzhvTzk2Z21WTFlESnB4WEh6ZVhFPSJ9.asdxe9ylLitBHD14QsqSC)"
        << R"(O1lhrsrqqMdJo73at50-C3B2OVu6n5uiQ-6AOnuwEY07cRtxLcUli92HiLFy-itD57)"
        << R"(amI8ovIRuonLsJqcplmw6imdxDWD3CCkV_I3LfUBqjuaBew71Q2HrddHn3KVTFp562)"
        << R"(xMYgFZmWiERnz7c-q4IuH_7AqvNm8leznVrCscAs5UquHqz3oHLU9xEn-Sur1aP0xl)"
        << R"(bN-USD9WET5wXLpiu9ECZ86CFTpc_i3zlEKpl8Vbvsb0NHW_932Lrye6nz3TsYQNFx)"
        << R"(Mcn5EIvHZoxIs_yHEtkJFyjFnktojrxFxGKZ5nFH-CrQH6VIwSSIH1FkJOIJiI8Qto)"
        << R"(vzlqdDkZNLMYQ3uM1yKt3anXTpwHbuBrpYKQXN4T7bWN_9PWxyhnzKIDi6BulyrD8-)"
        << R"(H8X7P_S7WBoFigb-nNrMFoSEm0qgAND01B0xJmsKf4Q6eB6L7k1S0bJPx5DwrPVW-9)"
        << R"(TK8GXM0VjZYZGtiLCPUTa6SVRKTey)"
        << R"("})";
    // clang-format on

    INFO(manifest_json_strm.str());

    const JSON_Value* downloadJsonValue{ ADUC_Json_GetRoot(manifest_json_strm.str().c_str()) };

    CHECK(downloadJsonValue != nullptr);

    CHECK(!ADUC_Json_ValidateManifest(downloadJsonValue));
}
TEST_CASE("ADUC_Json_GetRootAndValidateManifest: Changed updateManifest ")
{
    // Note: Testing that the expected hash within the updateManifestSignature does not
    // equal the one calculated from the updateManifest

    std::stringstream manifest_json_strm;
    // clang-format off
    manifest_json_strm
        << R"({)"
        << R"("updateManifest":)"
        << R"("{)"
        <<     R"(\"manifestVersion\":\"2.0\",)"
        <<     R"(\"updateId\":{)"
        <<         R"(\"provider\":\"adu\",)"
        <<         R"(\"name\":\"test\",)"
        <<         R"(\"version\":\"2.0.0.0\")"
        <<     R"(},)"
        <<     R"(\"updateType\":\"SWUpdate\",)"
        <<     R"(\"installedCriteria\":\"1.2.3.4\",)"
        <<     R"(\"files\":{)"
        <<         R"(\"00000\":{)"
        <<             R"(\"fileName\":\"foo.exe\",)"
        <<             R"(\"sizeInBytes\":76,)"
        <<             R"(\"hashes\":{)"
        <<                 R"(\"sha256\":\"aGc0x9DB3CFgRwPYgX2kQsN0oUsS4/Zn4kP//f3QyLc=\")"
        <<             R"(})"
        <<         R"(})"
        <<     R"(},)"
        <<     R"(\"createdDateTime\":\"2020-10-02T22:15:56.8012002Z\")"
        << R"(}")"
        << R"(,)"
        << R"("updateManifestSignature":")"
        << R"(eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9pSlNVekkxTmlJc0ltdHBaQ0)"
        << R"(k2SWtGRVZTNHlNREEzTURJdVVpNVVJbjAuZXlKcmRIa2lPaUpTVTBFaUxDSnVJam9p)"
        << R"(ZGtwYWJGRjFjM1J3T1RsUFRpOXJXWEJ4TlVveFRtdG1hMGxDTjNrdldVbzJNbWRrVj)"
        << R"(JWRFlteE5XWGt3WjNRd1VVVjZXVGR4TjI1UFJHMDJTbXQxYldoTVpuTTNaa1JCVTA4)"
        << R"(elJIaG5SVlE0Y1dkWmQyNVdhMFpsTVVkVVowZEVXR28xVTFGQ2VIQkxVRWxQTlVRcl)"
        << R"(Z6TXdSSEpKTmxGSk4xRm1hVFpRWmxaT2FsVlVTRkI2U0VaNWFFMVRUVnByYWxreGVF)"
        << R"(eFBNMDFSWjBobmRXUjFNV1pzU0hGREszUmlhbms1WTI0emVtcDBNM2hUVWpKbU9URk)"
        << R"(RVa0p5VWxRelJ6Vm1WbFkxVjA1U1EwOWlWVU51YUdNeWFHaHlSRU15V0dreFFWaHli)"
        << R"(azlRZDNWRFJFbEZkSFIzYjBzd1QyNVFjV3BLVHk5SVVqRjJMelJPY3l0S1YwOU1VRT)"
        << R"(U0ZW1aRlNqSnBXVTlSWkdwbFpteEJPRVoyVFhWMGVYVndaVXRwUTJsclYzY3lOWFpw)"
        << R"(YjAxUldWSXdiMVozUjFwMU16aG1PSFJvTVVoRlZFSjJkVVZWTDFkeGVIQllXbVJQUm)"
        << R"(5wa09VTkhabVZsTW5FMGIzcFFNQzlQVUUxRWQxQlRNRzVZWm0xYVVHUlhhWHBwVDNW)"
        << R"(RlUwSndVRko1TjFwcFJYSjVSbVpNWjJnemNVWkVXQ3RYZDB3eFdXWmxaRzU1V1N0cF)"
        << R"(VWWjNiaXRpVTNWalVHeFRlRXdyUmxJNVNuZFFaRGhNZUU1bmFHUnZZbTFCTjA1MmNX)"
        << R"(SkNaakp6Wld0dE1HUkpTa0pRTm5wc2MxQmxURU5ZVW1VMU1IbE9SVk5rYUhWRlFuVn)"
        << R"(hNV1k0Y1ZZek1VVklSVmx6V0dRMGMwWm5WMVZ2Wm5kM1dIaFBTM0FpTENKbElqb2lR)"
        << R"(VkZCUWlJc0ltRnNaeUk2SWxKVE1qVTJJaXdpYTJsa0lqb2lRVVJWTGpJd01EY3dNaT)"
        << R"(VTTGxOVUluMC5RS0ktckVJSXBIZTRPZVd1dkN4cFNYam1wbC05czNlN1d2ZGRPcFNn)"
        << R"(R2dJaUN5T1hPYThjSFdlbnpzSFVMTUZBZEF0WVBnM2ZVTnl3QUNJOEI2U044UGhUc3)"
        << R"(E5TFg4QjR2c0l2QlZ2S2FrTU1ucjRuZXRlVHN5V0NJbEhBeG4ydThzSlQ3amVzMmtF)"
        << R"(Q2ZrNnpBeS16cElSSXhYaF8yVEl5Y2RNLVU0UmJYVDhwSGpPUTVfZTRnSFRCZjRlOD)"
        << R"(RaWTdsUEw0RGtTa3haR1dnaERhOTFGVjlXbjRtc050eDdhMEJ3NEtTMURVd1c2eXE0)"
        << R"(c2diTDFncVl6VG04bENaSDJOb1pDdjZZUUd4YVE5UkJ1ek5Eb0VMb0RXXzB4S3V1Vz)"
        << R"(Faa3UxSDN6c3lZNlNJMVp0bkFkdDAxcmxIck5fTTlKdTBZM3EyNzJiRWxWSnVnN3F6)"
        << R"(cGF5WXFucllOMTBHRWhIUW9KTkRxZnlPSHNLRG9FbTNOR1pKY2FSWVl4eHY2SVZxTF)"
        << R"(JDaFZidnJ3ekdkTXdmME95aE52THFGWlhMS1RWMmhPX2t6MXZWaVNwRi1PVXVWbW5u)"
        << R"(RC1ZMXFQb28xSTBJeVFYallxaGNlME01UkhiQ1drVk1Qbk5jZHFMQzZlTWhiTjJGT1)"
        << R"(pHSUxSOVRqcDIyMmxucGFTbnVmMHlVbDF4MiJ9.eyJzaGEyNTYiOiJjTGRjZnBBRnN)"
        << R"(yYjA3ak5zVVFzQ0hDZDBmb0J3RU5remQybTB1UWZMdGc4PSJ9.ghH76UBj3uPgpfYP)"
        << R"(3Px24QUB0ZmAu9BthyL90CwkAb7TRM6yjCMxU7V7P2YERa_ATnjKygTefSzxpzc1c7)"
        << R"(B86X7DyLNInp3TvRktPf6PWGMbZ_O2l7XeUNfhhJwUTsFok405AK6rWBSt49MpnKbZ)"
        << R"(yEss-dVEFddun1Zr71InV2gA_BtdhkP3qS7y16zp3SpLi1eqyxgAL5hGddam7jt0pL)"
        << R"(XW-ds60n3zBivFJFzhbJPqanmw6VaWtgwT7c5Z437A2xMBSdfEjSmZoZgIqgE08Pub)"
        << R"(n0afZZFptE4RnuSPhqqIfusuVb4uIzr5qdZUCG2qUAN4osM6raMr5m5JF4fjvG7lCE)"
        << R"(FYhtfMW7qsCEmuaX2UNWeUlDEvSUHzupysWgBwIK8a-0AoUPh56Wm_m7N-Ppj3mqhk)"
        << R"(xY6qK4CcucML5VevRhUr8onQHmtp8Iwv5y2THnEWh6cSnFp9qSwexDPItdk8iwclw6)"
        << R"(SsmWpv3LpsuAlMHQhrfaqERBthyp-zdvr1)"
        << R"("})";
    // clang-format on

    INFO(manifest_json_strm.str());

    const JSON_Value* downloadJsonValue{ ADUC_Json_GetRoot(manifest_json_strm.str().c_str()) };

    CHECK(downloadJsonValue != nullptr);

    CHECK(!ADUC_Json_ValidateManifest(downloadJsonValue));
}

TEST_CASE("ADUC_Json_GetRootAndValidateManifest: Empty Update Manifest")
{
    std::stringstream manifest_json_strm;
    // clang-format off
    manifest_json_strm
        << R"({)"
        << R"("updateManifest":"{)"
        << R"(}",)"
        << R"("fileUrls":{)"
        << R"("00000":"http://setup.exe")"
        << R"(},)"
        << R"("updateManifestSignature": "")"
        << R"(})";
    // clang-format on
    INFO(manifest_json_strm.str());

    const JSON_Value* downloadJsonValue{ ADUC_Json_GetRoot(manifest_json_strm.str().c_str()) };

    CHECK(downloadJsonValue != nullptr);

    CHECK(!ADUC_Json_ValidateManifest(downloadJsonValue));
}

TEST_CASE("ADUC_Json_GetUpdateAction: Valid")
{
    // clang-format off
    auto updateactionVal = GENERATE( // NOLINT(google-build-using-namespace)
        ADUCITF_UpdateAction_Cancel,
        ADUCITF_UpdateAction_ProcessDeployment);
    // clang-format on

    SECTION("Verify update action values")
    {
        unsigned updateAction;
        std::stringstream strm;
        strm << R"({ "workflow" : {"action": )" << updateactionVal << R"( } })";
        INFO(strm.str())
        const JSON_Value* updateActionJson{ ADUC_Json_GetRoot(strm.str().c_str()) };
        REQUIRE(updateActionJson != nullptr);
        CHECK(ADUC_Json_GetUpdateAction(updateActionJson, &updateAction));
        CHECK(updateAction == updateactionVal);
    }
}

TEST_CASE("ADUC_Json_GetUpdateAction: Invalid")
{
    unsigned updateAction;
    std::stringstream strm;
    const JSON_Value* updateActionJsonValue{ ADUC_Json_GetRoot(R"({ "workflow" : { "action": "foo" }})") };
    INFO(strm.str())
    REQUIRE(updateActionJsonValue != nullptr);
    CHECK(!ADUC_Json_GetUpdateAction(updateActionJsonValue, &updateAction));
}

TEST_CASE("ADUC_Json_GetExpectedUpdateId: Valid")
{
    std::string provider{ "example-provider" };
    std::string name{ "example-name" };
    std::string version{ "1.2.3.4" };

    std::stringstream expectedUpdateIdJsonStrm;

    // clang-format off
    expectedUpdateIdJsonStrm
        << R"({)"
        <<     R"("workflow" : { )"
        <<     R"(  "action": 0  )"
        <<     R"( },)"
        <<     R"("updateManifest":"{)"
        <<         R"(\"updateId\":{)"
        <<             R"(\"provider\":\")" << provider << R"(\",)"
        <<             R"(\"name\":\")" << name << R"(\",)"
        <<             R"(\"version\":\")" << version << R"(\")"
        <<         R"(})"
        <<     R"(}")"
        << R"(})";
    // clang-format on

    const JSON_Value* expectedUpdateIdJsonValue{ ADUC_Json_GetRoot(expectedUpdateIdJsonStrm.str().c_str()) };
    REQUIRE(expectedUpdateIdJsonValue != nullptr);

    ADUC_UpdateId* expectedUpdateId = nullptr;
    CHECK(ADUC_Json_GetUpdateId(expectedUpdateIdJsonValue, &expectedUpdateId));

    CHECK_THAT(expectedUpdateId->Provider, Equals(provider));
    CHECK_THAT(expectedUpdateId->Name, Equals(name));
    CHECK_THAT(expectedUpdateId->Version, Equals(version));
}

TEST_CASE("ADUC_Json_GetExpectedUpdateId: Invalid")
{
    SECTION("Missing Update ID returns nullptr")
    {
        std::stringstream expectedUpdateIdJsonStrm;

        // clang-format off
        expectedUpdateIdJsonStrm
            << R"({)"
            <<     R"("workflow" : { )"
            <<     R"(  "action": 0  )"
            <<     R"( },)"
            <<     R"("updateManifest":"{)"
            <<     R"(}")"
            << R"(})";
        // clang-format on

        const JSON_Value* expectedUpdateIdJsonValue{ ADUC_Json_GetRoot(expectedUpdateIdJsonStrm.str().c_str()) };
        REQUIRE(expectedUpdateIdJsonValue != nullptr);
        ADUC_UpdateId* expectedUpdateId = nullptr;

        CHECK(!ADUC_Json_GetUpdateId(expectedUpdateIdJsonValue, &expectedUpdateId));
    }

    SECTION("Invalid Update ID returns nullptr")
    {
        std::stringstream expectedUpdateIdJsonStrm;

        // clang-format off
        expectedUpdateIdJsonStrm
            << R"({)"
            <<     R"("workflow" : { )"
            <<     R"(  "action": 0  )"
            <<     R"( },)"
            <<     R"("updateManifest":"{)"
            <<        R"(\"updateId\":[\"a\"])"
            <<     R"(}")"
            << R"(})";
        // clang-format on

        const JSON_Value* expectedUpdateIdJsonValue{ ADUC_Json_GetRoot(expectedUpdateIdJsonStrm.str().c_str()) };
        REQUIRE(expectedUpdateIdJsonValue != nullptr);
        ADUC_UpdateId* expectedUpdateId = nullptr;
        CHECK(!ADUC_Json_GetUpdateId(expectedUpdateIdJsonValue, &expectedUpdateId));
    }
}

TEST_CASE("ADUC_Json_GetInstalledCriteria: Valid")
{
    // clang-format off
    std::string installedCriteriaJson{
        R"({)"
            R"("action": 0,)"
            R"("updateManifest": "{)"
                R"(\"installedCriteria\":\"1.2.3.4\")"
            R"(}")"
        R"(})"
    };
    // clang-format on

    const JSON_Value* installedCriteriaJsonValue{ ADUC_Json_GetRoot(installedCriteriaJson.c_str()) };
    REQUIRE(installedCriteriaJsonValue != nullptr);
    cstr_wrapper installedCriteria;
    CHECK(ADUC_Json_GetInstalledCriteria(installedCriteriaJsonValue, installedCriteria.address_of()));
    CHECK_THAT(installedCriteria.get(), Equals("1.2.3.4"));
}

TEST_CASE("ADUC_Json_GetInstalledCriteria: Invalid")
{
    SECTION("Missing installed criteria returns nullptr")
    {
        // clang-format off
        std::string installedCriteriaJson{
            R"({)"
                R"("action": 0,)"
                R"("updateManifest": "{)"
                R"(}")"
            R"(})"
        };
        // clang-format on

        const JSON_Value* installedCriteriaJsonValue{ ADUC_Json_GetRoot(installedCriteriaJson.c_str()) };
        REQUIRE(installedCriteriaJsonValue != nullptr);
        cstr_wrapper installedCriteria;
        CHECK(!ADUC_Json_GetInstalledCriteria(installedCriteriaJsonValue, installedCriteria.address_of()));
    }

    SECTION("Invalid installed criteria returns nullptr")
    {
        // clang-format off
        std::string installedCriteriaJson{
            R"({)"
                R"("action": 0,)"
                R"("updateManifest": "{)"
                    R"(\"installedCriteria\":\"[\"a\"]\")"
                R"(}")"
            R"(})"
        };
        // clang-format on

        const JSON_Value* installedCriteriaJsonValue{ ADUC_Json_GetRoot(installedCriteriaJson.c_str()) };
        REQUIRE(installedCriteriaJsonValue != nullptr);
        cstr_wrapper installedCriteria;
        CHECK(!ADUC_Json_GetInstalledCriteria(installedCriteriaJsonValue, installedCriteria.address_of()));
    }
}

TEST_CASE("ADUC_Json_GetFiles: Valid")
{
    // clang-format off
    const std::map<std::string, std::string> files{
        {"w2cy42AR2pOR6uqQ7367IdIj+AnaCArEsSlNHjosQJY=", "http://file1" },
        {"x2cy42AR2pOR6uqQ7367IdIj+AnaCArEsSlNHjosQJY=", "http://file2" }
    };

    // clang-format on

    std::stringstream main_json_strm;

    std::stringstream url_json_strm;

    main_json_strm
        << R"({"Action":0, "updateManifest":"{\"updateId\":{\"provider\": \"Azure\",\"name\": \"IOT-Firmware\",\"version\": \"1.2.0.0\"},\"files\":{)";
    url_json_strm << R"("fileUrls": {)";

    int i = 0;
    for (auto it = files.begin(); it != files.end(); ++it)
    {
        main_json_strm << R"(\")" << i << R"(\":{)";
        main_json_strm << R"(\"fileName\":\"file)" << i << R"(\", \"hashes\": {\"sha256\": \")";
        main_json_strm << it->first << R"(\"}})";

        url_json_strm << R"(")" << i << R"(":")" << it->second << R"(")";

        if (it != (--files.end()))
        {
            main_json_strm << ",";
            url_json_strm << ",";
        }

        ++i;
    }

    url_json_strm << R"(})";
    main_json_strm << R"(}}",)" << url_json_strm.str() << "}";

    INFO(main_json_strm.str());

    const JSON_Value* downloadJsonValue{ ADUC_Json_GetRoot(main_json_strm.str().c_str()) };
    REQUIRE(downloadJsonValue != nullptr);

    unsigned int fileCount;
    ADUC_FileEntity* fileEntities;
    CHECK(ADUC_Json_GetFiles(downloadJsonValue, &fileCount, &fileEntities));
    REQUIRE(fileCount == 2);

    for (unsigned int i = 0; i < fileCount; ++i)
    {
        std::stringstream strm;
        strm << "file" << i;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        CHECK_THAT(fileEntities[i].TargetFilename, Equals(strm.str()));
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        CHECK_THAT(
            fileEntities[i].DownloadUri,
            Equals(
                files.find(ADUC_HashUtils_GetHashValue(fileEntities[i].Hash, fileEntities[i].HashCount, 0))->second));
    }

    ADUC_FileEntityArray_Free(fileCount, fileEntities);
}

TEST_CASE("ADUC_Json_GetFiles: Invalid")
{
    SECTION("Files missing returns nullptr")
    {
        const JSON_Value* downloadJsonValue{ ADUC_Json_GetRoot(R"({ "action": 0,"TargetVersion": "1.2.3.4" })") };
        REQUIRE(downloadJsonValue != nullptr);

        unsigned int fileCount;
        ADUC_FileEntity* fileEntities;
        CHECK(!ADUC_Json_GetFiles(downloadJsonValue, &fileCount, &fileEntities));
    }

    SECTION("Empty files returns nullptr")
    {
        const JSON_Value* downloadJsonValue{
            ADUC_Json_GetRoot(
                R"({ "action": 0,"TargetVersion": "1.2.3.4", "updateManifest":"{ \"files\": {} }", "fileUrls": { "001":"https://file1"} })")
        };
        REQUIRE(downloadJsonValue != nullptr);

        unsigned int fileCount;
        ADUC_FileEntity* fileEntities;
        CHECK(!ADUC_Json_GetFiles(downloadJsonValue, &fileCount, &fileEntities));
    }

    SECTION("Empty fileURLs returns nullptr")
    {
        const JSON_Value* downloadJsonValue{
            ADUC_Json_GetRoot(
                R"({ "action": 0,"TargetVersion": "1.2.3.4", "updateManifest":"{ \"files\": {\"001\":{\"fileName\":\"file1\", \"hashes\":{\"sha256\":\"w2cy42AR2pOR6uqQ7367IdIj+AnaCArEsSlNHjosQJY=\"}}}", "fileUrls": {} })")
        };
        REQUIRE(downloadJsonValue != nullptr);

        unsigned int fileCount;
        ADUC_FileEntity* fileEntities;
        CHECK(!ADUC_Json_GetFiles(downloadJsonValue, &fileCount, &fileEntities));
    }
}
