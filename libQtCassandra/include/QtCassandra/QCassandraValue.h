/*
 * Header:
 *      QCassandraValue.h
 *
 * Description:
 *      Handling of a cell value to access data within the Cassandra database.
 *
 * Documentation:
 *      See the corresponding .cpp file.
 *
 * License:
 *      Copyright (c) 2011-2014 Made to Order Software Corp.
 * 
 *      http://snapwebsites.org/
 *      contact@m2osw.com
 * 
 *      Permission is hereby granted, free of charge, to any person obtaining a
 *      copy of this software and associated documentation files (the
 *      "Software"), to deal in the Software without restriction, including
 *      without limitation the rights to use, copy, modify, merge, publish,
 *      distribute, sublicense, and/or sell copies of the Software, and to
 *      permit persons to whom the Software is furnished to do so, subject to
 *      the following conditions:
 *
 *      The above copyright notice and this permission notice shall be included
 *      in all copies or substantial portions of the Software.
 *
 *      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef QCASSANDRA_VALUE_H
#define QCASSANDRA_VALUE_H

#include "QtCassandra/QCassandraConsistencyLevel.h"
#include <controlled_vars/controlled_vars_limited_auto_init.h>
#include <controlled_vars/controlled_vars_limited_auto_enum_init.h>
#include <QString>
#include <QByteArray>
#include <memory>
#include <stdint.h>


namespace QtCassandra
{

class QCassandraPrivate;

namespace {
    /** \brief Maximum buffer size.
     *
     * This variable represents the maximum buffer size of a QCassandraValue.
     * At this time this is limited to 64Mb. Some users have successfully used
     * Cassandra with 200Mb buffers, however, remember that you need a huge
     * amount of RAM to handle large buffers (your copy is 200Mb, Cassandra's
     * copy is 200Mb, that's at least 0.5Gb of RAM just for that ONE cell!)
     */
    const uint64_t BUFFER_MAX_SIZE = 64 * 1024 * 1024;
}

// Verify final buffer size against limit
inline void checkBufferSize(const uint64_t new_size)
{
    if(new_size > BUFFER_MAX_SIZE) {
        throw std::runtime_error(QString("resulting value is more than %1 bytes").arg(BUFFER_MAX_SIZE).toUtf8().data());
    }
}

// Null
inline void setNullValue(QByteArray& array)
{
    array.clear();
}

// Bool
inline void appendBoolValue(QByteArray& array, const bool value)
{
    checkBufferSize(array.size() + 1);

    char buf[1];
    buf[0] = value ? 1 : 0;
    array.append(buf, 1);
}

inline void setBoolValue(QByteArray& array, const bool value)
{
    array.clear();
    appendBoolValue(array, value);
}

inline bool boolValue(const QByteArray& array, const int index = 0)
{
    if(static_cast<unsigned int>(index) >= static_cast<unsigned int>(array.size())) {
        throw std::runtime_error("buffer too small for this boolValue");
    }
    return array.at(index) != 0;
}

inline bool boolValueOrNull(const QByteArray& array, const int index = 0, const bool default_value = false)
{
    if(static_cast<unsigned int>(index) >= static_cast<unsigned int>(array.size())) {
        return default_value;
    }
    return array.at(index) != 0;
}

// Char
inline void appendCharValue(QByteArray& array, const char value)
{
    checkBufferSize(array.size() + 1);

    array.append(&value, 1);
}

inline void setCharValue(QByteArray& array, const char value)
{
    array.clear();
    appendCharValue(array, value);
}

inline char charValue(const QByteArray& array, const int index = 0)
{
    if(static_cast<unsigned int>(index) >= static_cast<unsigned int>(array.size())) {
        throw std::runtime_error("buffer too small for this charValue");
    }
    return array.at(index);
}

inline char charValueOrNull(const QByteArray& array, const int index = 0, const char default_value = 0)
{
    if(static_cast<unsigned int>(index) >= static_cast<unsigned int>(array.size())) {
        return default_value;
    }
    return array.at(index);
}

inline void appendSignedCharValue(QByteArray& array, const signed char value)
{
    appendCharValue(array, value);
}

inline void setSignedCharValue(QByteArray& array, const signed char value)
{
    setCharValue(array, value);
}

