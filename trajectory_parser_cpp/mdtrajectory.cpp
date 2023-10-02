#include <cstdint>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

namespace MDTraj
{
struct Frame
{
    int64_t timestep = -1L;
    int64_t natoms = -1L;
    double box[6] = {0.0};
    double tilt[3] = {0.0};
    std::vector<std::string> columns;
    std::vector<double> data;
};

bool isWhitespace(const std::string& str)
{
    if (str.empty())
        return true;
    for (const auto& c : str)
        if (!std::isspace(c))
            return false;
    return true;
}

std::optional<Frame> readTxtFrame(std::ifstream& file)
{
    std::string word;
    file >> word;
    if (word.empty())
        return {};

    if (word != "ITEM:")
    {
        std::cerr << "ERROR: Invalid start of frame. Expected 'ITEM:', found " << word << '\n';
        exit(1);
    }

    Frame frame;

    while (file)
    {
        file >> word;
        if (word == "TIMESTEP")
        {
            file >> frame.timestep;
        }
        else if (word == "TIME" || word == "UNITS")
        {
            file >> word;
        }
        else if (word == "BOX")
        {
            bool triclinic{ false };
            file >> word >> word;  // "BOX BOUNDS ab ab ab" or "BOX BOUNDS xy xz yz ab ab ab"
            if (word == "xy")
            {
                triclinic = true;
                file >> word >> word >> word;
            }
            file >> word >> word;

            for (size_t i = 0; i < 3; ++i)
            {
                file >> frame.box[2 * i] >> frame.box[2 * i + 1];
                if (triclinic)
                    file >> frame.tilt[i];
            }
        }
        else if (word == "NUMBER")
        {
            file >> word >> word;  // NUMBER OF ATOMS
            file >> frame.natoms;
        }
        else if (word == "ATOMS")
            break;
        else
        {
            std::cerr << "ERROR: Invalid section keyword: " << word << '\n';
            exit(1);
        }
    }

    if (!file)
    {
        std::cerr << "ERROR: Cannot read frame past '" << word << "'\n";
        exit(1);
    }

    if (frame.timestep == -1L)
    {
        std::cerr << "ERROR: Timestep not found in frame.\n";
        exit(1);
    }

    if (frame.natoms == -1L)
    {
        std::cerr << "ERROR: Number of atoms not found in frame.\n";
        exit(1);
    }

    for (size_t i = 0; i < 3; ++i)
    {
        if (box[2*i] >= box[2*i+1])
        {
            std::cerr << "ERROR: Lower box bound (" << box[2*i] << ") is larger than upper box bound (" << box[2*i+1] << ")\n"
            exit(1);
        }
    }

    std::string line;
    std::getline(file, line);
    std::istringstream iss{ line };
    while (!iss.eof())
    {
        iss >> word;
        if (isWhitespace(word))
            break;

        frame.columns.push_back(word);
    }
    if (frame.columns.size() < 2)
    {
        std::cerr << "ERROR: Only " << frame.columns.size() << " columns listed in frame:\n"
            << line << '\n';
        exit(1);
    }

    frame.data.resize(frame.natoms * frame.columns.size());
    for (auto& d : frame.data)
        file >> d;

    return frame;
}

std::optional<Frame> readBinFrame(std::ifstream& file)
{
    int64_t timestep = -1;
    file.read(reinterpret_cast<char*>(&timestep), sizeof timestep);

    Frame frame;
    int32_t endian = 1, revision = 1;
    if (timestep < 0)
    {
        int64_t magic_str_len = timestep;
        file.ignore(magic_str_len);
        file.read(reinterpret_cast<char*>(&endian), sizeof endian);
        file.read(reinterpret_cast<char*>(&revision), sizeof revision);
        file.read(reinterpret_cast<char*>(&timestep), sizeof timestep);
    }

    frame.timestep = timestep;

    int32_t triclinic = 0, ncols = 0;

    file.read(reinterpret_cast<char*>(&frame.natoms), sizeof(frame.natoms));
    file.read(reinterpret_cast<char*>(&triclinic), sizeof triclinic);
    file.read(reinterpret_cast<char*>(frame.box), sizeof(frame.box));

    if (triclinic)
        file.read(reinterpret_cast<char*>(frame.tilt), sizeof(frame.tilt));

    std::string columns;

    file.read(reinterpret_cast<char*>(&ncols), sizeof ncols);
    frame.columns.clear();
    frame.columns.reserve(ncols);

    if (revision > 1)
    {
        int32_t n = 0;
        file.read(reinterpret_cast<char*>(&n), sizeof n);
        if (n)
            file.ignore(n);  // units style

        char flag = 0;
        file.read(&flag, sizeof flag);
        if (flag)
            file.ignore(8);  // time
        
        file.read(reinterpret_cast<char*>(&n), sizeof n);
        columns.resize(n);
        if (n)
        {
            file.read(columns.data(), n);
            uint32_t idx = 0;
            for (int32_t i = 0; i < ncols; ++i)
            {
                auto new_idx = columns.find(' ', idx);
                frame.columns.push_back(columns.substr(idx, new_idx - idx));
            }
        }
    }

    int32_t nchunks = 0;
    file.read(reinterpret_cast<char*>(&nchunks), sizeof nchunks);

    frame.data.reserve(frame.natoms * ncols);

    for (int32_t i = 0; i < nchunks; ++i)
    {
        int32_t n = 0;
        file.read(reinterpret_cast<char*>(&n), sizeof n);

        auto back_ptr = &frame.data.back() + sizeof(double);
        frame.data.resize(frame.data.size() + n);
        file.read(reinterpret_cast<char*>(back_ptr), sizeof n);
    }

    return frame;
}
}
