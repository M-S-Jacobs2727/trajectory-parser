#include <cstdint>
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
}