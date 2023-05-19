#ifndef COM_HELPERS_HPP
#define COM_HELPERS_HPP

#define WIN32_LEAN_AND_MEAN
#include <objbase.h>
#include <oleauto.h>

#undef IID_PPV_ARGS
#define IID_PPV_ARGS(ppType) \
    __uuidof(**(ppType)), (static_cast<IUnknown*>(*(ppType)), reinterpret_cast<void**>(ppType))

class CCoInitialize
{
    CCoInitialize(const CCoInitialize&) = delete;
    CCoInitialize& operator=(const CCoInitialize&) = delete;

public:
    CCoInitialize()
    {
        m_hr = CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);
    }

    ~CCoInitialize()
    {
        if (SUCCEEDED(m_hr))
        {
            CoUninitialize();
        }
    }

private:
    HRESULT m_hr;
};
template<class T>
class CComPtr
{
    CComPtr(const CComPtr&) = delete;
    CComPtr& operator=(const CComPtr&) = delete;

public:
    CComPtr()
    {
    }

    ~CComPtr()
    {
        Release();
    }

    operator T*() const
    {
        return m_p;
    }

    T* operator->()
    {
        return m_p;
    }

    T** operator&()
    {
        Release();

        return &m_p;
    }

    void Release()
    {
        if (m_p != nullptr)
        {
            m_p->Release();
            m_p = nullptr;
        }
    }

private:
    T* m_p = nullptr;
};

class CComBSTR
{
    CComBSTR(const CComBSTR&) = delete;
    CComBSTR& operator=(const CComBSTR&) = delete;

public:
    CComBSTR(const OLECHAR* str)
    {
        if (str != NULL)
        {
            m_bstr = SysAllocString(str);
        }
    }

    CComBSTR(const char* str)
    {
        if (str != NULL)
        {
            const DWORD wsize = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
            m_bstr = SysAllocStringLen(NULL, wsize);
            MultiByteToWideChar(CP_UTF8, 0, str, -1, m_bstr, wsize);
        }
    }

    ~CComBSTR()
    {
        SysFreeString(m_bstr);
    }

    operator BSTR()
    {
        return m_bstr;
    }

private:
    BSTR m_bstr = nullptr;
};

class CComVariant
{
    CComVariant(const CComVariant&) = delete;
    CComVariant& operator=(const CComVariant&) = delete;

public:
    CComVariant()
    {
        VariantInit(&m_var);
    }

    CComVariant(const OLECHAR* str)
    {
        Clear();

        m_var.vt = VT_BSTR;
        m_var.bstrVal = SysAllocString(str);
    }

    CComVariant(bool val) throw()
    {
        Clear();

        m_var.vt = VT_BOOL;
        m_var.boolVal = val ? static_cast<VARIANT_BOOL>(-1) : static_cast<VARIANT_BOOL>(0);
    }

    CComVariant(long val) throw()
    {
        Clear();

        m_var.vt = VT_I4;
        m_var.lVal = val;
    }

    ~CComVariant()
    {
        Clear();
    }

    HRESULT Clear()
    {
        return VariantClear(&m_var);
    }

    VARIANT* operator&()
    {
        return &m_var;
    }

    const BSTR bstrVal() const
    {
        return m_var.bstrVal;
    }

    const INT intVal() const
    {
        return m_var.intVal;
    }

    SAFEARRAY* parray() const
    {
        return m_var.parray;
    }

private:
    VARIANT m_var;
};

#endif
