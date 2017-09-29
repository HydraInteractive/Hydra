/**
* A* pathfinding
*
* License: Mozilla Public License Version 2.0 (https://www.mozilla.org/en-US/MPL/2.0/ OR See accompanying file LICENSE)
* Authors:
*  - Dan Printzell
*/

#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <hydra/pathing/SearchCell.hpp>


class PathFinding
{
public:
	PathFinding();
	~PathFinding();

	void findPath(glm::vec3 currentPos, glm::vec3 targetPos, int map[WORLD_SIZE][WORLD_SIZE]);
	glm::vec3 nextPathPos(glm::vec3 pos, float radius);
	void clearOpenList() { _openList.clear(); }
	void clearVisitedList() { _visitedList.clear(); }
	void clearPathToGoal() { _pathToEnd.clear(); }
	bool intializedStartGoal;
	bool foundGoal;
	std::vector<SearchCell*> _openList;
	std::vector<SearchCell*> _visitedList;
	std::vector<glm::vec3*> _pathToEnd;
private:
	void _setStartAndGoal(SearchCell start, SearchCell end);
	void _pathOpened(int x, int z, float newCost, SearchCell *parent, int map[WORLD_SIZE][WORLD_SIZE]);
	SearchCell *_getNextCell();
	void _continuePath(int map[WORLD_SIZE][WORLD_SIZE]);

	SearchCell *_startCell;
	SearchCell *_endCell;

};