inline signed char signedCharValue(const QByteArray& array, const int index = 0)
{
    if(static_cast<unsigned int>(index) >= static_cast<unsigned int>(array.size())) {
        throw std::runtime_error("buffer too small for this signedCharValue");
    }
    return static_cast<signed char>(array.at(index));
}

inline signed char signedCharValueOrNull(const QByteArray& array, const int index = 0, const signed char default_value = 0)
{
    if(static_cast<unsigned int>(index) >= static_cast<unsigned int>(array.size())) {
        return default_value;
    }
    return array.at(index);
}

inline void appendUnsignedCharValue(QByteArray& array, const unsigned char value)
{
    appendCharValue(array, value);
}

inline void setUnsignedCharValue(QByteArray& array, const unsigned char value)
{
    setCharValue(array, value);
}

inline unsigned char unsignedCharValue(const QByteArray& array, const int index = 0)
{
    if(static_cast<unsigned int>(index) >= static_cast<unsigned int>(array.size())) {
        throw std::runtime_error("buffer too small for this unsignedCharValue");
    }
    return static_cast<unsigned char>(array.at(index));
}

inline unsigned char unsignedCharValueOrNull(const QByteArray& array, const int index = 0, const unsigned char default_value = 0)
{
    if(static_cast<unsigned int>(index) >= static_cast<unsigned int>(array.size())) {
        return default_value;
    }
    return array.at(index);
}

// Int16
inline void appendInt16Value(QByteArray& array, const int16_t value)
{
    checkBufferSize(array.size() + sizeof(int16_t));

    char buf[2];
    buf[0] = static_cast<char>(value >> 8);
    buf[1] = static_cast<char>(value);
    array.append(buf, 2);
}

inline void setInt16Value(QByteArray& array, const int16_t value)
{
    array.clear();
    appendInt16Value(array, value);
}

inline int16_t int16Value(const QByteArray& array, const int index = 0)
{
    if(static_cast<unsigned int>(index) > static_cast<unsigned int>(array.size()) // avoid potential overflow with the +2
    || static_cast<unsigned int>(index + sizeof(int16_t)) > static_cast<unsigned int>(array.size())) {
        throw std::runtime_error("buffer too small for this int16Value");
    }
    return static_cast<int16_t>((static_cast<unsigned char>(array.at(index + 0)) << 8)
                               | static_cast<unsigned char>(array.at(index + 1)));
}

inline int16_t int16ValueOrNull(const QByteArray& array, const int index = 0, const int16_t default_value = 0)
{
    if(static_cast<unsigned int>(index + sizeof(int16_t)) <= static_cast<unsigned int>(array.size())) {
        return static_cast<int16_t>((static_cast<unsigned char>(array.at(index + 0)) << 8)
                                   | static_cast<unsigned char>(array.at(index + 1)));
    }
    if(static_cast<unsigned int>(index) <= static_cast<unsigned int>(array.size())) {
        return default_value;
    }
    throw std::runtime_error("buffer too small for this int16ValueOrNull");
}

inline int16_t safeInt16Value(const QByteArray& array, const int index = 0, const int16_t default_value = 0)
{
    if(static_cast<unsigned int>(index + sizeof(int16_t)) <= static_cast<unsigned int>(array.size())) {
        return static_cast<int16_t>((static_cast<unsigned char>(array.at(index + 0)) << 8)
                                   | static_cast<unsigned char>(array.at(index + 1)));
    }
    return default_value;
}

inline void appendUInt16Value(QByteArray& array, const uint16_t value)
{
    appendInt16Value(array, value);
}

inline void setUInt16Value(QByteArray& array, const uint16_t value)
{
    setInt16Value(array, value);
}

inline uint16_t uint16Value(const QByteArray& array, const int index = 0)
{
    if(static_cast<unsigned int>(index) > static_cast<unsigned int>(array.size()) // test to make sure we catch any overflow
    || static_cast<unsigned int>(index + sizeof(uint16_t)) > static_cast<unsigned int>(array.size())) {
        throw std::runtime_error("buffer too small for this uint16Value");
    }
    return static_cast<uint16_t>((static_cast<unsigned char>(array.at(index + 0)) << 8)
                                | static_cast<unsigned char>(array.at(index + 1)));
}

