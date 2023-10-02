#include <fstream>
#include <optional>

#include "Frame.h"

namespace MDTraj
{
std::optional<Frame> readLmpTxtFrame(std::ifstream& file);
std::optional<Frame> readLmpBinFrame(std::ifstream& file);
}