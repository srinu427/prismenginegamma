#pragma once

#include <string>
#include <vector>

struct VecGraphNode {
	std::string _id;
	std::vector<VecGraphNode*> _out_nodes;
	std::vector<VecGraphNode*> _in_nodes;
};
