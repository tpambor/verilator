// -*- mode: C++; c-file-style: "cc-mode" -*-
//=============================================================================
//
// THIS MODULE IS PUBLICLY LICENSED
//
// Copyright 2000-2021 by Wilson Snyder. This program is free software; you
// can redistribute it and/or modify it under the terms of either the GNU
// Lesser General Public License Version 3 or the Perl Artistic License
// Version 2.0.
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//=============================================================================
///
/// \file
/// \brief Save-restore serialization of verilated modules
///
//=============================================================================

#ifndef VERILATOR_VERILATED_SAVE_C_H_
#define VERILATOR_VERILATED_SAVE_C_H_

#include "verilatedos.h"
#include "verilated_heavy.h"

#include <string>

//=============================================================================
// VerilatedSerialize - convert structures to a stream representation
// This class is not thread safe, it must be called by a single thread

class VerilatedSerialize VL_NOT_FINAL {
protected:
    // MEMBERS
    // For speed, keep m_cp as the first member of this structure
    vluint8_t* m_cp;  ///< Current pointer into m_bufp buffer
    vluint8_t* m_bufp;  ///< Output buffer
    bool m_isOpen = false;  ///< True indicates open file/stream
    std::string m_filename;  ///< Filename, for error messages
    VerilatedAssertOneThread m_assertOne;  ///< Assert only called from single thread

    static constexpr size_t bufferSize() { return 256 * 1024; }  // See below for slack calculation
    static constexpr size_t bufferInsertSize() { return 16 * 1024; }

    void header() VL_MT_UNSAFE_ONE;
    void trailer() VL_MT_UNSAFE_ONE;

    // CONSTRUCTORS
    VL_UNCOPYABLE(VerilatedSerialize);

public:
    VerilatedSerialize() {
        m_bufp = new vluint8_t[bufferSize()];
        m_cp = m_bufp;
    }
    virtual ~VerilatedSerialize() {
        close();
        if (m_bufp) VL_DO_CLEAR(delete[] m_bufp, m_bufp = nullptr);
    }
    // METHODS
    bool isOpen() const { return m_isOpen; }
    std::string filename() const { return m_filename; }
    virtual void close() VL_MT_UNSAFE_ONE { flush(); }
    virtual void flush() VL_MT_UNSAFE_ONE {}
    VerilatedSerialize& write(const void* __restrict datap, size_t size) VL_MT_UNSAFE_ONE {
        const vluint8_t* __restrict dp = (const vluint8_t* __restrict)datap;
        while (size) {
            bufferCheck();
            size_t blk = size;
            if (blk > bufferInsertSize()) blk = bufferInsertSize();
            const vluint8_t* __restrict maxp = dp + blk;
            for (; dp < maxp; *m_cp++ = *dp++) {}
            size -= blk;
        }
        return *this;  // For function chaining
    }

private:
    VerilatedSerialize& bufferCheck() VL_MT_UNSAFE_ONE {
        // Flush the write buffer if there's not enough space left for new information
        // We only call this once per vector, so we need enough slop for a very wide "b###" line
        if (VL_UNLIKELY(m_cp > (m_bufp + (bufferSize() - bufferInsertSize())))) flush();
        return *this;  // For function chaining
    }
};

//=============================================================================
// VerilatedDeserial - load structures from a stream representation
// This class is not thread safe, it must be called by a single thread

class VerilatedDeserialize VL_NOT_FINAL {
protected:
    // MEMBERS
    // For speed, keep m_cp as the first member of this structure
    vluint8_t* m_cp;  ///< Current pointer into m_bufp buffer
    vluint8_t* m_bufp;  ///< Output buffer
    vluint8_t* m_endp = nullptr;  ///< Last valid byte in m_bufp buffer
    bool m_isOpen = false;  ///< True indicates open file/stream
    std::string m_filename;  ///< Filename, for error messages
    VerilatedAssertOneThread m_assertOne;  ///< Assert only called from single thread

