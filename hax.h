#pragma once

#include <crtdbg.h>
#include <type_traits>
#include <cmath>

#ifndef HAX_ASSERT
#define HAX_ASSERT(expr) _ASSERTE(expr)
#endif

namespace Hax
{
    using int64 = int64_t;
    using uint64 = uint64_t;
    using uint32 = uint32_t;
    using uint8 = uint8_t;
    using char16 = wchar_t;
    using uintptr = uintptr_t;

    constexpr size_t kSizeMax = (size_t)-1;
    constexpr float kPi = 3.14159265358979323846f;

    template <typename T> concept TriCo = std::is_trivially_copyable_v<T>;
    template <typename T> concept CharT = std::is_same_v<T, char> || std::is_same_v<T, wchar_t>;

    template <CharT T> constexpr size_t StrLen(const T* s) noexcept { if (!s) return 0; size_t len = 0; while (s[len] != 0) len++; return len; }

    template <TriCo T> constexpr T Max(T a, T b) noexcept { return (a < b) ? b : a; }
    template <TriCo T> constexpr T Min(T a, T b) noexcept { return (b < a) ? b : a; }
    template <TriCo T> constexpr T Clamp(T v, T lo, T hi) noexcept { return (v < lo) ? lo : (hi < v) ? hi : v; }
    template <TriCo T> constexpr T Lerp(T a, T b, float t) noexcept { return static_cast<T>(a + t * (b - a)); }
    template <TriCo T> constexpr T Abs(T v) noexcept { return v < 0 ? -v : v; }
    template <TriCo T> constexpr void Swap(T& a, T& b) noexcept { T temp = a; a = b; b = temp; }

    inline float Sqrt(float v) noexcept { HAX_ASSERT(v >= 0.f); return ::sqrtf(v); }
    inline float Floor(float v) noexcept { return ::floorf(v); }
    inline float Sin(float radians) noexcept { return ::sinf(radians); }
    inline float Cos(float radians) noexcept { return ::cosf(radians); }

    constexpr float Trunc(float v) noexcept { return v < 0.f ? -static_cast<float>(static_cast<int>(-v)) : static_cast<float>(static_cast<int>( v)); }
    constexpr float Mod(float x, float y) noexcept { if (y == 0.f) return 0.f; return x - Trunc(x / y) * y; }

    struct Allocator
    {
        Allocator() = default;
        Allocator(const char* name) : m_Name(name) {}

        [[nodiscard]] void*                         Alloc(size_t size);
        void                                        Free(void* ptr);

        template<typename T, typename... Args> T*   New(Args&&... args) { void* ptr = this->Alloc(sizeof(T)); return ptr ? new(ptr) T(std::forward<Args>(args)...) : nullptr; }
        template<typename T> void                   Delete(T* ptr)      { if (ptr) { ptr->~T(); Free(ptr); } }

        size_t                                      TotalUsage() const  { return m_TotalAllocated; }
        size_t                                      PeakUsage() const   { return m_MaxAllocated; }

        const char*                                 Name() const        { return m_Name; }

    private:
        const char*                                 m_Name = "Unnamed";
        size_t                                      m_TotalAllocated = 0;
        size_t                                      m_MaxAllocated = 0;
    };

    extern Allocator g_GlobalAlloc;
    extern Allocator g_StateAlloc;

    template <TriCo T, size_t N>
    struct Array
    {
        Array() = default;
        Array(const T& val) { for (size_t i = 0; i < N; ++i) m_Data[i] = val; }

        constexpr size_t    Size() const { return N; }

        T&                  operator[](size_t index)                { HAX_ASSERT(index < N); return m_Data[index]; }
        const T&            operator[](size_t index) const          { HAX_ASSERT(index < N); return m_Data[index]; }

        T*                  begin()                                 { return m_Data; }
        const T*            begin() const                           { return m_Data; }
        T*                  end()                                   { return m_Data + N; }
        const T*            end() const                             { return m_Data + N; }

        T                   m_Data[N];
    };

    template <TriCo T>
    struct Vector
    {
        constexpr Vector() = default;
        Vector(Allocator& alloc) : m_Alloc(&alloc)                  {}
        Vector(const Vector<T>& src)                                { m_Alloc = src.m_Alloc; operator=(src); }
        ~Vector()                                                   { ClearFree(); }

        Vector<T>&          operator=(const Vector<T>& src)         { if (&src == this) return *this; ClearFree(); Resize(src.m_Size); if (src.m_Data) memcpy(m_Data, src.m_Data, m_Size * sizeof(T)); return *this; }

