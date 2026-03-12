#include <aduc/exception_utils.hpp>
#include <catch2/catch_all.hpp>

#include <functional>
#include <stdexcept>
#include <system_error>

TEST_CASE("CallVoidMethodAndHandleExceptions swallows all exception types")
{
    std::function<void()> callback;

    SECTION("success callback executes")
    {
        bool executed = false;
        callback = [&executed]() {
            executed = true;
        };

        CHECK_NOTHROW(ADUC::ExceptionUtils::CallVoidMethodAndHandleExceptions(callback));
        CHECK(executed);
    }

    SECTION("ADUC exception is handled")
    {
        callback = []() {
            ADUC::Exception::ThrowAducResult(0x12345, "aduc exception");
        };

        CHECK_NOTHROW(ADUC::ExceptionUtils::CallVoidMethodAndHandleExceptions(callback));
    }

    SECTION("std::exception is handled")
    {
        callback = []() {
            throw std::runtime_error("std exception");
        };

        CHECK_NOTHROW(ADUC::ExceptionUtils::CallVoidMethodAndHandleExceptions(callback));
    }

    SECTION("unknown exception is handled")
    {
        callback = []() {
            throw 42;
        };

        CHECK_NOTHROW(ADUC::ExceptionUtils::CallVoidMethodAndHandleExceptions(callback));
    }
}

TEST_CASE("CallResultMethodAndHandleExceptions returns mapped results")
{
    constexpr ADUC_Result_t failureCode = ADUC_GeneralResult_Failure;
    std::function<ADUC_Result()> callback;

    SECTION("success callback result is returned")
    {
        ADUC_Result expected{ ADUC_GeneralResult_Success, 0xABC };
        callback = [expected]() {
            return expected;
        };

        ADUC_Result result = ADUC::ExceptionUtils::CallResultMethodAndHandleExceptions(failureCode, callback);

        CHECK(result.ResultCode == expected.ResultCode);
        CHECK(result.ExtendedResultCode == expected.ExtendedResultCode);
    }

    SECTION("ADUC exception returns callback failure and original code")
    {
        callback = []() {
            ADUC::Exception::ThrowAducResult(0x223344, "aduc exception");
            return ADUC_Result{ ADUC_GeneralResult_Success, 0 };
        };

        ADUC_Result result = ADUC::ExceptionUtils::CallResultMethodAndHandleExceptions(failureCode, callback);

        CHECK(result.ResultCode == failureCode);
        CHECK(result.ExtendedResultCode == 0x223344);
    }

    SECTION("std exception returns not recoverable")
    {
        callback = []() {
            throw std::runtime_error("std exception");
            return ADUC_Result{ ADUC_GeneralResult_Success, 0 };
        };

        ADUC_Result result = ADUC::ExceptionUtils::CallResultMethodAndHandleExceptions(failureCode, callback);

        CHECK(result.ResultCode == failureCode);
        CHECK(result.ExtendedResultCode == ADUC_ERC_NOTRECOVERABLE);
    }

    SECTION("unknown exception returns not recoverable")
    {
        callback = []() {
            throw 7;
            return ADUC_Result{ ADUC_GeneralResult_Success, 0 };
        };

        ADUC_Result result = ADUC::ExceptionUtils::CallResultMethodAndHandleExceptions(failureCode, callback);

        CHECK(result.ResultCode == failureCode);
        CHECK(result.ExtendedResultCode == ADUC_ERC_NOTRECOVERABLE);
    }
}

TEST_CASE("ADUC Exception factory methods preserve code and message")
{
    SECTION("ThrowAducResult populates ADUC exception")
    {
        try
        {
            ADUC::Exception::ThrowAducResult(0x42, "adu factory");
            FAIL("Expected ADUC::Exception");
        }
        catch (const ADUC::Exception& e)
        {
            CHECK(e.Code() == 0x42);
            CHECK(e.Message() == "adu factory");
            CHECK(std::string(e.what()) == "ADU Agent Exception");
        }
    }

    SECTION("ThrowErrno maps errno to ADUC extended result")
    {
        try
        {
            ADUC::Exception::ThrowErrno(EINVAL, "errno factory");
            FAIL("Expected ADUC::Exception");
        }
        catch (const ADUC::Exception& e)
        {
            CHECK(e.Code() == MAKE_ADUC_EXTENDEDRESULTCODE_FOR_COMPONENT_ERRNO(EINVAL));
            CHECK(e.Message() == "errno factory");
        }
    }

    SECTION("ThrowErrc maps std::errc to ADUC extended result")
    {
        try
        {
            ADUC::Exception::ThrowErrc(std::errc::invalid_argument, "errc factory");
            FAIL("Expected ADUC::Exception");
        }
        catch (const ADUC::Exception& e)
        {
            CHECK(e.Code() == MAKE_ADUC_EXTENDEDRESULTCODE_FOR_COMPONENT_ERRNO(EINVAL));
            CHECK(e.Message() == "errc factory");
        }
    }
}