    static constexpr size_t bufferSize() { return 256 * 1024; }  // See below for slack calculation
    static constexpr size_t bufferInsertSize() { return 16 * 1024; }

    virtual void fill() = 0;
    void header() VL_MT_UNSAFE_ONE;
    void trailer() VL_MT_UNSAFE_ONE;

    // CONSTRUCTORS
    VL_UNCOPYABLE(VerilatedDeserialize);

public:
    VerilatedDeserialize() {
        m_bufp = new vluint8_t[bufferSize()];
        m_cp = m_bufp;
    }
    virtual ~VerilatedDeserialize() {
        close();
        if (m_bufp) VL_DO_CLEAR(delete[] m_bufp, m_bufp = nullptr);
    }
    // METHODS
    bool isOpen() const { return m_isOpen; }
    std::string filename() const { return m_filename; }
    virtual void close() VL_MT_UNSAFE_ONE { flush(); }
    virtual void flush() VL_MT_UNSAFE_ONE {}
    VerilatedDeserialize& read(void* __restrict datap, size_t size) VL_MT_UNSAFE_ONE {
        vluint8_t* __restrict dp = static_cast<vluint8_t* __restrict>(datap);
        while (size) {
            bufferCheck();
            size_t blk = size;
            if (blk > bufferInsertSize()) blk = bufferInsertSize();
            const vluint8_t* __restrict maxp = dp + blk;
            for (; dp < maxp; *dp++ = *m_cp++) {}
            size -= blk;
        }
        return *this;  // For function chaining
    }
    // Read a datum and compare with expected value
    VerilatedDeserialize& readAssert(const void* __restrict datap, size_t size) VL_MT_UNSAFE_ONE;
    VerilatedDeserialize& readAssert(vluint64_t data) VL_MT_UNSAFE_ONE {
        return readAssert(&data, sizeof(data));
    }

private:
    bool readDiffers(const void* __restrict datap, size_t size) VL_MT_UNSAFE_ONE;
    VerilatedDeserialize& bufferCheck() VL_MT_UNSAFE_ONE {
        // Flush the write buffer if there's not enough space left for new information
        // We only call this once per vector, so we need enough slop for a very wide "b###" line
        if (VL_UNLIKELY((m_cp + bufferInsertSize()) > m_endp)) fill();
        return *this;  // For function chaining
    }
};

//=============================================================================
// VerilatedSave - serialize to a file
// This class is not thread safe, it must be called by a single thread

class VerilatedSave final : public VerilatedSerialize {
private:
    int m_fd = -1;  ///< File descriptor we're writing to

public:
    // CONSTRUCTORS
    VerilatedSave() = default;
    virtual ~VerilatedSave() override { close(); }
    // METHODS
    /// Open the file; call isOpen() to see if errors
    void open(const char* filenamep) VL_MT_UNSAFE_ONE;
    void open(const std::string& filename) VL_MT_UNSAFE_ONE { open(filename.c_str()); }
    virtual void close() override VL_MT_UNSAFE_ONE;
    virtual void flush() override VL_MT_UNSAFE_ONE;
};

//=============================================================================
// VerilatedRestore - deserialize from a file
// This class is not thread safe, it must be called by a single thread

class VerilatedRestore final : public VerilatedDeserialize {
private:
    int m_fd = -1;  ///< File descriptor we're writing to

public:
    // CONSTRUCTORS
    VerilatedRestore() = default;
    virtual ~VerilatedRestore() override { close(); }

    // METHODS
    /// Open the file; call isOpen() to see if errors
    void open(const char* filenamep) VL_MT_UNSAFE_ONE;
    void open(const std::string& filename) VL_MT_UNSAFE_ONE { open(filename.c_str()); }
    virtual void close() override VL_MT_UNSAFE_ONE;
    virtual void flush() override VL_MT_UNSAFE_ONE {}
    virtual void fill() override VL_MT_UNSAFE_ONE;
};