        void                Clear()                                 { m_Size = 0; }
        void                ClearFree()                             { if (m_Data) { m_Size = m_Capacity = 0; m_Alloc->Free(m_Data); m_Data = nullptr; } }

        T*                  Data()                                  { return m_Data; }
        const T*            Data() const                            { return m_Data; }
        bool                Empty() const                           { return m_Size == 0; }
        size_t              Size() const                            { return m_Size; }
        size_t              Capacity() const                        { return m_Capacity; }
        
        T&                  operator[](size_t i)                    { HAX_ASSERT(i >= 0 && i < m_Size); return m_Data[i]; }
        const T&            operator[](size_t i) const              { HAX_ASSERT(i >= 0 && i < m_Size); return m_Data[i]; }

        T*                  begin()                                 { return m_Data; }
        const T*            begin() const                           { return m_Data; }
        T*                  end()                                   { return m_Data + m_Size; }
        const T*            end() const                             { return m_Data + m_Size; }

        T&                  First()                                 { HAX_ASSERT(m_Size > 0); return m_Data[0]; }
        const T&            First() const                           { HAX_ASSERT(m_Size > 0); return m_Data[0]; }
        T&                  Last()                                  { HAX_ASSERT(m_Size > 0); return m_Data[m_Size - 1]; }
        const T&            Last() const                            { HAX_ASSERT(m_Size > 0); return m_Data[m_Size - 1]; }

        void                EnsureCapacity(size_t desired);
        void                Resize(size_t newSize)                  { EnsureCapacity(newSize); m_Size = newSize; }
        void                Resize(size_t newSize, const T& v)      { EnsureCapacity(newSize); if (newSize > m_Size) for (size_t i = m_Size; i < newSize; ++i) memcpy(&m_Data[i], &v, sizeof(v)); m_Size = newSize; }
        void                Reserve(size_t newCapacity);

        T*                  Insert(const T* it, const T& v);
        T*                  Insert(size_t off, const T& v);
        T*                  Erase(const T* it);
        T*                  Erase(size_t off);
        T*                  Erase(const T* it, const T* itLast);    

        void                PushBack(const T& v)                    { EnsureCapacity(m_Size + 1); m_Data[m_Size++] = v; }
        void                PopBack()                               { HAX_ASSERT(m_Size > 0); m_Size--; }
        void                PushFront(const T& v)                   { if (m_Size == 0) PushBack(v); else Insert(m_Data, v); }

        bool                Contains(const T& v) const              { const T* data = m_Data; const T* dataEnd = m_Data + m_Size; while (data < dataEnd) if (*data++ == v) return true; return false; }
        T*                  Find(const T& v)                        { T* data = m_Data; const T* dataEnd = m_Data + m_Size; while (data < dataEnd) if (*data == v) break; else ++data; return data; }
        const T*            Find(const T& v) const                  { const T* data = m_Data; const T* dataEnd = m_Data + m_Size; while (data < dataEnd) if (*data == v) break; else ++data; return data; }
        size_t              FindIndex(const T& v) const             { const T* end = m_Data + m_Size; const T* it = Find(v); if (it == end) return (size_t)-1; return (size_t)(it - m_Data); }
        bool                FindErase(const T& v)                   { const T* it = Find(v); if (it < m_Data + m_Size) { Erase(it); return true; } return false; }

    private:
        Allocator*          m_Alloc = &g_GlobalAlloc;
        size_t              m_Size = 0;
        size_t              m_Capacity = 0;
        T*                  m_Data = nullptr;
    };

    template <TriCo K, TriCo V>
    struct Map
    {
        struct Entry
        {
            K key;
            V value;
        };

        Map() = default;
        Map(Allocator& alloc) : m_Entries(alloc)                    {}
        ~Map()                                                      { ClearFree(); }

        const V*            Get(const K& key) const;
        V*                  Get(const K& key)                       { return const_cast<V*>(static_cast<const Map*>(this)->Get(key)); }

        V&                  FindOrAdd(const K& key);

        void                Insert(const K& key, const V& value)    { FindOrAdd(key) = value; }
        bool                Erase(const K& key);

        bool                Contains(const K& key) const            { return Get(key) != nullptr; }
        void                Clear()                                 { m_Entries.Clear(); }
        void                ClearFree()                             { m_Entries.ClearFree(); }
        void                Reserve(size_t cap)                     { m_Entries.Reserve(cap); }
        size_t              Size() const                            { return m_Entries.Size(); }

