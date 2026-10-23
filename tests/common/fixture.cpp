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



#include "fixture.h"

#include <api/foil.h>
#include <api/objects2d.h>
#include <api/objects3d.h>
#include <api/planepolar.h>
#include <api/planetask.h>
#include <api/planexfl.h>
#include <api/wingxfl.h>


namespace
{
    constexpr int FOILPANELS = 100;   /**< number of panels of the NACA foils; coarse, the tests do not check accuracy */

    Foil *makeFoil(int digits, std::string const &name)
    {
        Foil *pFoil = Objects2d::foil(name);
        if(pFoil) return pFoil;
        pFoil = new Foil;
        Objects2d::makeNacaFoil(pFoil, digits, FOILPANELS);
        pFoil->setName(name);
        Objects2d::insertThisFoil(pFoil);
        return pFoil;
    }

    void setFoils(WingXfl *pWing, std::string const &foilName)
    {
        for(int isec=0; isec<pWing->nSections(); isec++)
        {
            pWing->section(isec).setLeftFoilName(foilName);
            pWing->section(isec).setRightFoilName(foilName);
        }
    }

    void runTask(PlaneXfl *pPlane, PlanePolar *pPolar, std::vector<double> const &opps)
    {
        PlaneTask task;
        task.setKeepOpps(true);  // the operating points are moved to the object store
        task.setObjects(pPlane, pPolar);
        if(pPolar->isType7()) task.setStabOppList(opps); // control values
        else                  task.setOppList(opps);     // angles of attack, degrees
        task.run();
    }
}


PlaneXfl *fixture::makePlane(std::string const &planeName)
{
    Foil const *pMainFoil = makeFoil(2412, "NACA 2412");
    Foil const *pTailFoil = makeFoil(9,    "NACA 0009");

    PlaneXfl *pPlane = new PlaneXfl;
    pPlane->setName(planeName);

    // positions and masses as in API_examples/PlaneRun1, so that the plane is statically stable and trims
    WingXfl *pWing = pPlane->addWing();
    pWing->makeDefaultWing();
    setFoils(pWing, pMainFoil->name());
    pWing->inertia().setStructuralMass(0.55);
    pPlane->setWingPosition(pWing, {0.0, 0.0, 0.0});

    WingXfl *pElev = pPlane->addWing();
    pElev->makeDefaultStab();
    setFoils(pElev, pTailFoil->name());
    pElev->inertia().setStructuralMass(0.05);
    pPlane->setWingPosition(pElev, {0.87, 0.0, 0.21});
    pPlane->setRyAngle(pElev, -2.5);

    WingXfl *pFin = pPlane->addWing();
    pFin->makeDefaultFin();
    setFoils(pFin, pTailFoil->name());
    pFin->inertia().setStructuralMass(0.035);
    pPlane->setWingPosition(pFin, {0.83, 0.0, 0.01});
    pPlane->setRxAngle(pFin, -90.0);
    pFin->setClosedInnerSide(true);

    Inertia &inertia = pPlane->inertia();
    inertia.appendPointMass(0.30, {-0.35, 0, 0}, "Nose lead");
    inertia.appendPointMass(0.20, {-0.25, 0, 0}, "Battery");
    inertia.appendPointMass(0.50, { 0.30, 0, 0}, "Fuse");

    pPlane->makePlane(false, false, true);
    Objects3d::insertPlane(pPlane);
    return pPlane;
}


static PlanePolar *makePolar(PlaneXfl *pPlane, xfl::enumPolarType type, std::string const &name)
{
    PlanePolar *pPolar = new PlanePolar;
    pPolar->setDefaultSpec(pPlane);
    pPolar->setPlaneName(pPlane->name());
    pPolar->setName(name);
    pPolar->setType(type);
    pPolar->setViscous(false);
    pPolar->setReferenceChordLength(pPlane->mac());
    pPolar->resizeFlapCtrls(pPlane);
    Objects3d::insertPlPolar(pPolar);
    return pPolar;
}


PlanePolar *fixture::runT1Polar(PlaneXfl *pPlane, std::vector<double> const &alphas_deg)
{
    PlanePolar *pPolar = makePolar(pPlane, xfl::T1POLAR, "T1 polar");
    pPolar->setVelocity(10.0); // m/s
    runTask(pPlane, pPolar, alphas_deg);
    return pPolar;
}


PlanePolar *fixture::runT7Polar(PlaneXfl *pPlane, std::vector<double> const &ctrls)
{
    PlanePolar *pPolar = makePolar(pPlane, xfl::T7POLAR, "T7 polar");
    runTask(pPlane, pPolar, ctrls);
    return pPolar;
}
