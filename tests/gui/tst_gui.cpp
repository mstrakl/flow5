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
 * GUI tests, run headless with the Qt offscreen platform. They drive the application's own objects
 * (the test links the app's object files) through the same slots as the menus and keys, and check that
 * the graphs, lists and docks follow the deletions.
 */

#include <QApplication>
#include <QDockWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QStandardItemModel>
#include <QTest>
#include <QTimer>

#include <api/api.h>
#include <api/objects3d.h>
#include <api/planeopp.h>
#include <api/planepolar.h>
#include <api/planexfl.h>

#include <globals/mainframe.h>
#include <interfaces/graphs/graph/curve.h>
#include <interfaces/graphs/graph/graph.h>
#include <interfaces/widgets/customwts/actionitemmodel.h>
#include <interfaces/widgets/customwts/cptableview.h>
#include <modules/xplane/controls/planeexplorer.h>
#include <modules/xplane/controls/stabtimectrls.h>
#include <modules/xplane/xplane.h>

#include "../common/fixture.h"


namespace
{
    /** a dock taller than this does not fit on a 1366x768 laptop screen, once the title bar, menus,
     * tool bar and status bar have taken their share */
    constexpr int MAXDOCKMINHEIGHT = 500; // pixels

    std::vector<double> const ALPHAS_DEG = {-2.0, 2.0, 6.0};

    /** answers the next modal message box, e.g. the "Are you sure?" questions of the delete actions */
    void answerNextMessageBox(QMessageBox::StandardButton answer)
    {
        QTimer::singleShot(0, [answer]()
        {
            QMessageBox *pBox = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
            if(pBox && pBox->button(answer)) pBox->button(answer)->click();
        });
    }

    bool hasCurveNamed(Graph const *pGraph, QString const &name)
    {
        for(int ic=0; ic<pGraph->curveCount(); ic++)
            if(pGraph->curve(ic)->name()==name) return true;
        return false;
    }

    int leftCurveCount(Graph const *pGraph)
    {
        int n = 0;
        for(int ic=0; ic<pGraph->curveCount(); ic++)
            if(pGraph->curve(ic)->isLeftAxis()) n++;
        return n;
    }
}


class TestGui : public QObject
{
    Q_OBJECT

    private slots:
        void initTestCase();
        void cleanupTestCase();
        void init();
        void cleanup();

        void docksFitSmallScreens();
        void deletePOppUpdatesGraphsAndPolar();
        void deleteLastPlaneClearsPOppGraphs();
        void deleteKeyAfterBulkPOppDeletion();
        void timeResponseCurveList();
        void timeResponsesComparedAcrossPointsCanBeDeleted();
        void deletingPolarPOppsEmptiesThePolarCurves();
        void deletingAllPOppsEmptiesThePolars();

    private:
        void loadIntoXPlane();

        MainFrame *m_pMainFrame{nullptr};
        XPlane *m_pXPlane{nullptr};
        PlaneXfl *m_pPlane{nullptr};
        PlanePolar *m_pT1Polar{nullptr};
};


void TestGui::initTestCase()
{
    m_pMainFrame = new MainFrame;
    m_pMainFrame->resize(1366, 768);
    m_pMainFrame->show();
    m_pXPlane = m_pMainFrame->m_pXPlane;
    QVERIFY(m_pXPlane);
}


void TestGui::cleanupTestCase()
{
    MainFrame::s_bSaved = true; // no "Save the current project?" question
    m_pMainFrame->onNewProject(); // empties the object stores and resets the modules
    delete m_pMainFrame;
}


void TestGui::init()
{
    m_pPlane = fixture::makePlane("Test plane");
    m_pT1Polar = fixture::runT1Polar(m_pPlane, ALPHAS_DEG);
}


void TestGui::cleanup()
{
    MainFrame::s_bSaved = true; // no "Save the current project?" question
    m_pMainFrame->onNewProject(); // empties the object stores and resets the modules
}


/** Sets the state that the GUI has after loading a project with the fixture's objects */
void TestGui::loadIntoXPlane()
{
    m_pMainFrame->handleIOResults(false);
    m_pXPlane->setPlane(m_pPlane);
    m_pXPlane->setPolar(m_pT1Polar);
    m_pXPlane->m_pPlaneExplorer->selectObjects();
}


void TestGui::docksFitSmallScreens()
{
    loadIntoXPlane();

    // the time response panel with the tallest page shown
    m_pXPlane->m_pStabTimeControls->m_ResponseType = StabTimeCtrls::FORCEDRESPONSE;
    m_pXPlane->m_pStabTimeControls->setControls();

    for(QDockWidget const *pDock : m_pMainFrame->findChildren<QDockWidget*>())
    {
        int h = pDock->minimumSizeHint().height();
        QVERIFY2(h<=MAXDOCKMINHEIGHT, qPrintable(QString("dock \"%1\" has a minimum height of %2 px").arg(pDock->windowTitle()).arg(h)));
    }
}


void TestGui::deletePOppUpdatesGraphsAndPolar()
{
    loadIntoXPlane();
    m_pXPlane->onPlaneOppView();
    m_pXPlane->m_bCurPOppOnly = false; // show all the operating points

    PlaneOpp *pPOpp = nullptr;
    for(int i=0; i<Objects3d::nPOpps(); i++)
        if(Objects3d::POppAt(i)->alpha()>1.0 && Objects3d::POppAt(i)->alpha()<3.0) pPOpp = Objects3d::POppAt(i);
    QVERIFY(pPOpp);
    m_pXPlane->setPlaneOpp(pPOpp);
    m_pXPlane->resetCurves();
    m_pXPlane->updateView();

    QString title = QString::fromStdString(pPOpp->title(true));
    QVERIFY2(hasCurveNamed(m_pXPlane->m_WingGraph.front(), title), "the operating point is not plotted before the deletion");
    int nPoints = m_pT1Polar->dataSize();

    m_pXPlane->onDeleteCurPOpp();

    QCOMPARE(Objects3d::nPOpps(), int(ALPHAS_DEG.size())-1);
    QVERIFY2(!hasCurveNamed(m_pXPlane->m_WingGraph.front(), title), "the deleted operating point is still plotted");
    QCOMPARE(m_pT1Polar->dataSize(), nPoints-1);
    for(double alpha : m_pT1Polar->m_Alpha) QVERIFY2(std::fabs(alpha-2.0)>0.001, "the polar still has the deleted point");
}


void TestGui::deleteLastPlaneClearsPOppGraphs()
{
    loadIntoXPlane();
    m_pXPlane->onPlaneOppView();
    m_pXPlane->m_bCurPOppOnly = false;
    m_pXPlane->resetCurves();
    m_pXPlane->updateView();
    QVERIFY(m_pXPlane->m_WingGraph.front()->curveCount()>0);

    answerNextMessageBox(QMessageBox::Yes);
    m_pXPlane->onDeleteCurPlane();

    QCOMPARE(Objects3d::nPlanes(), 0);
    QVERIFY(!m_pXPlane->curPOpp());
    for(Graph const *pGraph : m_pXPlane->m_WingGraph)
        QCOMPARE(pGraph->curveCount(), 0);
}


void TestGui::deleteKeyAfterBulkPOppDeletion()
{
    loadIntoXPlane();
    m_pXPlane->setPlaneOpp(Objects3d::POppAt(0));
    m_pXPlane->m_pPlaneExplorer->selectPlaneOpp(Objects3d::POppAt(0));

    m_pXPlane->onDeleteWPlrPOpps();
    QCOMPARE(Objects3d::nPOpps(), 0);
    QCOMPARE(Objects3d::nPolars(), 1);

    // the polar is now the highlighted item, so the Delete key deletes it
    answerNextMessageBox(QMessageBox::Yes);
    QTest::keyClick(m_pXPlane->m_pPlaneExplorer, Qt::Key_Delete);
    QCOMPARE(Objects3d::nPolars(), 0);
    QCOMPARE(Objects3d::nPlanes(), 1);
}


void TestGui::timeResponseCurveList()
{
    PlanePolar *pT7Polar = fixture::runT7Polar(m_pPlane);
    PlaneOpp *pT7POpp = nullptr;
    for(int i=0; i<Objects3d::nPOpps(); i++)
        if(pT7Polar->hasPOpp(Objects3d::POppAt(i))) pT7POpp = Objects3d::POppAt(i);
    QVERIFY2(pT7POpp, "the T7 analysis did not produce an operating point");

    m_pMainFrame->handleIOResults(false);
    m_pXPlane->setPlane(m_pPlane);
    m_pXPlane->setPolar(pT7Polar);
    m_pXPlane->setPlaneOpp(pT7POpp);
    m_pXPlane->onStabTimeView();

    StabTimeCtrls *pCtrls = m_pXPlane->m_pStabTimeControls;
    QVector<Graph*> const &graphs = m_pXPlane->m_TimeGraph;
    pCtrls->m_ResponseType = StabTimeCtrls::MODALRESPONSE;
    pCtrls->setControls();

    // two curves with the same settings get different names
    pCtrls->onAddCurve();
    pCtrls->onAddCurve();
    pCtrls->onAddCurve();
    QCOMPARE(pCtrls->m_pCurveModel->rowCount(), 3);
    QCOMPARE(leftCurveCount(graphs.front()), 3);
    QStringList names;
    for(int row=0; row<3; row++) names.append(pCtrls->m_pCurveModel->item(row)->text());
    names.removeDuplicates();
    QCOMPARE(names.size(), 3);

    // deleting the selected curve removes it from the list and from all the graphs, also from the right axis
    graphs.front()->showRightAxis(true);
    graphs.front()->setVariables(0, 0, 1);
    pCtrls->fillTimeGraphCurves();
    QString deleted = pCtrls->m_pCurveModel->item(1)->text();
    pCtrls->m_pcpCurveTable->setCurrentIndex(pCtrls->m_pCurveModel->index(1, 0));
    pCtrls->onDeleteCurve();
    QCOMPARE(pCtrls->m_pCurveModel->rowCount(), 2);
    for(Graph const *pGraph : graphs)
        QVERIFY2(!hasCurveNamed(pGraph, deleted), "a curve of the deleted response is still plotted");

    // switching between longitudinal and lateral clears the graphs and the list together
    pCtrls->m_prbLateral->click();
    QCOMPARE(pCtrls->m_pCurveModel->rowCount(), 0);
    for(Graph const *pGraph : graphs) QCOMPARE(pGraph->curveCount(), 0);
    pCtrls->m_prbLongitudinal->click();
}


/** The time responses are kept when another T7 operating point is selected, to compare them;
 * each one must still be deletable from the list */
void TestGui::timeResponsesComparedAcrossPointsCanBeDeleted()
{
    PlanePolar *pT7Polar = fixture::runT7Polar(m_pPlane, {0.0, 0.5});
    std::vector<PlaneOpp*> t7POpps;
    for(int i=0; i<Objects3d::nPOpps(); i++)
        if(pT7Polar->hasPOpp(Objects3d::POppAt(i))) t7POpps.push_back(Objects3d::POppAt(i));
    QCOMPARE(int(t7POpps.size()), 2);

    m_pMainFrame->handleIOResults(false);
    m_pXPlane->setPlane(m_pPlane);
    m_pXPlane->setPolar(pT7Polar);
    m_pXPlane->onStabTimeView();

    StabTimeCtrls *pCtrls = m_pXPlane->m_pStabTimeControls;
    QVector<Graph*> const &graphs = m_pXPlane->m_TimeGraph;
    pCtrls->m_ResponseType = StabTimeCtrls::MODALRESPONSE;

    // one response for each operating point
    for(PlaneOpp *pPOpp : t7POpps)
    {
        m_pXPlane->setPlaneOpp(pPOpp);
        m_pXPlane->resetCurves();
        m_pXPlane->updateView();
        pCtrls->setControls();
        pCtrls->onAddCurve();
    }
    QCOMPARE(pCtrls->m_pCurveModel->rowCount(), 2);
    QCOMPARE(leftCurveCount(graphs.front()), 2);

    // delete them one by one, the first one computed for the other operating point
    for(int n=2; n>0; n--)
    {
        QString name = pCtrls->m_pCurveModel->item(0)->text();
        pCtrls->m_pcpCurveTable->setCurrentIndex(pCtrls->m_pCurveModel->index(0, 0));
        pCtrls->onDeleteCurve();
        QCOMPARE(pCtrls->m_pCurveModel->rowCount(), n-1);
        for(Graph const *pGraph : graphs)
        {
            QVERIFY2(!hasCurveNamed(pGraph, name), "a curve of the deleted response is still plotted");
            QCOMPARE(leftCurveCount(pGraph), n-1);
        }
    }
}


/** the number of points of the curves which the polar has drawn in the current view */
static int polarCurvePoints(PlanePolar *pPolar)
{
    int n = 0;
    for(int ic=0; ic<pPolar->curveCount(); ic++)
        n += static_cast<Curve*>(pPolar->curve(ic))->count();
    return n;
}


/** "Current polar > Operating points > Delete" on a T7 polar: the polar's points are the results of
 * the operating points, so the polar graphs and the polar's data points become empty */
void TestGui::deletingPolarPOppsEmptiesThePolarCurves()
{
    PlanePolar *pT7Polar = fixture::runT7Polar(m_pPlane, {0.0, 0.5});
    QCOMPARE(pT7Polar->dataSize(), 2);

    m_pMainFrame->handleIOResults(false);
    m_pXPlane->setPlane(m_pPlane);
    m_pXPlane->setPolar(pT7Polar);
    m_pXPlane->setPlaneOpp(nullptr);

    m_pXPlane->onPolarView();
    QVERIFY(polarCurvePoints(pT7Polar)>0);

    m_pXPlane->onDeleteWPlrPOpps();
    QCOMPARE(pT7Polar->dataSize(), 0);          // what "Edit data points" shows
    QCOMPARE(polarCurvePoints(pT7Polar), 0);
    QCOMPARE(m_pT1Polar->dataSize(), int(ALPHAS_DEG.size())); // the other polar is unchanged

    m_pXPlane->onRootLocusView();
    QCOMPARE(polarCurvePoints(pT7Polar), 0);
}


void TestGui::deletingAllPOppsEmptiesThePolars()
{
    PlanePolar *pT7Polar = fixture::runT7Polar(m_pPlane, {0.0, 0.5});
    loadIntoXPlane();
    m_pXPlane->onPolarView();

    m_pXPlane->onDeleteAllPOpps();
    QCOMPARE(Objects3d::nPOpps(), 0);
    QCOMPARE(m_pT1Polar->dataSize(), 0);
    QCOMPARE(pT7Polar->dataSize(), 0);
    QCOMPARE(polarCurvePoints(m_pT1Polar), 0);
}


int main(int argc, char *argv[])
{
    // as in the app's main(): the 3d views share their OpenGL contexts
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication app(argc, argv);
    TestGui test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_gui.moc"