//=============================================================================

inline VerilatedSerialize& operator<<(VerilatedSerialize& os, vluint64_t& rhs) {
    return os.write(&rhs, sizeof(rhs));
}
inline VerilatedDeserialize& operator>>(VerilatedDeserialize& os, vluint64_t& rhs) {
    return os.read(&rhs, sizeof(rhs));
}
inline VerilatedSerialize& operator<<(VerilatedSerialize& os, vluint32_t& rhs) {
    return os.write(&rhs, sizeof(rhs));
}
inline VerilatedDeserialize& operator>>(VerilatedDeserialize& os, vluint32_t& rhs) {
    return os.read(&rhs, sizeof(rhs));
}
inline VerilatedSerialize& operator<<(VerilatedSerialize& os, vluint16_t& rhs) {
    return os.write(&rhs, sizeof(rhs));
}
inline VerilatedDeserialize& operator>>(VerilatedDeserialize& os, vluint16_t& rhs) {
    return os.read(&rhs, sizeof(rhs));
}
inline VerilatedSerialize& operator<<(VerilatedSerialize& os, vluint8_t& rhs) {
    return os.write(&rhs, sizeof(rhs));
}
inline VerilatedDeserialize& operator>>(VerilatedDeserialize& os, vluint8_t& rhs) {
    return os.read(&rhs, sizeof(rhs));
}
inline VerilatedSerialize& operator<<(VerilatedSerialize& os, bool& rhs) {
    return os.write(&rhs, sizeof(rhs));
}
inline VerilatedDeserialize& operator>>(VerilatedDeserialize& os, bool& rhs) {
    return os.read(&rhs, sizeof(rhs));
}
inline VerilatedSerialize& operator<<(VerilatedSerialize& os, double& rhs) {
    return os.write(&rhs, sizeof(rhs));
}
inline VerilatedDeserialize& operator>>(VerilatedDeserialize& os, double& rhs) {
    return os.read(&rhs, sizeof(rhs));
}
inline VerilatedSerialize& operator<<(VerilatedSerialize& os, float& rhs) {
    return os.write(&rhs, sizeof(rhs));
}
inline VerilatedDeserialize& operator>>(VerilatedDeserialize& os, float& rhs) {
    return os.read(&rhs, sizeof(rhs));
}
inline VerilatedSerialize& operator<<(VerilatedSerialize& os, const std::string& rhs) {
    vluint32_t len = rhs.length();
    os << len;
    return os.write(rhs.data(), len);
}
inline VerilatedDeserialize& operator>>(VerilatedDeserialize& os, std::string& rhs) {
    vluint32_t len = 0;
    os >> len;
    rhs.resize(len);
    return os.read((void*)rhs.data(), len);
}
VerilatedSerialize& operator<<(VerilatedSerialize& os, VerilatedContext* rhsp);
VerilatedDeserialize& operator>>(VerilatedDeserialize& os, VerilatedContext* rhsp);

template <class T_Key, class T_Value>
VerilatedSerialize& operator<<(VerilatedSerialize& os, VlAssocArray<T_Key, T_Value>& rhs) {
    os << rhs.atDefault();
    vluint32_t len = rhs.size();
    os << len;
    for (const auto& i : rhs) {
        T_Key index = i.first;  // Copy to get around const_iterator
        T_Value value = i.second;
        os << index << value;
    }
    return os;
}
template <class T_Key, class T_Value>
VerilatedDeserialize& operator>>(VerilatedDeserialize& os, VlAssocArray<T_Key, T_Value>& rhs) {
    os >> rhs.atDefault();
    vluint32_t len = 0;
    os >> len;
    rhs.clear();
    for (vluint32_t i = 0; i < len; ++i) {
        T_Key index;
        T_Value value;
        os >> index;
        os >> value;
        rhs.at(index) = value;
    }
    return os;
}

#endif  // Guard