        Entry*              begin()                                 { return m_Entries.begin(); }
        const Entry*        begin() const                           { return m_Entries.begin(); }
        Entry*              end()                                   { return m_Entries.end(); }
        const Entry*        end() const                             { return m_Entries.end(); }

    private:
        size_t              FindLowerBound(const K& key) const;

        Vector<Entry>       m_Entries;
    };

    template <TriCo T>
    struct Optional
    {
        constexpr           Optional() : m_Empty(0), m_HasValue(false) {}
        constexpr           Optional(const T& val) : m_Value(val), m_HasValue(true) {}
        constexpr           Optional(const Optional&) = default;

        constexpr Optional& operator=(const Optional&) = default;
        constexpr Optional& operator=(const T& val)                 { m_Value = val; m_HasValue = true; return *this; }

        constexpr void      Reset()                                 { m_HasValue = false; }

        constexpr bool      HasValue() const                        { return m_HasValue; }
        constexpr T&        Value()                                 { HAX_ASSERT(m_HasValue); return m_Value; }
        constexpr const T&  Value() const                           { HAX_ASSERT(m_HasValue); return m_Value; }

        constexpr T         ValueOr(const T& defaultValue) const    { return m_HasValue ? m_Value : defaultValue; }

        constexpr T*        operator->()                            { return &Value(); }
        constexpr T&        operator*()                             { return Value(); }

        constexpr bool      operator==(const T& val) const          { return m_HasValue && m_Value == val; }
        constexpr bool      operator==(const Optional& other) const { if (m_HasValue != other.m_HasValue) return false; return !m_HasValue || m_Value == other.m_Value; }

        constexpr explicit  operator bool() const { return m_HasValue; }

        union 
        {
            char m_Empty;
            T    m_Value;
        };
        bool     m_HasValue;
    };
    static_assert(std::is_trivially_copyable_v<Optional<int>>);

    struct String
    {
        constexpr String() = default;
        String(const wchar_t* str, Allocator& alloc) : m_Data(alloc){ operator=(str); }
        String(const wchar_t* str)                                  { operator=(str); }
        String(Allocator& alloc) : m_Data(alloc)                    {}
        String(const String& str) : m_Data(str.m_Data)              {}
        String(std::nullptr_t) = delete;
        ~String()                                                   { ClearFree(); }

        String&             operator=(const wchar_t* str);
        String&             operator=(const String& str)            { operator=(str.CStr()); }

        const wchar_t*      CStr()   const                          { return Empty() ? L"" : m_Data.Data(); }
        bool                Empty() const                           { return m_Data.Size() <= 1; }
        size_t              Length() const                          { size_t sz = m_Data.Size(); return sz > 0 ? sz - 1 : 0; }

        wchar_t&            First()                                 { return m_Data.First(); }
        wchar_t&            Last()                                  { return m_Data.Last(); }

        void                Clear()                                 { m_Data.Clear(); }
        void                ClearFree()                             { m_Data.ClearFree(); }
        void                Reserve(size_t n)                       { m_Data.Reserve(n); }
        wchar_t*            Data()                                  { return m_Data.Data(); }

        void                Append(const wchar_t* str);

        bool                Equals(const wchar_t* str) const;

        bool                operator==(const wchar_t* str) const    { return Equals(str); }
        bool                operator==(const String& other) const   { return Equals(other.CStr()); }

        wchar_t&            operator[](size_t i)                    { return m_Data[i]; }
        const wchar_t&      operator[](size_t i) const              { return m_Data[i]; }

        Vector<wchar_t>     m_Data{g_GlobalAlloc};
    };

    template <CharT T>
    struct StringViewImpl
    {
        constexpr                   StringViewImpl() = default;
        constexpr                   StringViewImpl(const StringViewImpl& other) = default;
        constexpr                   StringViewImpl(const T* str, size_t len) : m_Str(str), m_Len(len) {}
        constexpr                   StringViewImpl(const T* s) : m_Str(s), m_Len(StrLen(s)) {}
                                    StringViewImpl(std::nullptr_t) = delete;
                                    StringViewImpl(std::nullptr_t, size_t len) = delete;

        constexpr StringViewImpl&   operator=(const StringViewImpl& other) = default;

        constexpr const T*          begin()  const                                  { return m_Str; }
        constexpr const T*          end()    const                                  { return m_Str + m_Len; }

