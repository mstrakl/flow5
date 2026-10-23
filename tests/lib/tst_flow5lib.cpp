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



/**
 * Headless tests of flow5-lib and flow5-io-lib: the object stores, the consistency of polars and
 * operating points, and the .fl5 project round trip.
 */

#include <cmath>
#include <filesystem>

#include <QTest>
#include <QTemporaryDir>

#include <api/api.h>
#include <api/flow5-io.h>
#include <api/objects3d.h>
#include <api/planeopp.h>
#include <api/planepolar.h>
#include <api/planexfl.h>

#include "../common/fixture.h"


namespace
{
    constexpr double ROUNDTRIPTOLERANCE = 1.e-9;  /**< values are stored as doubles in the .fl5 file, so the round trip is exact up to rounding */
    std::vector<double> const ALPHAS_DEG = {-2.0, 2.0, 6.0};

    int countPolarsOf(std::string const &planeName)
    {
        int n = 0;
        for(int i=0; i<Objects3d::nPolars(); i++)
            if(Objects3d::planePolarAt(i)->planeName()==planeName) n++;
        return n;
    }

    int countPOppsOf(PlanePolar const *pPolar)
    {
        int n = 0;
        for(int i=0; i<Objects3d::nPOpps(); i++)
            if(pPolar->hasPOpp(Objects3d::POppAt(i))) n++;
        return n;
    }
}


class TestFlow5Lib : public QObject
{
    Q_OBJECT

    private slots:
        void cleanup() {globals::deleteObjects();}  // the object stores are global

        void t1PolarHasOnePointPerOperatingPoint();
        void t7PolarComputesTheModes();
        void deletePlanePolarDeletesItsOperatingPoints();
        void deletePlaneResultsWithPolars();
        void deletePlaneLeavesNoOrphanPolars();
        void projectRoundTrip();
        void analysisIsDeterministic();
};


void TestFlow5Lib::t1PolarHasOnePointPerOperatingPoint()
{
    PlaneXfl *pPlane = fixture::makePlane("Test plane");
    PlanePolar *pPolar = fixture::runT1Polar(pPlane, ALPHAS_DEG);

    QCOMPARE(countPOppsOf(pPolar), int(ALPHAS_DEG.size()));
    QCOMPARE(pPolar->dataSize(), int(ALPHAS_DEG.size()));

    for(int i=0; i<pPolar->dataSize(); i++)
    {
        QVERIFY2(std::isfinite(pPolar->m_AF.at(i).CL()), "CL is not finite");
        QCOMPARE(pPolar->m_Alpha.at(i), ALPHAS_DEG.at(i));
        if(i>0) QVERIFY2(pPolar->m_AF.at(i).CL()>pPolar->m_AF.at(i-1).CL(), "CL does not increase with alpha below stall");
    }
}


void TestFlow5Lib::t7PolarComputesTheModes()
{
    PlaneXfl *pPlane = fixture::makePlane("Test plane");
    PlanePolar *pPolar = fixture::runT7Polar(pPlane);

    QCOMPARE(countPOppsOf(pPolar), 1);
    PlaneOpp const *pPOpp = Objects3d::POppAt(0);
    QVERIFY(pPOpp->isType7());
    for(int i=0; i<8; i++)
    {
        QVERIFY2(std::isfinite(pPOpp->eigValue(i).real()) && std::isfinite(pPOpp->eigValue(i).imag()), "eigenvalue is not finite");
    }
}


void TestFlow5Lib::deletePlanePolarDeletesItsOperatingPoints()
{
    PlaneXfl *pPlane = fixture::makePlane("Test plane");
    PlanePolar *pPolar = fixture::runT1Polar(pPlane, ALPHAS_DEG);
    QCOMPARE(Objects3d::nPOpps(), int(ALPHAS_DEG.size()));

    Objects3d::deletePlanePolar(pPolar);
    QCOMPARE(Objects3d::nPolars(), 0);
    QCOMPARE(Objects3d::nPOpps(), 0);
    QCOMPARE(Objects3d::nPlanes(), 1);
}


