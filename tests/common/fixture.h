/****************************************************************************

    flow5 application
    Copyright (C) 2025 André Deperrois 
    
    This file is part of flow5.

    flow5 is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License,
    or (at your option) any later version.

    flow5 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with flow5.
    If not, see <https://www.gnu.org/licenses/>.


*****************************************************************************/


#pragma once

#include <string>
#include <vector>

class PlaneXfl;
class PlanePolar;

/**
 * A small plane model with computed results, built in code so that the tests do not depend on project files.
 * The analyses are inviscid, so that no foil polars are needed, and use the default triangular panel method.
 */
namespace fixture
{
    /** Creates the NACA foils and the plane with a main wing, an elevator and a fin, and inserts them in the object stores */
    PlaneXfl *makePlane(std::string const &planeName);

    /** Creates a T1 polar and runs it at the given angles of attack (degrees); the operating points are kept */
    PlanePolar *runT1Polar(PlaneXfl *pPlane, std::vector<double> const &alphas_deg);

    /** Creates a T7 stability polar and runs it at the control value 0; the operating point is kept */
    PlanePolar *runT7Polar(PlaneXfl *pPlane);
}