        constexpr size_t            Length() const                                  { return m_Len; }
        constexpr bool              Empty() const                                   { return m_Len == 0; }
        
        constexpr const T&          operator[](size_t i) const                      { HAX_ASSERT(i < m_Len); return m_Str[i]; }

        constexpr const T&          First() const                                   { HAX_ASSERT(m_Len > 0); return m_Str[0]; }
        constexpr const T&          Last() const                                    { HAX_ASSERT(m_Len > 0); return m_Str[m_Len - 1]; }

        constexpr const T*          Data() const                                    { return m_Str; }

        constexpr StringViewImpl    Substr(size_t pos, size_t n = kSizeMax) const   { HAX_ASSERT(pos <= m_Len); return StringViewImpl(m_Str + pos, Min(n, m_Len - pos)); }
        constexpr bool              Equals(const StringViewImpl& other) const       { if (m_Len != other.m_Len) return false; if (m_Str == other.m_Str) return true; for (size_t i = 0; i < m_Len; ++i) { if (m_Str[i] != other.m_Str[i]) return false; } return true; }

        constexpr bool              operator==(const StringViewImpl& other) const   { return Equals(other); }

    private:
        const T*                    m_Str = nullptr;
        size_t                      m_Len = 0;
    };

    using StringView  = StringViewImpl<wchar_t>;
    using AnsiStringView = StringViewImpl<char>;
}

namespace Hax
{
#ifdef _WIN64
    static constexpr size_t kFnvOffset = 0xcbf29ce484222325ULL;
    static constexpr size_t kFnvPrime  = 0x100000001b3ULL;
#else
    static constexpr size_t kFnvOffset = 0x811c9dc5U;
    static constexpr size_t kFnvPrime  = 0x01000193U;
#endif

    inline size_t Hash(const void* data, size_t sizeBytes, size_t seed = kFnvOffset) 
    {
        const uint8* ptr = static_cast<const uint8*>(data);
        for (size_t i = 0; i < sizeBytes; ++i) 
        {
            seed ^= ptr[i];
            seed *= kFnvPrime;
        }
        return seed;
    }

    template <CharT T>
    inline constexpr size_t Hash(StringViewImpl<T> sv) 
    {
        size_t hash = kFnvOffset;
        for (size_t i = 0; i < sv.Length(); ++i) 
        {
            hash ^= static_cast<size_t>(sv[i]);
            hash *= kFnvPrime;
        }
        return hash;
    }

    inline size_t Hash(const String& s) 
    {
        return Hash(StringViewImpl(s.CStr(), s.Length()));
    }

    template <CharT T>
    inline constexpr size_t Hash(const T* str)
    {
        return Hash(StringViewImpl(str));
    }

    template <TriCo T>
    inline size_t Hash(const T& v)
    {
        return Hash(&v, sizeof(T));
    }

    template <typename T>
    constexpr size_t GetTypeId() 
    { 
        return Hash(__FUNCSIG__);  
    }
}

namespace Hax
{
    template <TriCo T>
    void Vector<T>::EnsureCapacity(size_t desired)
    {  
        if (desired <= m_Capacity)
            return;

        size_t newCapacity = m_Capacity ? (m_Capacity + m_Capacity / 2) : 8;
        newCapacity = Max(newCapacity, desired);

        T* newData = (T*)m_Alloc->Alloc(newCapacity * sizeof(T));
        if (m_Data)
        {
            memcpy(newData, m_Data, sizeof(T) * m_Size);
            m_Alloc->Free(m_Data);
        }

        m_Data = newData;
        m_Capacity = newCapacity;
    }

    template <TriCo T>
    void Vector<T>::Reserve(size_t newCapacity)
    { 
        if (newCapacity <= m_Capacity) 
            return;

        T* newData = (T*)m_Alloc->Alloc(newCapacity * sizeof(T)); 
        if (m_Data)
        {
            memcpy(newData, m_Data, m_Size * sizeof(T)); 
            m_Alloc->Free(m_Data);
        }

        m_Data = newData;
        m_Capacity = newCapacity;
    }

    template <TriCo T>
    T* Vector<T>::Insert(const T* it, const T& v)     
    { 
        const size_t off = it - m_Data;
        HAX_ASSERT(off >= 0 && off <= m_Size);

        EnsureCapacity(m_Size + 1);
        if (off < m_Size)
            memmove(m_Data + off + 1, m_Data + off, (m_Size - off) * sizeof(T));

        memcpy(&m_Data[off], &v, sizeof(v));
        m_Size++;
        return m_Data + off;
    }

