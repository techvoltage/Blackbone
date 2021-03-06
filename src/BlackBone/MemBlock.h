#pragma once

#include "Winheaders.h"
#include "Macro.h"
#include "Types.h"

#include <stdint.h>
#include <map>
#include <set>

namespace blackbone
{

/// <summary>
/// Get rid of EXECUTABLE flag if DEP isn't enabled
/// </summary>
/// <param name="prot">Memry protection flags</param>
/// <param name="bDEP">DEP flag</param>
/// <returns>New protection flags</returns>
inline DWORD CastProtection( DWORD prot, bool bDEP )
{
    if (bDEP == true)
    {
        return prot;
    }
    else
    {
        if (prot == PAGE_EXECUTE_READ)
            return PAGE_READONLY;
        else if (prot == PAGE_EXECUTE_READWRITE)
            return PAGE_READWRITE;
        else if (prot == PAGE_EXECUTE_WRITECOPY)
            return PAGE_WRITECOPY;
        else
            return prot;
    }
}

class MemBlock
{
public:

    MemBlock();

    /// <summary>
    /// MemBlock ctor
    /// </summary>
    /// <param name="mem">Process memory routines</param>
    /// <param name="ptr">Memory address</param>
    /// <param name="own">true if caller will be responsible for block deallocation</param>
    MemBlock( class ProcessMemory* mem, ptr_t ptr, bool own = true );

    /// <summary>
    /// MemBlock ctor
    /// </summary>
    /// <param name="mem">Process memory routines</param>
    /// <param name="ptr">Memory address</param>
    /// <param name="size">Block size</param>
    /// <param name="prot">Memory protection</param>
    /// <param name="own">true if caller will be responsible for block deallocation</param>
    MemBlock( class ProcessMemory* mem, ptr_t ptr, size_t size, DWORD prot, bool own = true );

    ~MemBlock();

    /// <summary>
    /// Allocate new memory block
    /// </summary>
    /// <param name="process">Process memory routines</param>
    /// <param name="size">Block size</param>
    /// <param name="desired">Desired base address of new block</param>
    /// <param name="protection">Win32 Memory protection flags</param>
    /// <returns>Memory block. If failed - returned block will be invalid</returns>
    static MemBlock Allocate( class ProcessMemory& process, size_t size, ptr_t desired = 0, DWORD protection = PAGE_EXECUTE_READWRITE );

    /// <summary>
    /// Reallocate existing block for new size
    /// </summary>
    /// <param name="size">New block size</param>
    /// <param name="desired">Desired base address of new block</param>
    /// <param name="protection">Memory protection</param>
    /// <returns>New block address</returns>
    ptr_t Realloc( size_t size, ptr_t desired = 0, DWORD protection = PAGE_EXECUTE_READWRITE );

    /// <summary>
    /// Change memory protection
    /// </summary>
    /// <param name="protection">New protection flags</param>
    /// <param name="offset">Memory offset in block</param>
    /// <param name="size">Block size</param>
    /// <param name="pOld">Old protection flags</param>
    /// <returns>Status</returns>
    NTSTATUS Protect( DWORD protection, size_t offset = 0, size_t size = 0, DWORD* pOld = nullptr );

    /// <summary>
    /// Free memory
    /// </summary>
    /// <param name="size">Size of memory chunk to free. If 0 - whole block is freed</param>
    NTSTATUS Free( size_t size = 0 );

    /// <summary>
    /// Read data
    /// </summary>
    /// <param name="offset">Data offset in block</param>
    /// <param name="size">Size of data to read</param>
    /// <param name="pResult">Output buffer</param>
    /// <param name="handleHoles">
    /// If true, function will try to read all committed pages in range ignoring uncommitted.
    /// Otherwise function will fail if there is at least one non-committed page in region.
    /// </param>
    /// <returns>Status</returns>
    NTSTATUS Read( size_t offset, size_t size, PVOID pResult, bool handleHoles = false );

    /// <summary>
    /// Write data
    /// </summary>
    /// <param name="offset">Data offset in block</param>
    /// <param name="size">Size of data to write</param>
    /// <param name="pData">Buffer to write</param>
    /// <returns>Status</returns>
    NTSTATUS Write( size_t offset, size_t size, const void* pData );

    /// <summary>
    /// Read data
    /// </summary>
    /// <param name="offset">Data offset in block</param>
    /// <param name="def_val">Defult return value if read has failed</param>
    /// <returns>Read data</returns>
    template<class T>
    T Read( size_t offset, T def_val )
    {
        T res = def_val;
        Read( offset, sizeof(T), &res );
        return res;
    };

    /// <summary>
    /// Write data
    /// </summary>
    /// <param name="offset">Offset in block</param>
    /// <param name="data">Data to write</param>
    /// <returns>Status</returns>
    template<class T>
    NTSTATUS Write( size_t offset, T data )
    {
        return Write( offset, sizeof(T), &data );
    }

    /// <summary>
    /// Memory will not be deallocated upon object destruction
    /// </summary>
    inline void Release() const  { _own = false; }

    /// <summary>
    /// Get memory pointer
    /// </summary>
    /// <returns>Memory pointer</returns>
    template< typename T = ptr_t >
    inline T ptr() const { return (T)_ptr; }

    /// <summary>
    /// Get block size
    /// </summary>
    /// <returns>Block size</returns>
    inline size_t size() const { return _size; }

    /// <summary>
    /// Get block memory protection
    /// </summary>
    /// <returns>Memory protection flags</returns>
    inline DWORD  protection() const { return _protection; }

    /// <summary>
    /// Validate memory block
    /// <returns>true if memory pointer isn't 0</returns>
    inline bool valid() const { return (_memory != nullptr && _ptr != 0); }

    /// <summary>
    /// Get memory pointer
    /// </summary>
    /// <returns>Memory pointer</returns>
    operator ptr_t() const  { return _ptr; }

    MemBlock& operator =(const MemBlock& other)
    {
        _memory = other._memory;
        _ptr = other._ptr;
        _size = other._size;
        _protection = other._protection;
        _own = true;

        // Transfer memory ownership
        other.Release();

        return *this;
    }

private:
    ptr_t        _ptr = 0;          // Raw memory pointer
    size_t       _size = 0;         // Region size
    DWORD        _protection = 0;   // Region protection
    mutable bool _own = true;       // Memory will be freed in destructor
    class ProcessMemory* _memory;   // Target process routines
};

}