inline uint16_t uint16ValueOrNull(const QByteArray& array, const int index = 0, const uint16_t default_value = 0)
{
    if(static_cast<unsigned int>(index + sizeof(uint16_t)) <= static_cast<unsigned int>(array.size())) {
        return static_cast<uint16_t>((static_cast<unsigned char>(array.at(index + 0)) << 8)
                                    | static_cast<unsigned char>(array.at(index + 1)));
    }
    if(static_cast<unsigned int>(index) <= static_cast<unsigned int>(array.size())) {
        return default_value;
    }
    throw std::runtime_error("buffer too small for this uint16ValueOrNull");
}

inline uint16_t safeUInt16Value(const QByteArray& array, const int index = 0, const uint16_t default_value = 0)
{
    if(static_cast<unsigned int>(index + sizeof(uint16_t)) <= static_cast<unsigned int>(array.size())) {
        return static_cast<uint16_t>((static_cast<unsigned char>(array.at(index + 0)) << 8)
                                    | static_cast<unsigned char>(array.at(index + 1)));
    }
    return default_value;
}

// Int32
inline void appendInt32Value(QByteArray& array, const int32_t value)
{
    checkBufferSize(array.size() + sizeof(int32_t));

    char buf[4];
    buf[0] = static_cast<char>(value >> 24);
    buf[1] = static_cast<char>(value >> 16);
    buf[2] = static_cast<char>(value >> 8);
    buf[3] = static_cast<char>(value);
    array.append(buf, 4);
}

inline void setInt32Value(QByteArray& array, const int32_t value)
{
    array.clear();
    appendInt32Value(array, value);
}

inline int32_t int32Value(const QByteArray& array, const int index = 0)
{
    if(static_cast<unsigned int>(index) > static_cast<unsigned int>(array.size()) // test to make sure we catch any overflow
    || static_cast<unsigned int>(index + sizeof(int32_t)) > static_cast<unsigned int>(array.size())) {
        throw std::runtime_error("buffer too small for this int32Value");
    }
    return (static_cast<int32_t>(static_cast<unsigned char>(array.at(index + 0))) << 24)
         | (static_cast<int32_t>(static_cast<unsigned char>(array.at(index + 1))) << 16)
         | (static_cast<int32_t>(static_cast<unsigned char>(array.at(index + 2))) << 8)
         | static_cast<int32_t>(static_cast<unsigned char>(array.at(index + 3)));
}

inline int32_t int32ValueOrNull(const QByteArray& array, const int index = 0, const int32_t default_value = 0)
{
    if(static_cast<unsigned int>(index + sizeof(int32_t)) <= static_cast<unsigned int>(array.size())) {
        return (static_cast<int32_t>(static_cast<unsigned char>(array.at(index + 0))) << 24)
             | (static_cast<int32_t>(static_cast<unsigned char>(array.at(index + 1))) << 16)
             | (static_cast<int32_t>(static_cast<unsigned char>(array.at(index + 2))) << 8)
             | static_cast<int32_t>(static_cast<unsigned char>(array.at(index + 3)));
    }
    if(static_cast<unsigned int>(index) <= static_cast<unsigned int>(array.size())) {
        return default_value;
    }
    throw std::runtime_error("buffer too small for this int32ValueOrNull");
}

inline int32_t safeInt32Value(const QByteArray& array, const int index = 0, const int32_t default_value = 0)
{
    if(static_cast<unsigned int>(index + sizeof(int32_t)) <= static_cast<unsigned int>(array.size())) {
        return (static_cast<int32_t>(static_cast<unsigned char>(array.at(index + 0))) << 24)
             | (static_cast<int32_t>(static_cast<unsigned char>(array.at(index + 1))) << 16)
             | (static_cast<int32_t>(static_cast<unsigned char>(array.at(index + 2))) << 8)
             | static_cast<int32_t>(static_cast<unsigned char>(array.at(index + 3)));
    }
    return default_value;
}

inline void appendUInt32Value(QByteArray& array, const uint32_t value)
{
    appendInt32Value(array, value);
}

inline void setUInt32Value(QByteArray& array, const uint32_t value)
{
    setInt32Value(array, value);
}

inline uint32_t uint32Value(const QByteArray& array, const int index = 0)
{
    if(static_cast<unsigned int>(index) > static_cast<unsigned int>(array.size()) // test to make sure we catch any overflow
    || static_cast<unsigned int>(index + sizeof(uint32_t)) > static_cast<unsigned int>(array.size())) {
        throw std::runtime_error("buffer too small for this uint32Value");
    }
    return (static_cast<uint32_t>(static_cast<unsigned char>(array.at(index + 0))) << 24)
         | (static_cast<uint32_t>(static_cast<unsigned char>(array.at(index + 1))) << 16)
         | (static_cast<uint32_t>(static_cast<unsigned char>(array.at(index + 2))) << 8)
         | static_cast<uint32_t>(static_cast<unsigned char>(array.at(index + 3)));
}

