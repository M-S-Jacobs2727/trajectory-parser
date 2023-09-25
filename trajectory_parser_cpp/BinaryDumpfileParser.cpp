#include "BinaryDumpfileParser.h"

#include <algorithm>

namespace LmpDump
{
BinaryDumpfileParser::BinaryDumpfileParser(const std::string& filename)
{
    m_file.open(filename, std::ios::binary | std::ios::in);
    if (m_file.fail())
        throw 1;
}

void BinaryDumpfileParser::scan(const int64_t target_frame_idx)
{
    if (m_file_complete)
        return; 

    uint64_t frame_idx = m_positions.size();
    m_file.seekg(m_positions.back());
    
    while (!m_file.eof() && frame_idx < target_frame_idx)
    {
        skip_frame();
        m_positions.push_back(m_file.tellg());
        ++frame_idx;
    }

    if (m_file.eof())
    {
        m_positions.pop_back();
        m_file_complete = true;
    }
}

std::optional<Frame> BinaryDumpfileParser::load(const int64_t frame_idx)
{
    if (frame_idx < m_positions.size())
        m_file.seekg(m_positions[frame_idx]);
    else
        scan(frame_idx);

    return read_frame();
}

std::optional<Frame> BinaryDumpfileParser::load_timestep(const int64_t target_timestep)
{
    if (m_timesteps.size())
    {
        auto it = std::find(m_timesteps.begin(), m_timesteps.end(), target_timestep);
        if (it < m_timesteps.end())
        {
            if (*it != target_timestep)
                throw 2;
            return load(static_cast<int64_t>(it - m_timesteps.begin()));
        }
        if (m_file_complete)
            throw 3;
    }

    m_file.seekg(m_positions.back());
    while (true)
    {
        int64_t timestep = 0;
        m_file.read(reinterpret_cast<char*>(&timestep), sizeof timestep);
        if (m_file.eof() || timestep > target_timestep)
            throw 4;

        m_file.seekg(m_positions.back());
        if (timestep == target_timestep)
            return read_frame();
        
        skip_frame();
        m_positions.push_back(m_file.tellg());
    }
}

std::optional<Frame> BinaryDumpfileParser::read_frame()
{
    Frame frame;

    int64_t timestep = 0;
    m_file.read(reinterpret_cast<char*>(&timestep), sizeof timestep);

    if (m_file.eof())
    {
        m_file_complete = true;
        return {};
    }

    int32_t endian = 1, revision = 1;
    if (timestep < 0)
    {
        int64_t magic_str_len = timestep;
        m_file.ignore(magic_str_len);
        m_file.read(reinterpret_cast<char*>(&endian), sizeof endian);
        m_file.read(reinterpret_cast<char*>(&revision), sizeof revision);
        m_file.read(reinterpret_cast<char*>(&timestep), sizeof timestep);
    }

    frame.timestep = timestep;

    int32_t triclinic = 0, ncols = 0;

    std::string columns;

    m_file.read(reinterpret_cast<char*>(&frame.natoms), sizeof(frame.natoms));
    m_file.read(reinterpret_cast<char*>(&triclinic), sizeof triclinic);
    m_file.read(reinterpret_cast<char*>(frame.box), sizeof(frame.box));

    if (triclinic)
        m_file.read(reinterpret_cast<char*>(frame.tilt), sizeof(frame.tilt));

    m_file.read(reinterpret_cast<char*>(&ncols), sizeof ncols);
    frame.columns.clear();
    frame.columns.reserve(ncols);

    if (revision > 1)
    {
        int32_t n = 0;
        m_file.read(reinterpret_cast<char*>(&n), sizeof n);
        if (n)
            m_file.ignore(n);  // units style

        char flag = 0;
        m_file.read(&flag, sizeof flag);
        if (flag)
            m_file.ignore(8);  // time
        
        m_file.read(reinterpret_cast<char*>(&n), sizeof n);
        columns.resize(n);
        if (n)
        {
            m_file.read(columns.data(), n);
            uint32_t idx = 0;
            for (int32_t i = 0; i < ncols; ++i)
            {
                auto new_idx = columns.find(' ', idx);
                frame.columns.push_back(columns.substr(idx, new_idx - idx));
            }
        }
    }

    int32_t nchunks = 0;
    m_file.read(reinterpret_cast<char*>(&nchunks), sizeof nchunks);

    frame.data.reserve(frame.natoms * ncols);

    for (int32_t i = 0; i < nchunks; ++i)
    {
        int32_t n = 0;
        m_file.read(reinterpret_cast<char*>(&n), sizeof n);

        auto back_ptr = &frame.data.back() + sizeof(double);
        frame.data.resize(frame.data.size() + n);
        m_file.read(reinterpret_cast<char*>(back_ptr), sizeof n);
    }

    return frame;
}

void BinaryDumpfileParser::skip_frame()
{
    int64_t timestep = 0;
    m_file.read(reinterpret_cast<char*>(&timestep), sizeof timestep);

    if (m_file.eof())
    {
        m_file_complete = true;
        return;
    }

    int32_t revision = 1;
    if (timestep < 0)
    {
        m_file.ignore(4 - timestep);
        m_file.read(reinterpret_cast<char*>(&revision), sizeof revision);
        m_file.ignore(8);
    }

    int32_t n = 0;

    m_file.ignore(8);
    m_file.read(reinterpret_cast<char*>(&n), sizeof n);
    m_file.ignore(48);

    if (n)
        m_file.ignore(24);

    m_file.read(reinterpret_cast<char*>(&ncols), sizeof ncols);

    if (revision > 1)
    {
        m_file.read(reinterpret_cast<char*>(&n), sizeof n);
        if (n)
            m_file.ignore(n);  // units style

        char flag = 0;
        m_file.read(&flag, 1);
        if (flag)
            m_file.ignore(8);  // time

        m_file.read(reinterpret_cast<char*>(&n), sizeof n);
        if (n)
            m_file.ignore(n);
    }

    m_file.read(reinterpret_cast<char*>(&n), sizeof n);

    for (int32_t i = 0; i < n; ++i)
    {
        int32_t n2 = 0;
        m_file.read(reinterpret_cast<char*>(&n2), sizeof n2);
        m_file.ignore(n2);
    }
}
}

