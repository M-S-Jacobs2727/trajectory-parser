#include "TextDumpfileParser.h"

#include <algorithm>
#include <sstream>

namespace LmpDump
{
TextDumpfileParser::TextDumpfileParser(const std::string& filename)
{
    m_file.open(filename, std::ios::in);
    if (m_file.fail())
        throw 1;
}

void TextDumpfileParser::scan(const int64_t target_frame_idx)
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

std::optional<Frame> TextDumpfileParser::load(const int64_t frame_idx)
{
    if (frame_idx < m_positions.size())
        m_file.seekg(m_positions[frame_idx]);
    else
        scan(frame_idx);

    return read_frame();
}

std::optional<Frame> TextDumpfileParser::load_timestep(const int64_t target_timestep)
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

int64_t TextDumpfileParser::read_timestep()
{
    while (true)
    {
        std::string word;
        m_file >> word;
        if (word != "ITEM:")
            throw 4;

        m_file >> word;
        if (word == "TIMESTEP")
        {
            int64_t timestep;
            m_file >> timestep;
            return timestep;
        }
        else if (word == "ATOMS")
        {
            throw 5;
        }
        else if (word == "UNITS") ;
    }
}

std::optional<Frame> TextDumpfileParser::read_frame()
{
    Frame frame;

    std::string word;
    m_file >> word;
    auto bad_case = word != "ITEM:";

    if (bad_case && (m_file.eof() || isWhitespace(word)))
    {
        m_file_complete = true;
        return {};
    }

    if (bad_case)
        throw 1;
    
    while (m_file.good())
    {
        m_file >> word;
        if (word == "TIMESTEP")
        {
            m_file >> frame.timestep;
        }
        else if (word == "TIME" || word == "UNITS")
        {
            m_file >> word;
        }
        else if (word == "BOX")
        {
            bool triclinic{ false };
            m_file >> word >> word;
            if (word == "xy")
            {
                triclinic = true;
                m_file >> word >> word >> word;
            }
            m_file >> word >> word;
            for (size_t i = 0; i < 3; ++i)
                m_file >> frame.box[2 * i] >> frame.box[2 * i + 1] >> frame.tilt[i];
        }
        else if (word == "NUMBER")
        {
            m_file.ignore(1024, '\n');
            m_file >> frame.natoms;
        }
        else if (word == "ATOMS")
        {
            break;
        }
        else
            throw 2;
    }

    std::getline(m_file, word);
    std::istringstream iss{word};
    while (!iss.eof())
    {
        iss >> word;
        if (!isWhitespace(word))
            frame.columns.push_back(word);
    }

    frame.data.resize(frame.natoms * frame.columns.size());
    for (auto& d : frame.data)
        m_file >> d;

    return frame;
}

void TextDumpfileParser::skip_frame()
{
    std::string word;
    m_file >> word;

    auto bad_case = word != "ITEM:";

    if (bad_case && (m_file.eof() || isWhitespace(word)))
    {
        m_file_complete = true;
        return;
    }

    if (bad_case)
        throw 1;

    uint64_t natoms = 0;

    while (m_file.good())
    {
        m_file >> word;
        if (word == "TIME" || word == "UNITS" || word == "TIMESTEP")
        {
            m_file >> word;
        }
        else if (word == "BOX")
        {
            std::getline(m_file, word);
            std::getline(m_file, word);
            std::getline(m_file, word);
            std::getline(m_file, word);
        }
        else if (word == "NUMBER")
        {
            std::getline(m_file, word);
            m_file >> natoms;
        }
        else if (word == "ATOMS")
        {
            break;
        }
        else
            throw 2;
    }

    std::getline(m_file, word);

    for (uint64_t i = 0; i < natoms; ++i)
        m_file.ignore(1024, '\n');

}

bool TextDumpfileParser::isWhitespace(const std::string& str)
{
    return str.empty() || std::all_of(
        str.begin(), str.end(), [](char c) {
            return std::isspace(c);
        }
    );
}
}