inline uint32_t uint32ValueOrNull(const QByteArray& array, const int index = 0, const uint32_t default_value = 0)
{
    if(static_cast<unsigned int>(index + sizeof(uint32_t)) <= static_cast<unsigned int>(array.size())) {
        return (static_cast<uint32_t>(static_cast<unsigned char>(array.at(index + 0))) << 24)
             | (static_cast<uint32_t>(static_cast<unsigned char>(array.at(index + 1))) << 16)
             | (static_cast<uint32_t>(static_cast<unsigned char>(array.at(index + 2))) << 8)
             | static_cast<uint32_t>(static_cast<unsigned char>(array.at(index + 3)));
    }
    if(static_cast<unsigned int>(index) <= static_cast<unsigned int>(array.size())) {
        return default_value;
    }
    throw std::runtime_error("buffer too small for this uint32ValueOrNull");
}

inline uint32_t safeUInt32Value(const QByteArray& array, const int index = 0, const uint32_t default_value = 0)
{
    if(static_cast<unsigned int>(index + sizeof(uint32_t)) <= static_cast<unsigned int>(array.size())) {
        return (static_cast<uint32_t>(static_cast<unsigned char>(array.at(index + 0))) << 24)
             | (static_cast<uint32_t>(static_cast<unsigned char>(array.at(index + 1))) << 16)
             | (static_cast<uint32_t>(static_cast<unsigned char>(array.at(index + 2))) << 8)
             | static_cast<uint32_t>(static_cast<unsigned char>(array.at(index + 3)));
    }
    return default_value;
}

// Int64
inline void appendInt64Value(QByteArray& array, const int64_t value)
{
    checkBufferSize(array.size() + 8);

    char buf[8];
    buf[0] = static_cast<char>(value >> 56);
    buf[1] = static_cast<char>(value >> 48);
    buf[2] = static_cast<char>(value >> 40);
    buf[3] = static_cast<char>(value >> 32);
    buf[4] = static_cast<char>(value >> 24);
    buf[5] = static_cast<char>(value >> 16);
    buf[6] = static_cast<char>(value >> 8);
    buf[7] = static_cast<char>(value);
    array.append(buf, 8);
}

inline void setInt64Value(QByteArray& array, const int64_t value)
{
    array.clear();
    appendInt64Value(array, value);
}

inline int64_t int64Value(const QByteArray& array, const int index = 0)
{
    if(static_cast<unsigned int>(index) > static_cast<unsigned int>(array.size()) // test to make sure we catch any overflow
    || static_cast<unsigned int>(index + sizeof(int64_t)) > static_cast<unsigned int>(array.size())) {
        throw std::runtime_error(QString("buffer too small (%1) for this int64Value").arg(array.size()).toStdString());
    }
    return (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 0))) << 56)
         | (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 1))) << 48)
         | (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 2))) << 40)
         | (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 3))) << 32)
         | (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 4))) << 24)
         | (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 5))) << 16)
         | (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 6))) << 8)
         | static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 7)));
}

inline int64_t int64ValueOrNull(const QByteArray& array, const int index = 0, const int64_t default_value = 0)
{
    if(static_cast<unsigned int>(index + sizeof(int64_t)) <= static_cast<unsigned int>(array.size())) {
        return (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 0))) << 56)
             | (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 1))) << 48)
             | (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 2))) << 40)
             | (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 3))) << 32)
             | (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 4))) << 24)
             | (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 5))) << 16)
             | (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 6))) << 8)
             | static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 7)));
    }
    if(static_cast<unsigned int>(index) <= static_cast<unsigned int>(array.size())) {
        return default_value;
    }
    throw std::runtime_error("buffer too small for this int64ValueOrNull");
}

inline int64_t safeInt64Value(const QByteArray& array, const int index = 0, const int64_t default_value = 0)
{
    if(static_cast<unsigned int>(index + sizeof(int64_t)) <= static_cast<unsigned int>(array.size())) {
        return (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 0))) << 56)
             | (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 1))) << 48)
             | (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 2))) << 40)
             | (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 3))) << 32)
             | (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 4))) << 24)
             | (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 5))) << 16)
             | (static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 6))) << 8)
             | static_cast<int64_t>(static_cast<unsigned char>(array.at(index + 7)));
    }
    return default_value;
}

