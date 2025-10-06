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
#include "types.h"

#define G_SERIALIZE(serializer, var) serializer.Serialize(var)
#define G_SERIALIZE_ARRAY(serializer, array, count) serializer.SerializeArray(array, count)

class StateSerializer
{
public:
    StateSerializer(std::ostream& stream) : m_output_stream(&stream), m_input_stream(nullptr), m_is_saving(true) {}
    StateSerializer(std::istream& stream) : m_output_stream(nullptr), m_input_stream(&stream), m_is_saving(false) {}

    inline bool IsSaving() const { return m_is_saving; }
    inline bool IsLoading() const { return !m_is_saving; }

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

    std::ostream* GetOutputStream() { return m_output_stream; }
    std::istream* GetInputStream() { return m_input_stream; }

private:
    std::ostream* m_output_stream;
    std::istream* m_input_stream;
    bool m_is_saving;
};

#endif /* STATE_SERIALIZER_H */