#include "../readLmp.h"

#include <fstream>

#ifndef PREFIXPATH
#define PREFIXPATH "./test_dumps"
#endif

const double BOX_UPPER_BOUND = 16.795961913825074

int main()
{
    std::string filename{ PREFIXPATH };
    filename.append("dump.0.txt");
    std::ifstream infile{ filename };
    auto frame = readLmpTxtFrame(infile);
    if (!frame)
    {
        std::cerr << "Optional frame is empty.\n";
        return 1;
    }

    if (frame.natoms != 4000)
    {
        std::cerr << "Number of atoms should be 4000, found " << frame.natoms << '\n';
        return 2;
    }

    if (frame.timestep != 0)
    {
        std::cerr << "Timestep should be 0, found " << frame.timestep << '\n';
        return 2;
    }

    for (int i = 0; i < 3; ++i)
    {
        if (frame.box[2*i] != 0.0)
        {
            std::cerr << "Lower box boundary " << i << " should be 0.0, found " << frame.box[2*i] << '\n';
            return 2;
        }
        if (frame.box[2*i+1] != BOX_UPPER_BOUND)
        {
            std::cerr << "Upper box boundary " << i << " should be " << BOX_UPPER_BOUND << ", found " << frame.box[2*i] << '\n';
            return 2;
        }
        if (frame.tilt[i] != 0.0)
        {
            std::cerr << "Box tilt factor " << i << " should be 0.0, found " << frame.tilt[i] << '\n';
            return 2;
        }
    }

    std::vector<std::string> expected_cols{ "id", "type", "x", "y", "z", "xu", "yu", "zu", "vx", "vy", "vz" };
    int ncols = frame.columns.size();
    if (ncols != expected_cols.size())
    {
        std::cerr << "Number of columns should be " << expected_cols.size() << ", found " << ncols << '\n';
        return 2;
    }
    for (int i = 0; i < expected_cols.size(); ++i)
    {
        if (expected_cols[i] != frame.columns[i])
        {
            std::cerr << "Column " << i << " should be " << expected_cols[i] << ", found " << frame.columns[i] << '\n';
            return 2;
        }
    }

    if (frame.natoms * ncols != frame.data.size())
    {
        std::cerr << "Total number of elements in data array should be " << frame.natoms * frame.columns.size()
                << ", found " << frame.data.size() << '\n';
        return 2;
    }

    for (int i = 0; i < frame.natoms; ++i)
    {
        if (frame.data[ncols*i + 1] != 1.0)
        {
            std::cerr << "All atom types should be 1, but atom " << frame.data[ncols*i] << " has type " << frame.data[ncols*i + 1] << '\n';
            return 2;
        }
        for (int j = 0; j < 3; ++j)
        {
            double wrapped_coord = frame.data[ncols*i + 2 + j];
            double unwrapped_coord = frame.data[ncols*i + 5 + j];
            if (wrapped_coord < 0 || BOX_UPPER_BOUND < wrapped_coord)
            {
                std::cerr << "All wrapped coords should be inside box, but atom " << frame.data[ncols*i] << " has wrapped coords (" 
                    << frame.data[ncols*i + 2] << ", " << frame.data[ncols*i + 3] << ", " << frame.data[ncols*i + 4] << ")\n";
                return 2;
            }
            if (unwrapped_coord < 0 || BOX_UPPER_BOUND < unwrapped_coord)
            {
                std::cerr << "All unwrapped coords should be inside box (at timestep 0), but atom " << frame.data[ncols*i] << " has unwrapped coords (" 
                    << frame.data[ncols*i + 5] << ", " << frame.data[ncols*i + 6] << ", " << frame.data[ncols*i + 7] << ")\n";
                return 2;
            }
            if (wrapped_coord != unwrapped_coord)
            {
                std::cerr << "At timestep 0, wrapped and unwrapped coords should be identical, but atom " << frame.data[ncols*i] << " has wrapped coord "
                    << frame.data[ncols*i + 2] << ", " << frame.data[ncols*i + 3] << ", " << frame.data[ncols*i + 4] << "), and unwrapped coord "
                    << frame.data[ncols*i + 5] << ", " << frame.data[ncols*i + 6] << ", " << frame.data[ncols*i + 7] << ")\n";
                    return 2;
            }
            double vel_component = frame.data[ncols*i + 8 + j];
            if (vel_component < -10.0 || vel_component > 10.0)
            {
                std::cerr << "Absolute value of velociy components should be all less than 10.0, but atom " << frame.data[ncols*i] << " has velocity "
                    << frame.data[ncols*i + 8] << ", " << frame.data[ncols*i + 9] << ", " << frame.data[ncols*i + 10] << ")\n";
                return 2;
            }
        }
    }
    
    return 0;
}