inline void appendUInt64Value(QByteArray& array, const uint64_t value)
{
    appendInt64Value(array, value);
}

inline void setUInt64Value(QByteArray& array, const uint64_t value)
{
    setInt64Value(array, value);
}

inline uint64_t uint64Value(const QByteArray& array, const int index = 0)
{
    if(static_cast<unsigned int>(index) > static_cast<unsigned int>(array.size()) // test to make sure we catch any overflow
    || static_cast<unsigned int>(index + sizeof(uint64_t)) > static_cast<unsigned int>(array.size())) {
        throw std::runtime_error("buffer too small for this uint64Value");
    }
    return (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 0))) << 56)
         | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 1))) << 48)
         | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 2))) << 40)
         | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 3))) << 32)
         | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 4))) << 24)
         | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 5))) << 16)
         | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 6))) << 8)
         | static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 7)));
}

inline uint64_t uint64ValueOrNull(const QByteArray& array, const int index = 0, const uint64_t default_value = 0)
{
    if(static_cast<unsigned int>(index + sizeof(uint64_t)) <= static_cast<unsigned int>(array.size())) {
        return (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 0))) << 56)
             | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 1))) << 48)
             | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 2))) << 40)
             | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 3))) << 32)
             | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 4))) << 24)
             | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 5))) << 16)
             | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 6))) << 8)
             | static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 7)));
    }
    if(static_cast<unsigned int>(index) <= static_cast<unsigned int>(array.size())) {
        return default_value;
    }
    throw std::runtime_error("buffer too small for this uint64ValueOrNull");
}

inline uint64_t safeUInt64Value(const QByteArray& array, const int index = 0, const uint64_t default_value = 0)
{
    if(static_cast<unsigned int>(index + sizeof(uint64_t)) <= static_cast<unsigned int>(array.size())) {
        return (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 0))) << 56)
             | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 1))) << 48)
             | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 2))) << 40)
             | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 3))) << 32)
             | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 4))) << 24)
             | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 5))) << 16)
             | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 6))) << 8)
             | static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 7)));
    }
    return default_value;
}

// Float
inline void setFloatValue(QByteArray& array, const float value)
{
    union switch_t {
        float f;
        uint32_t v;
    };
    switch_t s;
    s.f = value;
    setInt32Value(array, s.v);
}

inline void appendFloatValue(QByteArray& array, const float value)
{
    union switch_t {
        float f;
        uint32_t v;
    };
    switch_t s;
    s.f = value;
    appendInt32Value(array, s.v);
}

inline float floatValue(const QByteArray& array, const int index = 0)
{
    if(static_cast<unsigned int>(index) > static_cast<unsigned int>(array.size()) // test to make sure we catch any overflow
    || static_cast<unsigned int>(index + sizeof(float)) > static_cast<unsigned int>(array.size())) {
        throw std::runtime_error("buffer too small for this floatValue");
    }
    union switch_t {
        uint32_t    v;
        float       f;
    };
    switch_t s;
    s.v = static_cast<uint32_t>((static_cast<unsigned char>(array.at(index + 0))) << 24)
        | static_cast<uint32_t>((static_cast<unsigned char>(array.at(index + 1))) << 16)
        | static_cast<uint32_t>((static_cast<unsigned char>(array.at(index + 2))) << 8)
        | static_cast<uint32_t>(static_cast<unsigned char>(array.at(index + 3)));
    return s.f;
}

inline float floatValueOrNull(const QByteArray& array, const int index = 0, const float default_value = 0)
{
    if(static_cast<unsigned int>(index + sizeof(float)) <= static_cast<unsigned int>(array.size())) {
        union switch_t {
            uint32_t    v;
            float       f;
        };
        switch_t s;
        s.v = static_cast<uint32_t>((static_cast<unsigned char>(array.at(index + 0))) << 24)
            | static_cast<uint32_t>((static_cast<unsigned char>(array.at(index + 1))) << 16)
            | static_cast<uint32_t>((static_cast<unsigned char>(array.at(index + 2))) << 8)
            | static_cast<uint32_t>(static_cast<unsigned char>(array.at(index + 3)));
        return s.f;
    }
    if(static_cast<unsigned int>(index) <= static_cast<unsigned int>(array.size())) {
        return default_value;
    }
    throw std::runtime_error("buffer too small for this floatValueOrNull");
}

