#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "Frame.h"

namespace LmpDump
{
class BinaryDumpfileParser
{
public:
    BinaryDumpfileParser(const std::string&);
    void scan(const int64_t frame_idx = std::numeric_limits<int64_t>::max());
    std::optional<Frame> load(const int64_t);
    std::optional<Frame> load_timestep(const int64_t);

private:
    std::optional<Frame> read_frame();
    void skip_frame();

private:
    std::ifstream m_file;
    std::vector<std::ifstream::pos_type> m_positions;
    bool m_file_complete = false;
    std::vector<int64_t> m_timesteps;
};
}