    template <TriCo T>
    T* Vector<T>::Insert(size_t off, const T& v) 
    { 
        HAX_ASSERT(off <= m_Size);

        EnsureCapacity(m_Size + 1);
        if (off < m_Size)
            memmove(m_Data + off + 1, m_Data + off, (m_Size - off) * sizeof(T));

        memcpy(&m_Data[off], &v, sizeof(v));
        m_Size++;
        return m_Data + off;
    }

    template <TriCo T>
    T* Vector<T>::Erase(const T* it)                  
    { 
        const size_t off = it - m_Data;
        HAX_ASSERT(off < m_Size); 

        if (off == 0)
            return m_Data;

        memmove(m_Data + off, m_Data + off + 1, (m_Size - off - 1) * sizeof(T));
        m_Size--;
        return m_Data + off;
    }

    template <TriCo T>
    T* Vector<T>::Erase(size_t off)                  
    {
        HAX_ASSERT(off < m_Size); 

        if (off < m_Size - 1)
            memmove(m_Data + off, m_Data + off + 1, (m_Size - off - 1) * sizeof(T));

        m_Size--;

        return m_Data + off;
    }

    template <TriCo T>
    T* Vector<T>::Erase(const T* it, const T* itLast)
    { 
        const size_t off = it - m_Data;
        HAX_ASSERT(off >= 0 && off < m_Size && itLast >= it && itLast <= m_Data + m_Size); 

        const size_t count = itLast - it;
        memmove(m_Data + off, m_Data + off + count, (m_Size - off - count) * sizeof(T));
        m_Size -= count;
        return m_Data + off;
    }

    template <TriCo K, TriCo V>
    const V* Map<K, V>::Get(const K& key) const
    {
        size_t idx = FindLowerBound(key);
        const size_t size = m_Entries.Size();

        if (idx < size)
        {
            const auto* data = m_Entries.Data();
            if (data[idx].key == key)
                return &data[idx].value;
        }
        return nullptr;
    }

    template <TriCo K, TriCo V>
    V& Map<K, V>::FindOrAdd(const K& key)
    {
        size_t idx = FindLowerBound(key);
        Entry* data = m_Entries.Data();

        if (idx < m_Entries.Size() && data[idx].key == key)
            return data[idx].value;

        m_Entries.Insert(idx, {key, V{}});
        return m_Entries.Data()[idx].value;
    }

    template <TriCo K, TriCo V>
    bool Map<K, V>::Erase(const K& key)
    {
        size_t idx = FindLowerBound(key);
        if (idx < m_Entries.Size() && m_Entries.Data()[idx].key == key) 
        {
            m_Entries.Erase(idx);
            return true;
        }
        return false;
    }

    template <TriCo K, TriCo V>
    size_t Map<K, V>::FindLowerBound(const K& key) const
    {
        const Entry* data = m_Entries.Data();
        size_t count = m_Entries.Size();

        if (count < 64)
        {
            size_t idx = 0;
            while (idx < count && data[idx].key < key)
                idx++;
            return idx;
        }

        size_t first = 0;
        while (count > 0) 
        {
            size_t step = count / 2;
            size_t mid = first + step;

            if (data[mid].key < key) 
            {
                first = mid + 1;
                count -= step + 1;
            } 
            else 
            {
                count = step;
            }
        }
        return first;
    }

    inline String& String::operator=(const wchar_t* str)
    {
        if (str == CStr())
            return *this;

        ClearFree();

        if (str && str[0] != '\0')
        {
            size_t len = 0;
            while (str[len]) len++;
            m_Data.Resize(len + 1);
            memcpy(m_Data.Data(), str, (len + 1) * sizeof(wchar_t));
        }

        return *this;
    }

    inline void String::Append(const wchar_t* str) 
    {
        if (str && str[0] != '\0')
        {
            size_t addLen = 0;
            while (str[addLen]) addLen++;

            size_t oldLen = Length();
            m_Data.Resize(oldLen + addLen + 1);
            memcpy(m_Data.Data() + oldLen, str, (addLen + 1) * sizeof(wchar_t));
        }
    }

    inline bool String::Equals(const wchar_t* str) const 
    {
        if (!str || str[0] == '\0') 
            return Empty();

        const wchar_t* p1 = CStr();
        while (*p1 && (*p1 == *str))
        {
            p1++; str++;
        }

        return *p1 == *str;
    }
}