inline float safeFloatValue(const QByteArray& array, const int index = 0, const float default_value = 0)
{
    if(static_cast<unsigned int>(index + sizeof(float)) <= static_cast<unsigned int>(array.size())) {
        union switch_t {
            uint32_t    v;
            float       f;
        };
        switch_t s;
        s.v = static_cast<uint32_t>((static_cast<unsigned char>(array.at(index + 0))) << 24)
            | static_cast<uint32_t>((static_cast<unsigned char>(array.at(index + 1))) << 16)
            | static_cast<uint32_t>((static_cast<unsigned char>(array.at(index + 2))) << 8)
            | static_cast<uint32_t>(static_cast<unsigned char>(array.at(index + 3)));
        return s.f;
    }
    return default_value;
}

// Double
inline void setDoubleValue(QByteArray& array, const double value)
{
    union switch_t {
        double d;
        uint64_t v;
    };
    switch_t s;
    s.d = value;
    setInt64Value(array, s.v);
}

inline void appendDoubleValue(QByteArray& array, const double value)
{
    union switch_t {
        double d;
        uint64_t v;
    };
    switch_t s;
    s.d = value;
    appendInt64Value(array, s.v);
}

inline double doubleValue(const QByteArray& array, const int index = 0)
{
    if(static_cast<unsigned int>(index) > static_cast<unsigned int>(array.size()) // test to make sure we catch any overflow
    || static_cast<unsigned int>(index + sizeof(double)) > static_cast<unsigned int>(array.size())) {
        throw std::runtime_error("buffer too small for this doubleValue");
    }
    union switch_t {
        uint64_t    v;
        double      d;
    };
    switch_t s;
    s.v = (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 0))) << 56)
        | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 1))) << 48)
        | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 2))) << 40)
        | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 3))) << 32)
        | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 4))) << 24)
        | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 5))) << 16)
        | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 6))) << 8)
        | static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 7)));
    return s.d;
}

inline double doubleValueOrNull(const QByteArray& array, const int index = 0, const double default_value = 0)
{
    if(static_cast<unsigned int>(index + sizeof(double)) <= static_cast<unsigned int>(array.size())) {
        union switch_t {
            uint64_t    v;
            double      d;
        };
        switch_t s;
        s.v = (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 0))) << 56)
            | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 1))) << 48)
            | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 2))) << 40)
            | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 3))) << 32)
            | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 4))) << 24)
            | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 5))) << 16)
            | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 6))) << 8)
            | static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 7)));
        return s.d;
    }
    if(static_cast<unsigned int>(index) <= static_cast<unsigned int>(array.size())) {
        return default_value;
    }
    throw std::runtime_error("buffer too small for this doubleValueOrNull");
}

inline double safeDoubleValue(const QByteArray& array, const int index = 0, const double default_value = 0)
{
    if(static_cast<unsigned int>(index + sizeof(double)) <= static_cast<unsigned int>(array.size())) {
        union switch_t {
            uint64_t    v;
            double      d;
        };
        switch_t s;
        s.v = (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 0))) << 56)
            | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 1))) << 48)
            | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 2))) << 40)
            | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 3))) << 32)
            | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 4))) << 24)
            | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 5))) << 16)
            | (static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 6))) << 8)
            | static_cast<uint64_t>(static_cast<unsigned char>(array.at(index + 7)));
        return s.d;
    }
    return default_value;
}

// String
inline void setStringValue(QByteArray& array, const QString& value)
{
    const QByteArray str(value.toUtf8());
    checkBufferSize(str.size());

    array = str;
}

inline void appendStringValue(QByteArray& array, const QString& value)
{
    const QByteArray str(value.toUtf8());
    checkBufferSize(array.size() + str.size());

    array.append(str);
}

inline QString stringValue(const QByteArray& array, const int index = 0, int size = -1)
{
    if(size == -1) {
        size = array.size() - index;
    }
    if(static_cast<unsigned int>(index) > static_cast<unsigned int>(array.size())
    || static_cast<unsigned int>(size) > static_cast<unsigned int>(array.size())
    || static_cast<unsigned int>(index + size) > static_cast<unsigned int>(array.size())) {
        throw std::runtime_error("buffer too small for this stringValue");
    }
    return QString::fromUtf8(array.data() + index, size);
}

