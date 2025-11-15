#include "pottery_geometric_validator_simple.h"

// Static member definitions
std::map<int, Vector3d> SimplePotteryValidator::piece_positions;
std::map<int, Vector3d> SimplePotteryValidator::piece_axes;
bool SimplePotteryValidator::data_loaded = false;