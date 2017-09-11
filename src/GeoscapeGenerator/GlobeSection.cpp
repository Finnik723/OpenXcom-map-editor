/*
 * Copyright 2010-2017 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "GlobeSection.h"
#include <algorithm>
#include "GeoscapeGenerator.h"

namespace OpenXcom
{

GlobeSection::GlobeSection(GeoscapeGenerator *parent) : _parent(parent), _heightIndex(0)
{
	_greatCircles.clear();
	_intersections.clear();
}

// Cleans up the GlobeSection
GlobeSection::~GlobeSection()
{
	// delete variables to clean up here
}

// Gets a pointer to the map of great circles
std::map<size_t, int> *GlobeSection::getGreatCircles()
{
	return &_greatCircles;
}

// Gets a pointer to the list of intersection references
std::vector<GreatCircleIntersection*> *GlobeSection::getIntersections()
{
	return &_intersections;
}

// Gets the heightIndex of the section
int GlobeSection::getHeightIndex()
{
	return _heightIndex;
}

// Sets the heightIndex of the section
void GlobeSection::setHeightIndex(int heightIndex)
{
	_heightIndex = heightIndex;
	return;
}

/** Intersects a great circle with this globe section
 * Adds intersections and circle references as necessary, splits the section into
 * two and creates a new one if the circle intersects. Otherwise just changes the
 * height index
 * @param circleIndex The index of the great circle
 */
void GlobeSection::intersectWithGreatCircle(size_t circleIndex)
{
	// Test whether the intersection with each of the great circles that form
	// the boundaries of this section fall on the perimeter of the section
	size_t intersectionIndex[2];
	size_t intersectionCounter = 0;
	GreatCircleIntersection *intersections[2];

	for (std::map<size_t, int>::iterator i = _greatCircles.begin(); i != _greatCircles.end(); ++i)
	{
		// Get the intersection points
		_parent->intersectGreatCircles((*i).first, circleIndex, &intersectionIndex[0], &intersectionIndex[1]);

		// Special case for first few sections: no intersections exist, just add the new ones
		size_t tempCircles[2];
		size_t tempIndex = 0;

		if (_intersections.size() == 0)
		{
			tempCircles[0] = tempCircles[1] = 0;
			intersectionCounter = 2;
			intersections[0] = &_parent->getIntersections()->at(intersectionIndex[0]);
			intersections[1] = &_parent->getIntersections()->at(intersectionIndex[1]);
			break;
		}

		// Find which other pair of great circles intersect the same boundary circle as the one just intersected
		for (std::vector<GreatCircleIntersection*>::iterator j = _intersections.begin(); j != _intersections.end(); ++j)
		{
			if ((*j)->circles.first == (*i).first)
			{
				tempCircles[tempIndex] = (*j)->circles.second;
				tempIndex++;
			}

			if ((*j)->circles.second == (*i).first)
			{
				tempCircles[tempIndex] = (*j)->circles.first;
				tempIndex++;
			}

			if (tempIndex == 2)
			{
				break;
			}
		}

		bool intersectionOnPerimeter[2] = {false, false};
		// Test whether the intersection points are properly above/below the other great circles according to the parity of the circle
		for (int j = 0; j < 2; ++j) // Loop over the two intersections
		{
			for (int k = 0; k < 2; ++k) // Loop over the two other great circles
			{
				double latitude = _parent->getIntersections()->at(intersectionIndex[j]).coordinates.first;
				double longitude = _parent->getIntersections()->at(intersectionIndex[j]).coordinates.second;
				_parent->rotatePointOnSphere(tempCircles[k], &latitude, &longitude);
				int parity = _greatCircles.find(tempCircles[k])->second;

				intersectionOnPerimeter[k] = (latitude * parity > 0);
			}

			if (intersectionOnPerimeter[0] && intersectionOnPerimeter[1])
			{
				intersections[intersectionCounter] = &_parent->getIntersections()->at(intersectionIndex[j]);
				intersectionCounter++;
				break;
			}
		}

		// The new great circle should intersect the globe section at only 2 points
		if (intersectionCounter == 2)
			break;
	}

	// Create a new globe section if the new circle intersects this globe section
	if (intersectionCounter)
	{
		std::map<size_t, int> newCircles, oldCircles;
		std::vector<GreatCircleIntersection*> newIntersections, oldIntersections;

		// Iterate over other intersections and add them to either new or this section
		for (std::vector<GreatCircleIntersection*>::const_iterator i = _intersections.begin(); i != _intersections.end(); ++i)
		{
			if ((*i) == intersections[0] || (*i) == intersections[1])
				continue;

			double latitude = (*i)->coordinates.first;
			double longitude = (*i)->coordinates.second;
			_parent->rotatePointOnSphere(circleIndex, &latitude, &longitude);

			// Assume this intersection is the one above the new circle, the new one is below
			if (latitude > 0)
			{
				oldIntersections.push_back((*i));
			}
			else
			{
				newIntersections.push_back((*i));
			}
		}

		// Add intersections on new great circle to both new sections
		for (size_t i = 0; i < 2; i++)
		{
			newIntersections.push_back(intersections[i]);
			oldIntersections.push_back(intersections[i]);
		}

		// Loop through the intersections, adding the great circles to the new or old globe sections according to the intersection data
		for (std::vector<GreatCircleIntersection*>::iterator i = newIntersections.begin(); i != newIntersections.end(); ++i)
		{
			std::map<size_t, int>::iterator it = _greatCircles.find((*i)->circles.first);
			if (it != _greatCircles.end())
				newCircles.insert(std::make_pair((*i)->circles.first, (*it).second)); // insert skips repeat keys

			it = _greatCircles.find((*i)->circles.second);
			if (it != _greatCircles.end())
				newCircles.insert(std::make_pair((*i)->circles.second, (*it).second));
		}

		for (std::vector<GreatCircleIntersection*>::iterator i = oldIntersections.begin(); i != oldIntersections.end(); ++i)
		{
			std::map<size_t, int>::iterator it = _greatCircles.find((*i)->circles.first);
			if (it != _greatCircles.end())
				oldCircles.insert(std::make_pair((*i)->circles.first, (*it).second));

			it = _greatCircles.find((*i)->circles.second);
			if (it != _greatCircles.end())
				oldCircles.insert(std::make_pair((*i)->circles.second, (*it).second));
		}

		// Add new circle to both sections
		newCircles[circleIndex] = -1;
		oldCircles[circleIndex] = 1;

		// Create new section and set the appropriate data to both sections
		GlobeSection *newSection = new GlobeSection(_parent);
		newSection->getGreatCircles()->insert(newCircles.begin(), newCircles.end());
		newSection->getIntersections()->assign(newIntersections.begin(), newIntersections.end());
		newSection->setHeightIndex(_heightIndex - 1);
		_parent->getGlobeSections()->push_back(newSection);

		_greatCircles.clear();
		_intersections.clear();
		_greatCircles = oldCircles;
		_intersections = oldIntersections;
		_heightIndex++;
	}
	else // Change the height of this section according to being above or below the new circle
	{
		double latitude = (*_intersections.begin())->coordinates.first;
		double longitude = (*_intersections.begin())->coordinates.second;
		_parent->rotatePointOnSphere(circleIndex, &latitude, &longitude);

		if (latitude > 0)
		{
			_heightIndex++;
		}
		else
		{
			_heightIndex--;
		}
	}
}

}