// Binary
inline void setBinaryValue(QByteArray& array, const QByteArray& value)
{
    checkBufferSize(value.size());

    array = value;
}

inline void appendBinaryValue(QByteArray& array, const QByteArray& value)
{
    checkBufferSize(array.size() + value.size());

    array.append(value);
}

inline QByteArray binaryValue(const QByteArray& array, const int index = 0, int size = -1)
{
    if(size == -1) {
        size = array.size() - index;
    }
    if(static_cast<unsigned int>(index) > static_cast<unsigned int>(array.size())
    || static_cast<unsigned int>(size) > static_cast<unsigned int>(array.size())
    || static_cast<unsigned int>(index + size) > static_cast<unsigned int>(array.size())) {
        throw std::runtime_error("buffer too small for this binaryValue() call");
    }
    return QByteArray(array.data() + index, size);
}







class QCassandraValue //: public QObject -- values are copyable and not named
{
public:
    static const int32_t TTL_PERMANENT = 0;

    // TTL must be positive, although Cassandra allows 0 as "permanent"
    typedef controlled_vars::limited_auto_init<int32_t, 0, INT_MAX, TTL_PERMANENT> cassandra_ttl_t;

    enum def_timestamp_mode_t {
        TIMESTAMP_MODE_CASSANDRA,
        TIMESTAMP_MODE_AUTO,
        TIMESTAMP_MODE_DEFINED
    };
    typedef controlled_vars::limited_auto_enum_init<def_timestamp_mode_t, TIMESTAMP_MODE_CASSANDRA, TIMESTAMP_MODE_DEFINED, TIMESTAMP_MODE_AUTO> timestamp_mode_t;

    // CASSANDRA_VALUE_TYPE_BINARY (empty buffer)
    QCassandraValue();

    // CASSANDRA_VALUE_TYPE_INTEGER
    QCassandraValue(bool value);
    QCassandraValue(char value);
    QCassandraValue(signed char value);
    QCassandraValue(unsigned char value);
    QCassandraValue(int16_t value);
    QCassandraValue(uint16_t value);
    QCassandraValue(int32_t value);
    QCassandraValue(uint32_t value);
    QCassandraValue(int64_t value);
    QCassandraValue(uint64_t value);

    // CASSANDRA_VALUE_TYPE_FLOAT
    QCassandraValue(float value);
    QCassandraValue(double value);

    // CASSANDRA_VALUE_TYPE_STRING
    QCassandraValue(const QString& value);

    // CASSANDRA_VALUE_TYPE_BINARY
    QCassandraValue(const QByteArray& value);
    QCassandraValue(const char *data, int size);

    void setNullValue();
    void setBoolValue(bool value);
    void setCharValue(char value);
    void setSignedCharValue(signed char value);
    void setUnsignedCharValue(unsigned char value);
    void setInt16Value(int16_t value);
    void setUInt16Value(uint16_t value);
    void setInt32Value(int32_t value);
    void setUInt32Value(uint32_t value);
    void setInt64Value(int64_t value);
    void setUInt64Value(uint64_t value);
    void setFloatValue(float value);
    void setDoubleValue(double value);
    void setStringValue(const QString& value);
    void setBinaryValue(const QByteArray& value);
    void setBinaryValue(const char *data, int data_size);

    int size() const;

    // whether size is zero
    bool nullValue() const;

    // bool
    bool boolValue      (int index = 0) const;
    bool boolValueOrNull(int index = 0, const bool default_value = false) const;
    bool safeBoolValue  (int index = 0, const bool default_value = false) const;

    // [[un]signed] char
    char          charValue              (int index = 0) const;
    char          charValueOrNull        (int index = 0, const char default_value = 0) const;
    char          safeCharValue          (int index = 0, const char default_value = 0) const;
    signed char   signedCharValue        (int index = 0) const;
    signed char   signedCharValueOrNull  (int index = 0, const signed char default_value = 0) const;
    signed char   safeSignedCharValue    (int index = 0, const signed char default_value = 0) const;
    unsigned char unsignedCharValue      (int index = 0) const;
    unsigned char unsignedCharValueOrNull(int index = 0, const unsigned char default_value = 0) const;
    unsigned char safeUnsignedCharValue  (int index = 0, const unsigned char default_value = 0) const;

