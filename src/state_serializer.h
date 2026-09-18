/*
 * Gearlynx - Lynx Emulator
 * Copyright (C) 2025  Ignacio Sanchez

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/
 *
 */

#ifndef STATE_SERIALIZER_H
#define STATE_SERIALIZER_H

#include <iostream>
#include <limits>
#include <string>
#include <vector>
#include "types.h"

#define G_SERIALIZE(serializer, var) serializer.Serialize(var)
#define G_SERIALIZE_ARRAY(serializer, array, count) serializer.SerializeArray(array, count)

class StateSerializer
{
public:
    StateSerializer(std::ostream& stream) : m_output_stream(&stream), m_input_stream(NULL), m_is_saving(true) {}
    StateSerializer(std::istream& stream) : m_output_stream(NULL), m_input_stream(&stream), m_is_saving(false) {}

    inline bool IsSaving() const { return m_is_saving; }
    inline bool IsLoading() const { return !m_is_saving; }
    inline bool IsValid() const { return m_is_saving ? m_output_stream->good() : m_input_stream->good(); }

    // Serialize a single variable
    template<typename T>
    void Serialize(T& value)
    {
        if (m_is_saving)
            m_output_stream->write(reinterpret_cast<const char*>(&value), sizeof(T));
        else
            m_input_stream->read(reinterpret_cast<char*>(&value), sizeof(T));
    }

    // Serialize an array
    template<typename T>
    void SerializeArray(T* array, size_t count)
    {
        if (m_is_saving)
            m_output_stream->write(reinterpret_cast<const char*>(array), sizeof(T) * count);
        else
            m_input_stream->read(reinterpret_cast<char*>(array), sizeof(T) * count);
    }

    void SerializeString(std::string& value, size_t max_size)
    {
        u32 size = (u32)value.size();
        Serialize(size);

        if (IsLoading())
        {
            if (!CheckReadSize(size, max_size, 1))
                return;
            value.resize(size);
        }
        if (size > 0)
            SerializeArray(&value[0], size);
    }

    template<typename T>
    void SerializeVector(std::vector<T>& value, size_t max_size)
    {
        u32 size = (u32)value.size();
        Serialize(size);

        if (IsLoading())
        {
            if (!CheckReadSize(size, max_size, sizeof(T)))
                return;
            value.resize(size);
        }
        if (size > 0)
            SerializeArray(&value[0], size);
    }

    std::ostream* GetOutputStream() { return m_output_stream; }
    std::istream* GetInputStream() { return m_input_stream; }

private:
    bool CheckReadSize(size_t count, size_t max_count, size_t element_size)
    {
        if (!m_input_stream->good())
            return false;
        if (count > max_count || count > std::numeric_limits<size_t>::max() / element_size ||
            count > (u64)std::numeric_limits<std::streamsize>::max() / element_size)
        {
            m_input_stream->setstate(std::ios::failbit);
            return false;
        }
        if (count == 0)
            return true;

        std::streampos position = m_input_stream->tellg();
        m_input_stream->seekg(0, std::ios::end);
        std::streampos end = m_input_stream->tellg();
        if (position < 0 || end < position || count > (u64)(end - position) / element_size)
        {
            m_input_stream->setstate(std::ios::failbit);
            return false;
        }
        m_input_stream->seekg(position);
        return m_input_stream->good();
    }

    std::ostream* m_output_stream;
    std::istream* m_input_stream;
    bool m_is_saving;
};

#endif /* STATE_SERIALIZER_H */