void TestFlow5Lib::deletePlaneResultsWithPolars()
{
    PlaneXfl *pPlane = fixture::makePlane("Test plane");
    fixture::runT1Polar(pPlane, ALPHAS_DEG);

    Objects3d::deletePlaneResults(pPlane, true);
    QCOMPARE(Objects3d::nPOpps(), 0);
    QCOMPARE(countPolarsOf("Test plane"), 0);
    QCOMPARE(Objects3d::nPlanes(), 1);
}


void TestFlow5Lib::deletePlaneLeavesNoOrphanPolars()
{
    PlaneXfl *pPlane = fixture::makePlane("Test plane");
    fixture::runT1Polar(pPlane, ALPHAS_DEG);

    Objects3d::deletePlane(pPlane); // default: bDeleteResults=true
    QCOMPARE(Objects3d::nPlanes(), 0);
    QCOMPARE(Objects3d::nPOpps(), 0);
    // deletePlane() calls deletePlaneResults() with bDeletePolars=false, so the polars stay in the store,
    // emptied, without their plane; the GUI avoids this by deleting the polars first
    QEXPECT_FAIL("", "Objects3d::deletePlane() leaves the plane's polars in the store", Continue);
    QCOMPARE(countPolarsOf("Test plane"), 0);
}


void TestFlow5Lib::projectRoundTrip()
{
    PlaneXfl *pPlane = fixture::makePlane("Test plane");
    PlanePolar *pPolar = fixture::runT1Polar(pPlane, ALPHAS_DEG);
    std::vector<double> CL, Cm;
    for(int i=0; i<pPolar->dataSize(); i++)
    {
        CL.push_back(pPolar->m_AF.at(i).CL());
        Cm.push_back(pPolar->m_AF.at(i).Cm());
    }
    int nPOpps = Objects3d::nPOpps();

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    std::string path = dir.filePath("roundtrip.fl5").toStdString();
    std::string log;
    QVERIFY2(io::saveProject(path, log), log.c_str());

    globals::deleteObjects();
    QCOMPARE(Objects3d::nPlanes(), 0);

    QVERIFY2(io::loadProject(path, log), log.c_str());
    QCOMPARE(Objects3d::nPlanes(), 1);
    QCOMPARE(Objects3d::nPolars(), 1);
    QCOMPARE(Objects3d::nPOpps(), nPOpps);

    PlanePolar const *pLoaded = Objects3d::planePolarAt(0);
    QCOMPARE(pLoaded->name(), std::string("T1 polar"));
    QCOMPARE(pLoaded->dataSize(), int(CL.size()));
    for(int i=0; i<pLoaded->dataSize(); i++)
    {
        QVERIFY(std::fabs(pLoaded->m_AF.at(i).CL()-CL.at(i))<ROUNDTRIPTOLERANCE);
        QVERIFY(std::fabs(pLoaded->m_AF.at(i).Cm()-Cm.at(i))<ROUNDTRIPTOLERANCE);
    }
}


void TestFlow5Lib::analysisIsDeterministic()
{
    PlaneXfl *pPlane = fixture::makePlane("Test plane");
    PlanePolar *pPolar = fixture::runT1Polar(pPlane, ALPHAS_DEG);
    std::vector<double> CL;
    for(int i=0; i<pPolar->dataSize(); i++) CL.push_back(pPolar->m_AF.at(i).CL());
    globals::deleteObjects();

    pPlane = fixture::makePlane("Test plane");
    pPolar = fixture::runT1Polar(pPlane, ALPHAS_DEG);
    QCOMPARE(pPolar->dataSize(), int(CL.size()));
    for(int i=0; i<pPolar->dataSize(); i++)
        QCOMPARE(pPolar->m_AF.at(i).CL(), CL.at(i)); // exactly: same inputs, same results
}


QTEST_GUILESS_MAIN(TestFlow5Lib)
#include "tst_flow5lib.moc"