    // [u]int16_t
    int16_t  int16Value       (int index = 0) const;
    int16_t  int16ValueOrNull (int index = 0, const int16_t default_value = 0) const;
    int16_t  safeInt16Value   (int index = 0, const int16_t default_value = 0) const;
    uint16_t uint16Value      (int index = 0) const;
    uint16_t uint16ValueOrNull(int index = 0, const uint16_t default_value = 0) const;
    uint16_t safeUInt16Value  (int index = 0, const uint16_t default_value = 0) const;

    // [u]int32_t
    int32_t  int32Value       (int index = 0) const;
    int32_t  int32ValueOrNull (int index = 0, const int32_t default_value = 0) const;
    int32_t  safeInt32Value   (int index = 0, const int32_t default_value = 0) const;
    uint32_t uint32Value      (int index = 0) const;
    uint32_t uint32ValueOrNull(int index = 0, const uint32_t default_value = 0) const;
    uint32_t safeUInt32Value  (int index = 0, const uint32_t default_value = 0) const;

    // [u]int64_t
    int64_t  int64Value       (int index = 0) const;
    int64_t  int64ValueOrNull (int index = 0, const int64_t default_value = 0) const;
    int64_t  safeInt64Value   (int index = 0, const int64_t default_value = 0) const;
    uint64_t uint64Value      (int index = 0) const;
    uint64_t uint64ValueOrNull(int index = 0, const uint64_t default_value = 0) const;
    uint64_t safeUInt64Value  (int index = 0, const uint64_t default_value = 0) const;

    // floating point
    float  floatValue       (int index = 0) const;
    float  floatValueOrNull (int index = 0, const float default_value = 0.0f) const;
    float  safeFloatValue   (int index = 0, const float default_value = 0.0f) const;
    double doubleValue      (int index = 0) const;
    double doubleValueOrNull(int index = 0, const double default_value = 0.0) const;
    double safeDoubleValue  (int index = 0, const double default_value = 0.0) const;

    // string / binary
    QString stringValue(int index = 0, int size = -1) const;
    const QByteArray& binaryValue() const;
    QByteArray binaryValue(int index, int size = -1) const;

    // There is no toString() because we do not know the type of your
    // cell (value) and therefore we have no means to convert it to
    // a string; you may try stringValue() instead?
    //QString toString() const;

    QCassandraValue& operator = (const char *null_value); // i.e. NULL
    QCassandraValue& operator = (bool value);
    QCassandraValue& operator = (char value);
    QCassandraValue& operator = (signed char value);
    QCassandraValue& operator = (unsigned char value);
    QCassandraValue& operator = (int16_t value);
    QCassandraValue& operator = (uint16_t value);
    QCassandraValue& operator = (int32_t value);
    QCassandraValue& operator = (uint32_t value);
    QCassandraValue& operator = (int64_t value);
    QCassandraValue& operator = (uint64_t value);
    QCassandraValue& operator = (float value);
    QCassandraValue& operator = (double value);
    QCassandraValue& operator = (const QString& value);
    QCassandraValue& operator = (const QByteArray& value);

    bool operator == (const QCassandraValue& rhs);
    bool operator != (const QCassandraValue& rhs);
    bool operator < (const QCassandraValue& rhs);
    bool operator <= (const QCassandraValue& rhs);
    bool operator > (const QCassandraValue& rhs);
    bool operator >= (const QCassandraValue& rhs);

    int32_t ttl() const;
    void setTtl(int32_t ttl = TTL_PERMANENT);

    consistency_level_t consistencyLevel() const;
    void setConsistencyLevel(consistency_level_t level);

    timestamp_mode_t timestampMode() const;
    void setTimestampMode(timestamp_mode_t mode);
    int64_t timestamp() const;
    void setTimestamp(int64_t timestamp);

private:
    void assignTimestamp(int64_t timestamp);

    // prevent share pointer assignments (i.e. output of
    // row->cell() instead of row->cell()->value())
    template<class T> QCassandraValue& operator = (std::shared_ptr<T>);

    friend class QCassandraPrivate;

    QByteArray                  f_value;
    cassandra_ttl_t             f_ttl;
    consistency_level_t         f_consistency_level;
    timestamp_mode_t            f_timestamp_mode;
    controlled_vars::zint64_t   f_timestamp;
};



} // namespace QtCassandra
#endif
//#ifndef QCASSANDRA_VALUE_H
// vim: ts=4 sw=4 et
