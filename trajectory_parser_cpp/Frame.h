#include <cstdint>
#include <string>
#include <vector>

namespace LmpDump
{
struct Frame
{
    int64_t timestep = 0;
    int64_t natoms = 0;
    double box[6] = {0.0};
    double tilt[3] = {0.0};
    std::vector<double> data;
    std::vector<std::string> columns;
};
}
