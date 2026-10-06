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


#define _MATH_DEFINES_DEFINED


#include "stabtimectrls.h"

#include <QApplication>
#include <QGridLayout>
#include <QGroupBox>
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QTimer>
#include <QCheckBox>
#include <QMenu>
#include <QStandardItemModel>
#include <complex>


#include <api/constants.h>
#include <api/geom_global.h>
#include <api/planeopp.h>
#include <api/planepolar.h>
#include <api/planexfl.h>
#include <api/units.h>
#include <api/flow5-io.h>

#include <core/displayoptions.h>
#include <core/xflcore.h>

#include <core/xflcore.h>
#include <interfaces/graphs/controls/graphoptions.h>
#include <interfaces/graphs/graph/curve.h>
#include <interfaces/widgets/customdlg/newnamedlg.h>
#include <interfaces/widgets/customwts/actionitemmodel.h>
#include <interfaces/widgets/customwts/cptableview.h>
#include <interfaces/widgets/customwts/floatedit.h>
#include <interfaces/widgets/customwts/xfldelegate.h>
#include <interfaces/widgets/line/linebtn.h>
#include <interfaces/widgets/line/linemenu.h>
#include <modules/xplane/glview/gl3dxplaneview.h>
#include <modules/xplane/xplane.h>


XPlane *StabTimeCtrls::s_pXPlane=nullptr;


StabTimeCtrls::StabTimeCtrls(QWidget *pParent) : QFrame(pParent)
{
    setWindowTitle(tr("Stability time controls"));

    m_InputGraph.setName("StabTime graph");
    m_InputGraph.setCurveModel(new CurveModel);
    m_InputGraph.setXVariableList({"t (s)"});
    m_InputGraph.setYVariableList({"u (ctrl units)"});

    m_bLinearInput   = true;
    m_bHoldLastInput = true;

    m_ResponseType = MODALRESPONSE;

    m_iCurrentMode = 0;
    m_TimeInput[0] = m_TimeInput[1] = m_TimeInput[2] = m_TimeInput[3] = 0.0;
    m_TotalTime = 1;//s
    m_Deltat    = 0.001;//s

    m_pRenameAct = new QAction(tr("Rename"), this);
    m_pDeleteAct = new QAction(tr("Delete"), this);

    makeTable();
    setupLayout();

    m_prbLongitudinal->setChecked(true);

    connectSignals();


}


StabTimeCtrls::~StabTimeCtrls()
{
    m_InputGraph.deleteCurveModel();
    if(m_pCurveModel) delete m_pCurveModel;
}


void StabTimeCtrls::connectSignals()
{
    connect(m_prbLongitudinal, SIGNAL(clicked(bool)), SLOT(onStabilityDirection()));
    connect(m_prbLateral,      SIGNAL(clicked(bool)), SLOT(onStabilityDirection()));

    for(int imode=0; imode<4; imode++)
        connect(m_prbTimeMode[imode], SIGNAL(clicked()), SLOT(onModeSelection()));

    connect(m_pfeDeltat,    SIGNAL(floatChanged(float)), SLOT(onReadData()));

    connect(m_prbInitCondResponse, SIGNAL(clicked()), SLOT(onResponseType()));
    connect(m_prbForcedResponse,   SIGNAL(clicked()), SLOT(onResponseType()));
    connect(m_prbModalResponse,    SIGNAL(clicked()), SLOT(onResponseType()));

    connect(m_ppbAddCurve,   SIGNAL(clicked()),      SLOT(onAddCurve()));

    connect(m_pRenameAct, SIGNAL(triggered(bool)), SLOT(onRenameCurve()));
    connect(m_pDeleteAct, SIGNAL(triggered(bool)), SLOT(onDeleteCurve()));

    connect(m_pcpCurveTable, SIGNAL(clicked(QModelIndex)), SLOT(onCurveTableClicked(QModelIndex)));
    connect(m_pCurveModel, SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)), SLOT(onDataChanged(QModelIndex,QModelIndex)));

    connect(m_pInputDelegate,   SIGNAL(closeEditor(QWidget*)),             SLOT(onInputTableChanged()));
    connect(m_pcptInputTable,   SIGNAL(dataPasted()),                      SLOT(onInputTableChanged()));
    connect(m_pcptInputTable,   SIGNAL(customContextMenuRequested(QPoint)), SLOT(onInputTableContextMenu(QPoint)));
    connect(m_pSplGraphWt,      SIGNAL(splineModified(int,int)),           SLOT(onInputSplineModified()));
    connect(m_prbInputLinear,   SIGNAL(clicked(bool)),                     SLOT(onInputInterpolation()));
    connect(m_prbInputSmooth,   SIGNAL(clicked(bool)),                     SLOT(onInputInterpolation()));
    connect(m_pchHoldLastInput, &QCheckBox::toggled, this, [this](bool b){m_bHoldLastInput=b;});
    connect(m_pcbAVLControls,   &QComboBox::currentIndexChanged, this, [this](int){updateGainHint();});
}


void StabTimeCtrls::showEvent(QShowEvent *pEvent)
{
    QFrame::showEvent(pEvent);

    GraphOptions::resetGraphSettings(*m_pSplGraphWt->graph());
    QFont fnt(DisplayOptions::tableFont());
    fnt.setPointSize(DisplayOptions::tableFont().pointSize());
    m_pcpCurveTable->setFont(fnt);
    m_pcpCurveTable->horizontalHeader()->setFont(fnt);
    m_pcpCurveTable->verticalHeader()->setFont(fnt);


    onResizeColumns();
    setControls();
}


void StabTimeCtrls::resizeEvent(QResizeEvent *pEvent)
{
    QFrame::resizeEvent(pEvent);
    onResizeColumns();
}


void StabTimeCtrls::onResizeColumns()
{
    double w = double(m_pcpCurveTable->width())/100.0;
    m_pcpCurveTable->setColumnWidth(0, int(35.0*w));
    m_pcpCurveTable->setColumnWidth(1, int(35.0*w));
    update();
}


void StabTimeCtrls::keyPressEvent(QKeyEvent *pEvent)
{
    switch (pEvent->key())
    {
        case Qt::Key_Return:
        case Qt::Key_Enter:
        {
            if(!m_ppbAddCurve->hasFocus()) m_ppbAddCurve->setFocus();
            else onPlotStabilityGraph();

            break;
        }
        default:
        {
            s_pXPlane->keyPressEvent(pEvent);
        }
    }
}


void StabTimeCtrls::onStabilityDirection()
{
    fillCurveList();

    setMode();
    setControls();

    bool bLongitudinal = isStabLongitudinal();
    s_pXPlane->updateStabilityDirection(bLongitudinal);
}


void StabTimeCtrls::onDataChanged(QModelIndex topleft,QModelIndex )
{
    // using the standard model, so retrieve the data
    // only case to process: col 0, the name has been edited
    // the line style is edited from this class using a LineMenu

    if(!topleft.isValid()) return;

    int row = topleft.row();
    if(topleft.column()==0)
    {
        QString curvename = m_pCurveModel->index(row, topleft.column(), QModelIndex()).data().toString();
        Curve const *pOldCurve = s_pXPlane->m_TimeGraph.at(0)->curve(row);
        if(pOldCurve) renameTimeResponse(pOldCurve->name(), curvename);
        for(int ig=0; ig<s_pXPlane->m_TimeGraph.size(); ig++)
        {
            Curve *pCurve= s_pXPlane->m_TimeGraph.at(ig)->curve(row);
            if(pCurve)
            {
                pCurve->setName(curvename);
            }
        }
        s_pXPlane->makeLegend();
        s_pXPlane->updateView();
    }
}


void StabTimeCtrls::onCurveTableClicked(QModelIndex index)
{
    if(!index.isValid()) return

    m_pcpCurveTable->selectRow(index.row());
    for(int i=0; i<s_pXPlane->m_TimeGraph.size(); i++)
    {
        s_pXPlane->m_TimeGraph.at(i)->clearSelection();
        Curve *pCurve = s_pXPlane->m_TimeGraph.at(i)->curve(index.row());
        s_pXPlane-> m_TimeGraph[i]->selectCurve(pCurve);
    }

    switch(index.column())
    {
        case 1:
        {
            int row = index.row();

            Curve *pCurrentCurve = s_pXPlane->m_TimeGraph[0]->curve(row);
            if(!pCurrentCurve) return;

            LineMenu *lineMenu = new LineMenu(nullptr);
            lineMenu->initMenu(pCurrentCurve->theStyle());
            lineMenu->exec(QCursor::pos());

            // update the model
            QStandardItem *pItem = m_pCurveModel->itemFromIndex(index);
            if(pItem) pItem->setData(QVariant::fromValue(lineMenu->theStyle()), Qt::DisplayRole);

            // update the curve style
            for(int ig=0; ig<s_pXPlane->m_TimeGraph.size(); ig++)
            {
                Curve *pCurrentCurve = s_pXPlane->m_TimeGraph[ig]->curve(row);
                pCurrentCurve->setTheStyle(lineMenu->theStyle());
            }
            s_pXPlane->makeLegend();
            s_pXPlane->updateView();
            break;
        }
        case 2:
        {
            QRect itemrect = m_pcpCurveTable->visualRect(index);
            QPoint menupos = m_pcpCurveTable->mapToGlobal(itemrect.topLeft());
            QMenu *pCurveTableRowMenu = new QMenu(tr("Section"),this);
            pCurveTableRowMenu->addAction(m_pRenameAct);
            pCurveTableRowMenu->addAction(m_pDeleteAct);
            pCurveTableRowMenu->exec(menupos, m_pRenameAct);

            break;
        }
        default:
        {
            break;
        }
    }
}


void StabTimeCtrls::onPlotStabilityGraph()
{
    s_pXPlane->createStabilityCurves();
    s_pXPlane->updateView();
}


void StabTimeCtrls::onModeSelection()
{
    if(s_pXPlane->isStabTimeView())
    {
        for(int imode=0; imode<4; imode++)
        {
            if(m_prbTimeMode[imode]->isChecked())
            {
                m_iCurrentMode = imode;
                break;
            }
        }
    }
    if(!isStabLongitudinal())m_iCurrentMode +=4;

    setMode(m_iCurrentMode);
}


void StabTimeCtrls::onReadData()
{
    m_Deltat = m_pfeDeltat->value();
}



void StabTimeCtrls::onResponseType()
{
    enumStabTimeResponse type = INITIALCONDITIONS;

    if     (m_prbInitCondResponse->isChecked())    type=INITIALCONDITIONS;
    else if(m_prbForcedResponse->isChecked())      type=FORCEDRESPONSE;
    else if(m_prbModalResponse->isChecked())       type=MODALRESPONSE;

    if(type==m_ResponseType) return;

    m_ResponseType=type;
    setControls();
    s_pXPlane->updateView();
}


void StabTimeCtrls::setMode(int iMode)
{
    if(iMode>=0)
    {
        m_iCurrentMode = iMode%4;
        if(!isStabLongitudinal()) m_iCurrentMode += 4;
    }
    else if(m_iCurrentMode<0) m_iCurrentMode=0;
}


void StabTimeCtrls::makeTable()
{
    m_pcpCurveTable = new CPTableView(this);
    m_pcpCurveTable->setShowGrid(false);
    m_pcpCurveTable->setEditable(true);

    m_pCurveModel = new ActionItemModel(this);
    m_pCurveModel->setRowCount(0);//nothing to start with
    m_pCurveModel->setColumnCount(3);
    m_pCurveModel->setActionColumn(2);
    m_pCurveModel->setHeaderData(0, Qt::Horizontal, "Name");
    m_pCurveModel->setHeaderData(1, Qt::Horizontal, "Style");
    m_pCurveModel->setHeaderData(2, Qt::Horizontal, "Actions");
    m_pcpCurveTable->setModel(m_pCurveModel);

    QHeaderView *pHorizontalHeader = m_pcpCurveTable->horizontalHeader();
    pHorizontalHeader->setStretchLastSection(true);

    XflDelegate *pEditActionDelegate = new XflDelegate(this);
    pEditActionDelegate->setActionColumn(2);
    pEditActionDelegate->setDigits({-1,-1,-1});

    QVector<XflDelegate::enumItemType> ItemType = {XflDelegate::STRING, XflDelegate::LINE, XflDelegate::ACTION};
    pEditActionDelegate->setItemTypes(ItemType);

    m_pcpCurveTable->setItemDelegate(pEditActionDelegate);
}


void StabTimeCtrls::setupLayout()
{

    QGroupBox *pgbDirection = new QGroupBox(tr("Direction"));
    {
        QHBoxLayout *pDirLayout = new QHBoxLayout;
        {
            QButtonGroup *pGroup = new QButtonGroup(this);
            {
                m_prbLongitudinal = new QRadioButton(tr("Longitudinal"));
                m_prbLateral = new QRadioButton(tr("Lateral"));
                pGroup->addButton(m_prbLongitudinal);
                pGroup->addButton(m_prbLateral);
            }

            pDirLayout->addStretch();
            pDirLayout->addWidget(m_prbLongitudinal);
            pDirLayout->addWidget(m_prbLateral);
            pDirLayout->addStretch();
        }
        pgbDirection->setLayout(pDirLayout);
    }

    QGroupBox *pgbCurveType = new QGroupBox(tr("Response type"));
    {
        QHBoxLayout *pResponseTypeLayout = new QHBoxLayout;
        {
            m_prbModalResponse = new QRadioButton(tr("Modal"));
            m_prbModalResponse->setToolTip(tr("<p>Display the time response on a specific mode with normalized amplitude and random initial phase</p>"));
            m_prbInitCondResponse = new QRadioButton(tr("Initial conditions"));
            m_prbInitCondResponse->setToolTip(tr("<p>Display the time response for specified initial conditions</p>"));
            m_prbForcedResponse = new QRadioButton(tr("Forced"));
            m_prbForcedResponse->setToolTip(tr("<p>Display the time response for a given control actuation in the form "
                                            "of a user-specified function of time</p>"));
            pResponseTypeLayout->addStretch();
            pResponseTypeLayout->addWidget(m_prbModalResponse);
            pResponseTypeLayout->addWidget(m_prbInitCondResponse);
            pResponseTypeLayout->addWidget(m_prbForcedResponse);
            pResponseTypeLayout->addStretch();
        }
        pgbCurveType->setLayout(pResponseTypeLayout);
    }

    m_pswInitialConditions = new QStackedWidget;
    {
        QGroupBox *pgbInitCondResponse = new QGroupBox(tr("Initial conditions response"));
        {
            QGridLayout *pVarParamsLayout = new QGridLayout;
            {
                m_plabStab1 = new QLabel("u0__");
                m_plabStab2 = new QLabel("w0__");
                m_plabStab3 = new QLabel("q0__");
                m_plabUnit1 = new QLabel("m/s");
                m_plabUnit2 = new QLabel("m/s");
                m_plabUnit3 = new QLabel("rad/s");
                m_plabStab1->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
                m_plabStab2->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
                m_plabStab3->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
                m_plabUnit1->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                m_plabUnit2->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                m_plabUnit3->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                m_pfeStabVar1 = new FloatEdit;
                m_pfeStabVar2 = new FloatEdit;
                m_pfeStabVar3 = new FloatEdit;
                pVarParamsLayout->addWidget(m_plabStab1,   1,1);
                pVarParamsLayout->addWidget(m_plabStab2,   2,1);
                pVarParamsLayout->addWidget(m_plabStab3,   3,1);
                pVarParamsLayout->addWidget(m_pfeStabVar1, 1,2);
                pVarParamsLayout->addWidget(m_pfeStabVar2, 2,2);
                pVarParamsLayout->addWidget(m_pfeStabVar3, 3,2);
                pVarParamsLayout->addWidget(m_plabUnit1,   1,3);
                pVarParamsLayout->addWidget(m_plabUnit2,   2,3);
                pVarParamsLayout->addWidget(m_plabUnit3,   3,3);
                pVarParamsLayout->setRowStretch(4,1);
                pVarParamsLayout->setColumnStretch(4,1);
            }
            pgbInitCondResponse->setLayout(pVarParamsLayout);
        }

        QGroupBox *pgbForcedResponse = new QGroupBox(tr("Forced response"));
        {
            QVBoxLayout *pForcedResponseLayout = new QVBoxLayout;
            {
                m_pcbAVLControls = new QComboBox(this);
                QLabel *pForcedText = new QLabel(tr("Control function"));

                m_pSplGraphWt = new SplinedGraphWt;
                {
                    m_pSplGraphWt->setGraph(&m_InputGraph);
                    m_pSplGraphWt->setEndPointConstrain(false); // the input may start and end at any value
                    m_pSplGraphWt->setXLimits(0.0, 1.e10);
                    m_pSplGraphWt->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
                    m_pSplGraphWt->setToolTip(tr("<p>Drag a point to move it, Shift+click to insert a point, Ctrl+click to delete a point.</p>"));
                    GraphOptions::resetGraphSettings(m_InputGraph);
                }

                QHBoxLayout *pInterpolationLayout = new QHBoxLayout;
                {
                    m_prbInputLinear = new QRadioButton(tr("Linear"));
                    m_prbInputLinear->setToolTip(tr("<p>The control input is interpolated linearly between the points; "
                                                    "each point is a value of the input.<br>"
                                                    "Two points at the same time make a step.</p>"));
                    m_prbInputSmooth = new QRadioButton(tr("Smooth"));
                    m_prbInputSmooth->setToolTip(tr("<p>The control input is a B-spline; "
                                                    "the points are the spline's control points and the input passes only through the first and last ones.</p>"));
                    m_pchHoldLastInput = new QCheckBox(tr("Hold last value"));
                    m_pchHoldLastInput->setToolTip(tr("<p>If checked, the input keeps the last point's value until the end of the simulation; "
                                                      "otherwise it returns to 0 after the last point.<br>"
                                                      "Before the first point the input is 0.</p>"));
                    pInterpolationLayout->addWidget(m_prbInputLinear);
                    pInterpolationLayout->addWidget(m_prbInputSmooth);
                    pInterpolationLayout->addStretch();
                    pInterpolationLayout->addWidget(m_pchHoldLastInput);
                }

                m_pcptInputTable = new CPTableView(this);
                {
                    m_pcptInputTable->setEditable(true);
                    m_pcptInputTable->setWindowTitle(tr("Control input"));
                    m_pcptInputTable->setContextMenuPolicy(Qt::CustomContextMenu);
                    m_pcptInputTable->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
                    m_pInputModel = new QStandardItemModel(this);
                    m_pInputModel->setColumnCount(2);
                    m_pInputModel->setHeaderData(0, Qt::Horizontal, tr("t (s)"));
                    m_pInputModel->setHeaderData(1, Qt::Horizontal, tr("u (ctrl units)"));
                    m_pcptInputTable->setModel(m_pInputModel);
                    m_pcptInputTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
                    m_pInputDelegate = new XflDelegate(this);
                    m_pInputDelegate->setNCols(2, XflDelegate::DOUBLE);
                    m_pInputDelegate->setDigits({3,3});
                    m_pcptInputTable->setItemDelegate(m_pInputDelegate);
                }

                m_plabGainHint = new QLabel;
                m_plabGainHint->setWordWrap(true);

                pForcedResponseLayout->addWidget(m_pcbAVLControls);
                pForcedResponseLayout->addWidget(pForcedText);
                pForcedResponseLayout->addWidget(m_pSplGraphWt);
                pForcedResponseLayout->addLayout(pInterpolationLayout);
                pForcedResponseLayout->addWidget(m_pcptInputTable);
                pForcedResponseLayout->addWidget(m_plabGainHint);
            }
            pgbForcedResponse->setLayout(pForcedResponseLayout);
        }

        QGroupBox *pgbModalTime = new QGroupBox(tr("Modal response"));
        {
            pgbModalTime->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
            QVBoxLayout *pModalTimeLayout = new QVBoxLayout;
            {
                for(int imode=0; imode<4; imode++)
                {
                    m_prbTimeMode[imode] = new QRadioButton(tr("Mode %1").arg(imode+1));
                    pModalTimeLayout->addWidget(m_prbTimeMode[imode]);
                }
                pModalTimeLayout->addStretch(1);
            }
            pgbModalTime->setLayout(pModalTimeLayout);
        }

        m_pswInitialConditions->addWidget(pgbModalTime);
        m_pswInitialConditions->addWidget(pgbInitCondResponse);
        m_pswInitialConditions->addWidget(pgbForcedResponse);
    }


    QGridLayout *pDtLayout  = new QGridLayout;
    {
        QLabel *pDtLabel        = new QLabel(tr("dt="));
        QLabel *pTotalTimeLabel = new QLabel(tr("Total Time ="));
        QLabel *pTimeLab1       = new QLabel(tr("s"));
        QLabel *pTimeLab2       = new QLabel(tr("s"));
        m_pfeTotalTime = new FloatEdit(5.0f);
        m_pfeTotalTime->setToolTip(tr("<p>Define the total time range for the graphs</p>"));
        m_pfeDeltat    = new FloatEdit(.01f);
        m_pfeDeltat->setToolTip(tr("<p>Define the time step for the resolution of the differential equations</p>"));
        pDtLayout->addWidget(pDtLabel,         1,1, Qt::AlignRight);
        pDtLayout->addWidget(m_pfeDeltat,      1,2);
        pDtLayout->addWidget(pTimeLab1,        1,3);
        pDtLayout->addWidget(pTotalTimeLabel,  2,1, Qt::AlignRight);
        pDtLayout->addWidget(m_pfeTotalTime,   2,2);
        pDtLayout->addWidget(pTimeLab2,        2,3);
    }

    QGroupBox *pgbCurveSettings = new QGroupBox(tr("Curve settings"));
    {
        QVBoxLayout *pTimeLayout = new QVBoxLayout;
        {
            m_ppbAddCurve  = new QPushButton(tr("Add"));
            m_ppbAddCurve->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);
            m_ppbAddCurve->setToolTip(tr("<p>Add a new curve to the graphs, using the current user-specified input</p>"));

            pTimeLayout->addLayout(pDtLayout);
            pTimeLayout->addWidget(m_ppbAddCurve);
            pTimeLayout->addWidget(m_pcpCurveTable);
        }
        pgbCurveSettings->setLayout(pTimeLayout);
    }

    QVBoxLayout *pTimeParamsLayout = new QVBoxLayout;
    {
        //    TimeParamsLayout->addLayout(InitialConditionsLayout);
        pTimeParamsLayout->addWidget(pgbDirection);
        pTimeParamsLayout->addWidget(pgbCurveType);
        pTimeParamsLayout->addWidget(m_pswInitialConditions);
        pTimeParamsLayout->addWidget(pgbCurveSettings);
    }

    setLayout(pTimeParamsLayout);
}


void StabTimeCtrls::fillAVLcontrols(PlanePolar const*pWPolar)
{
    m_pcbAVLControls->clear();
    if(pWPolar)
    {
        for(int ic=0; ic<pWPolar->nAVLCtrls(); ic++)
        {
            m_pcbAVLControls->addItem(QString::fromStdString(pWPolar->AVLCtrl(ic).name()));
        }
    }
    m_pcbAVLControls->setPlaceholderText(tr("No AVL-type control set in the polar"));
}


/** Returns an explanation of why the forced response cannot be computed, or an empty string if it can */
QString StabTimeCtrls::forcedResponseError(PlaneOpp const *pPOpp) const
{
    PlanePolar const *pPolar = s_pXPlane->curPlPolar();
    if(!pPOpp || !pPOpp->isType7())
        return tr("No T7 stability operating point is selected. Run a T7 analysis and select one of its operating points.");
    if(!pPolar || pPolar->nAVLCtrls()==0)
        return tr("The T7 polar has no AVL-type control set, so no control derivatives are available.\n"
                  "Define a control set with a non-zero gain in the polar's \"AVL-type ctrls\" tab, then re-run the analysis.");
    if(pPOpp->m_BLong.size()==0 || pPOpp->m_BLat.size()==0)
        return tr("The selected operating point has no control derivatives. "
                  "It was probably computed before the AVL-type control sets were defined; re-run the T7 analysis.");
    int iCtrl = m_pcbAVLControls->currentIndex();
    if(iCtrl<0 || iCtrl>=int(pPOpp->m_BLong.size()))
        return tr("No control set is selected in the forced response list, or the selected set does not match "
                  "the operating point's control derivatives; re-run the T7 analysis.");
    if(!pPolar->AVLCtrl(iCtrl).hasActiveAngle())
        return tr("The control set %1 has only zero gains, so its control derivatives are zero. "
                  "Set a non-zero gain in the polar's \"AVL-type ctrls\" tab, then re-run the analysis.")
                .arg(QString::fromStdString(pPolar->AVLCtrl(iCtrl).name()));
    return QString();
}


void StabTimeCtrls::setControls()
{
    QString str, strong;
    str = Units::speedUnitQLabel();

    m_pswInitialConditions->setCurrentIndex(m_ResponseType);

    m_prbInitCondResponse->setChecked(m_ResponseType==INITIALCONDITIONS);
    m_prbForcedResponse->setChecked(  m_ResponseType==FORCEDRESPONSE);
    m_prbModalResponse->setChecked(   m_ResponseType==MODALRESPONSE);

    setMode(m_iCurrentMode);

    strong = DEGch + "/s";
    if(isStabLongitudinal())
    {
        m_plabStab1->setText("<p>u<sub>0</sub>=</p>");
        m_plabStab2->setText("<p>w<sub>0</sub>=</p>");
        m_plabStab3->setText("<p>q<sub>0</sub>=</p>");
        m_plabUnit1->setText(str);
        m_plabUnit2->setText(str);
        m_plabUnit3->setText(strong);
    }
    else
    {
        m_plabStab1->setText("<p>v<sub>0</sub>=</p>");
        m_plabStab2->setText("<p>p<sub>0</sub>=</p>");
        m_plabStab3->setText("<p>r<sub>0</sub>=</p>");
        m_plabUnit1->setText(str);
        m_plabUnit2->setText(strong);
        m_plabUnit3->setText(strong);
    }

    m_pfeStabVar1->setValue(m_TimeInput[0]);
    m_pfeStabVar2->setValue(m_TimeInput[1]);
    m_pfeStabVar3->setValue(m_TimeInput[2]);
    m_pfeTotalTime->setValue(m_TotalTime);
    m_pfeDeltat->setValue(m_Deltat);

    bool bEnableTimeCtrl = s_pXPlane->m_pCurPOpp && s_pXPlane->m_pCurPOpp->isType7() && s_pXPlane->isStabTimeView();
    m_ppbAddCurve->setEnabled(bEnableTimeCtrl);
    m_pcpCurveTable->setEnabled(m_pCurveModel->rowCount());

    for(int imode=0; imode<4; imode++)
    {
        m_prbTimeMode[imode]->setChecked(m_iCurrentMode%4==imode);
        m_prbTimeMode[imode]->setEnabled(bEnableTimeCtrl);
    }

    m_pfeStabVar1->setEnabled(bEnableTimeCtrl);
    m_pfeStabVar2->setEnabled(bEnableTimeCtrl);
    m_pfeStabVar3->setEnabled(bEnableTimeCtrl);
    m_pfeDeltat->setEnabled(bEnableTimeCtrl);
    m_pfeTotalTime->setEnabled(bEnableTimeCtrl);

    m_pcbAVLControls->setEnabled(bEnableTimeCtrl && m_prbForcedResponse->isChecked());

    m_prbInputLinear->setChecked(m_bLinearInput);
    m_prbInputSmooth->setChecked(!m_bLinearInput);
    m_pchHoldLastInput->setChecked(m_bHoldLastInput);
    setInputInterpolation();
    updateGainHint();
}


QString StabTimeCtrls::selectedCurveName()
{
    QModelIndex index = m_pcpCurveTable->currentIndex();
    if(!index.isValid()) return QString();

    QModelIndex sib = index.sibling(index.row(), 0);
    if(sib.isValid()) return m_pCurveModel->data(sib).toString();
    else              return QString();
}


Curve *StabTimeCtrls::selectedCurve()
{
    // cannot proceed by index because the sorting order of the graph's CurveModel
    // may not match the order of the table's model

    QModelIndex index = m_pcpCurveTable->currentIndex();
    if(!index.isValid()) return nullptr;

    QModelIndex sib = index.sibling(index.row(), 0);
    if(!sib.isValid()) return nullptr;

    QString strange = m_pCurveModel->data(sib).toString();
    return s_pXPlane->m_TimeGraph.at(0)->curve(strange);
}


void StabTimeCtrls::selectCurve(Curve const *pCurve)
{
    if(!pCurve) return;

    for(int ir=0; ir<m_pCurveModel->rowCount(); ir++)
    {
        QModelIndex index = m_pCurveModel->index(ir, 0);
        QStandardItem *pItem = m_pCurveModel->itemFromIndex(index);
        if(pItem && pItem->text().compare(pCurve->name())==0)
        {
            m_pcpCurveTable->setCurrentIndex(index);
        }
    }
}

void StabTimeCtrls::onRenameCurve()
{
    Curve *pSelCurve = selectedCurve();
    if(!pSelCurve) return;

    QString NewName = "Test Name";
    NewNameDlg dlg(pSelCurve->name(), this);

    if(dlg.exec() != QDialog::Accepted) return;
    NewName = dlg.newName();
    renameTimeResponse(pSelCurve->name(), NewName);

    for (int i=0; i<s_pXPlane->m_TimeGraph.at(0)->curveCount(); i++)
    {
        Curve *pCurve = s_pXPlane->m_TimeGraph.at(0)->curve(i);
        if(pCurve && (pCurve == pSelCurve))
        {
            for(int ig=0; ig<s_pXPlane->m_TimeGraph.size(); ig++)
            {
                pCurve = s_pXPlane->m_TimeGraph.at(ig)->curve(i);
                pCurve->setName(NewName);
            }

            fillCurveList();
            s_pXPlane->makeLegend();
            s_pXPlane->updateView();
            return;
        }
    }
}


void StabTimeCtrls::onSelChangeCurve(int )
{
}


void StabTimeCtrls::addCurve()
{
    QString strong;
    bool bLongitudinal = isStabLongitudinal();

    double val1 = m_pfeStabVar1->value();
    double val2 = m_pfeStabVar2->value();
    double val3 = m_pfeStabVar3->value();
    switch(m_ResponseType)
    {
        case INITIALCONDITIONS:
        {
            if(bLongitudinal) strong = tr("u0=%1 w0=%2 q0=%3").arg(val1, 5, 'f', 2).arg(val2, 5, 'f', 2).arg(val3, 5, 'f', 2);
            else              strong = tr("v0=%1 p0=%2 r0=%3").arg(val1, 5, 'f', 2).arg(val2, 5, 'f', 2).arg(val3, 5, 'f', 2);
            break;
        }
        case FORCEDRESPONSE:
        {
            strong = tr("Forced response");
            break;
        }
        case MODALRESPONSE:
        {
            strong = tr("Mode %1").arg(m_iCurrentMode+1);
            break;
        }
    }

    // check if the name exists
    for(int row=0; row<m_pCurveModel->rowCount(); row++)
    {
        QStandardItem *pItem = m_pCurveModel->item(row);
        if(pItem && pItem->text().compare(strong, Qt::CaseInsensitive)==0)
        {
            strong += " (2)";
        }
    }

    Curve *pCurve=nullptr;
    fl5Color clr = xfl::randomfl5Color(!DisplayOptions::isLightTheme());
    for(int ig=0; ig<s_pXPlane->m_TimeGraph.size(); ig++)
    {
        pCurve = s_pXPlane->m_TimeGraph.at(ig)->addCurve();
        pCurve->setColor(clr);
        pCurve->setDefaultLineWidth(Curve::defaultLineWidth());
        pCurve->setName(strong);
    }

    if(pCurve)
    {
        appendRow(pCurve);
    }
//    m_pCurveModel->sort(0, Qt::AscendingOrder);

    int row=0;
    if(pCurve)
    {
        for(row=0; row<m_pCurveModel->rowCount(); row++)
        {
            QStandardItem *pItem = m_pCurveModel->item(row);
            if(pItem && pItem->text().compare(pCurve->name())==0) break;
        }
    }

    if(row<m_pCurveModel->rowCount())
    {
        m_pcpCurveTable->selectRow(row);
    }

    m_pcpCurveTable->setEnabled(   s_pXPlane->m_pCurPOpp && m_pCurveModel->rowCount());
}


void StabTimeCtrls::appendRow(Curve const *pCurve)
{
    int rowcount = m_pCurveModel->rowCount();
    rowcount++;
    m_pCurveModel->setRowCount(rowcount);

    m_pCurveModel->blockSignals(true);

    QModelIndex Xindex = m_pCurveModel->index(rowcount-1, 0, QModelIndex());
    m_pCurveModel->setData(Xindex, pCurve->name());

    QModelIndex lineindex = m_pCurveModel->index(rowcount-1, 1, QModelIndex());
    QStandardItem *pItem = m_pCurveModel->itemFromIndex(lineindex);
    if(pItem) pItem->setData(QVariant::fromValue(pCurve->theStyle()), Qt::DisplayRole);

    QModelIndex actionindex = m_pCurveModel->index(rowcount-1, 2, QModelIndex());
    m_pCurveModel->setData(actionindex, QString("..."));

    m_pCurveModel->blockSignals(false);
    m_pcpCurveTable->resizeRowsToContents();
}


void StabTimeCtrls::onAddCurve()
{
    if(m_ResponseType==FORCEDRESPONSE)
    {
        QString strange = forcedResponseError(s_pXPlane->curPOpp());
        if(strange.length())
        {
            s_pXPlane->displayMessage(tr("Forced response not computed: ") + strange + "\n", true, true);
            return;
        }
    }

    addCurve();

    onPlotStabilityGraph();
    s_pXPlane->makeLegend();
}


void StabTimeCtrls::onDeleteCurve()
{
    Curve *pSelCurve = selectedCurve();
    if(!pSelCurve) return;
    QString curvename = pSelCurve->name();
    for(int ig=0; ig<s_pXPlane->m_TimeGraph.size(); ig++) s_pXPlane->m_TimeGraph[ig]->deleteCurve(curvename);

    int row=0;
    for(row=0; row<m_pCurveModel->rowCount(); row++)
    {
        QStandardItem *pItem = m_pCurveModel->item(row);
        if(pItem->text().compare(curvename)==0)
        {
            m_pCurveModel->removeRow(row);
            break;
        }
    }

    m_pcpCurveTable->setEnabled(    m_pCurveModel->rowCount());

    QModelIndex index = m_pcpCurveTable->currentIndex();
    if(index.isValid())
    {
        int newrow = index.row();
        QStandardItem *pItem = m_pCurveModel->item(newrow);
        if(pItem) curvename = pItem->text();
        else      curvename.clear();
    }

    s_pXPlane->makeLegend();
    s_pXPlane->updateView();
}


void StabTimeCtrls::fillCurveList()
{
     m_pCurveModel->setRowCount(0);
    for(int i=0; i<s_pXPlane->m_TimeGraph.at(0)->curveCount(); i++)
    {
        appendRow(s_pXPlane->m_TimeGraph.at(0)->curve(i));
    }
}


double StabTimeCtrls::getControlInput(const double &time) const
{
    BSpline const &spline = m_pSplGraphWt->spline();
    if(time<0.0 || spline.ctrlPointCount()==0) return 0.0;

    Node2d const &first = spline.controlPoint(0);
    Node2d const &last  = spline.lastCtrlPoint();
    if(time<first.x) return 0.0;
    if(time>=last.x) return m_bHoldLastInput ? last.y : 0.0;

    if(m_bLinearInput)
    {
        // exact linear interpolation between the points; points at equal times make a step
        for(int ic=1; ic<spline.ctrlPointCount(); ic++)
        {
            Node2d const &p0 = spline.controlPoint(ic-1);
            Node2d const &p1 = spline.controlPoint(ic);
            if(p0.x<=time && time<p1.x)
                return p0.y + (time-p0.x)/(p1.x-p0.x) * (p1.y-p0.y);
        }
        return 0.0;
    }

    for(int io=1; io<spline.outputSize(); io++)
    {
        if(spline.outputPt(io-1).x<=time && time<spline.outputPt(io).x)
        {
            double tau = (time - spline.outputPt(io-1).x)/(spline.outputPt(io).x-spline.outputPt(io-1).x);
            return spline.outputPt(io-1).y + tau * (spline.outputPt(io).y-spline.outputPt(io-1).y);
        }
    }
    return 0.0;
}


/** Copies the input points into the table */
void StabTimeCtrls::fillInputTable()
{
    BSpline const &spline = m_pSplGraphWt->spline();
    m_pInputModel->setRowCount(spline.ctrlPointCount());
    for(int ic=0; ic<spline.ctrlPointCount(); ic++)
    {
        m_pInputModel->setData(m_pInputModel->index(ic, 0), spline.controlPoint(ic).x);
        m_pInputModel->setData(m_pInputModel->index(ic, 1), spline.controlPoint(ic).y);
    }
}


/** Sets the input points sorted by time, keeping the order of points at equal times, and updates the graph and the table */
void StabTimeCtrls::setInputPoints(std::vector<Node2d> pts)
{
    for(Node2d &pt : pts) pt.x = std::max(pt.x, 0.0);
    std::stable_sort(pts.begin(), pts.end(), [](Node2d const &a, Node2d const &b){return a.x<b.x;});

    BSpline &spline = m_pSplGraphWt->spline();
    spline.setCtrlPoints(pts);
    spline.updateSpline();
    spline.makeCurve();
    m_pSplGraphWt->onResetGraphScales();
    fillInputTable();
}


void StabTimeCtrls::setInputInterpolation()
{
    BSpline &spline = m_pSplGraphWt->spline();
    spline.setDegree(m_bLinearInput ? 1 : 3);
    m_pSplGraphWt->setMonotonicX(m_bLinearInput);
    m_pInputModel->setHeaderData(1, Qt::Horizontal, m_bLinearInput ? tr("u (ctrl units)") : tr("ctrl point (ctrl units)"));

    std::vector<Node2d> pts;
    for(int ic=0; ic<spline.ctrlPointCount(); ic++) pts.push_back(spline.controlPoint(ic));
    setInputPoints(pts);
}


void StabTimeCtrls::onInputInterpolation()
{
    m_bLinearInput = m_prbInputLinear->isChecked();
    setInputInterpolation();
}


void StabTimeCtrls::onInputTableChanged()
{
    std::vector<Node2d> pts;
    for(int row=0; row<m_pInputModel->rowCount(); row++)
    {
        bool bOkt=false, bOku=false;
        double t = m_pInputModel->index(row, 0).data().toDouble(&bOkt);
        double u = m_pInputModel->index(row, 1).data().toDouble(&bOku);
        if(bOkt && bOku) pts.push_back(Node2d(t, u));
    }

    if(pts.size()<2)
    {
        s_pXPlane->displayMessage(tr("The control input needs at least two points; the table has been restored.\n"), true, true);
        fillInputTable();
        return;
    }
    setInputPoints(pts);
}


void StabTimeCtrls::onInputSplineModified()
{
    fillInputTable();
}


void StabTimeCtrls::onInputTableContextMenu(QPoint pos)
{
    int row = m_pcptInputTable->indexAt(pos).row();
    BSpline const &spline = m_pSplGraphWt->spline();
    std::vector<Node2d> pts;
    for(int ic=0; ic<spline.ctrlPointCount(); ic++) pts.push_back(spline.controlPoint(ic));

    QMenu menu(this);
    QAction *pInsertBefore = menu.addAction(tr("Insert before"));
    QAction *pInsertAfter  = menu.addAction(tr("Insert after"));
    QAction *pAppend       = menu.addAction(tr("Append"));
    QAction *pDelete       = menu.addAction(tr("Delete"));
    menu.addSeparator();
    QAction *pCopy         = menu.addAction(tr("Copy"));
    QAction *pPaste        = menu.addAction(tr("Paste"));
    bool bRow = row>=0 && row<int(pts.size());
    pInsertBefore->setEnabled(bRow);
    pInsertAfter->setEnabled(bRow);
    pDelete->setEnabled(bRow && pts.size()>2);

    QAction *pAction = menu.exec(m_pcptInputTable->viewport()->mapToGlobal(pos));
    if(!pAction) return;

    if(pAction==pCopy)  {m_pcptInputTable->copySelection(); return;}
    if(pAction==pPaste) {m_pcptInputTable->pasteClipboard(); onInputTableChanged(); return;}

    if(pAction==pDelete)
        pts.erase(pts.begin()+row);
    else if(pAction==pInsertBefore)
    {
        // midway between the previous point and this one, or at this point's time if it is the first
        Node2d pt = row>0 ? Node2d((pts[row-1].x+pts[row].x)/2.0, (pts[row-1].y+pts[row].y)/2.0) : pts[row];
        pts.insert(pts.begin()+row, pt);
    }
    else if(pAction==pInsertAfter)
    {
        Node2d pt = row<int(pts.size())-1 ? Node2d((pts[row].x+pts[row+1].x)/2.0, (pts[row].y+pts[row+1].y)/2.0) : Node2d(pts[row].x+1.0, pts[row].y);
        pts.insert(pts.begin()+row+1, pt);
    }
    else if(pAction==pAppend)
    {
        Node2d pt = pts.size() ? Node2d(pts.back().x+1.0, pts.back().y) : Node2d(0.0, 0.0);
        pts.push_back(pt);
    }
    setInputPoints(pts);
}


/** Shows the deflection which one control unit gives to each surface of the selected AVL-type control set */
void StabTimeCtrls::updateGainHint()
{
    PlanePolar const *pPolar = s_pXPlane->curPlPolar();
    int iAVLCtrl = m_pcbAVLControls->currentIndex();
    QStringList parts;
    if(pPolar && iAVLCtrl>=0 && iAVLCtrl<pPolar->nAVLCtrls())
    {
        for(ActiveFlap const &flap : activeFlaps())
        {
            double gain = pPolar->AVLGain(iAVLCtrl, flap.m_iGlobal);
            if(fabs(gain)>FLAPANGLEPRECISION)
                parts.append(QString("%1: %2%3").arg(flap.m_Name).arg(gain, 0, 'f', 3).arg(DEGch));
        }
    }
    if(parts.isEmpty()) m_plabGainHint->setText(tr("u = 1 deflects no surface."));
    else                m_plabGainHint->setText(tr("u = 1 adds, relative to trim: ") + parts.join(", "));
}


/** Returns the flaps of the current plane which are deflected in the current T7 polar,
 * either by the trim control (Flaps tab) or by at least one AVL-type control set. */
std::vector<StabTimeCtrls::ActiveFlap> StabTimeCtrls::activeFlaps() const
{
    std::vector<ActiveFlap> flaps;
    PlaneXfl const *pPlaneXfl = dynamic_cast<PlaneXfl const*>(s_pXPlane->curPlane());
    PlanePolar const *pPolar = s_pXPlane->curPlPolar();
    if(!pPlaneXfl || !pPolar || !pPolar->isType7()) return flaps;

    int iGlobal = 0;
    for(int iw=0; iw<pPlaneXfl->nWings(); iw++)
    {
        WingXfl const *pWing = pPlaneXfl->wingAt(iw);
        int iFlap = 0;
        for(int jSurf=0; jSurf<pWing->nSurfaces(); jSurf++)
        {
            Surface const &surf = pWing->surfaceAt(jSurf);
            if(!surf.hasTEFlap()) continue;

            bool bActive = iw<pPolar->nFlapCtrls() && fabs(pPolar->flapCtrls(iw).value(iFlap))>FLAPANGLEPRECISION;
            for(int ie=0; ie<pPolar->nAVLCtrls() && !bActive; ie++)
                bActive = fabs(pPolar->AVLGain(ie, iGlobal))>FLAPANGLEPRECISION;

            if(bActive)
            {
                ActiveFlap flap;
                flap.m_Name  = QString::fromStdString(pWing->name()) + QString::asprintf(" flap_%d", iFlap+1);
                flap.m_Label = DELTAch + " " + flap.m_Name + " (" + DEGch + ")";
                flap.m_iWing   = iw;
                flap.m_iFlap   = iFlap;
                flap.m_iGlobal = iGlobal;
                // same convention as the analysis log's "total flap angle"
                Foil const *pFoilA = surf.foilA();
                Foil const *pFoilB = surf.foilB();
                if(pFoilA && pFoilB && fabs(pFoilA->TEFlapAngle())>0.0 && fabs(pFoilB->TEFlapAngle())>0.0)
                    flap.m_GeomAngle = (pFoilA->TEFlapAngle()+pFoilB->TEFlapAngle())/2.0;
                flaps.push_back(flap);
            }
            iFlap++;
            iGlobal++;
        }
    }
    return flaps;
}


/** The flight parameters reconstructed from the trimmed state and the perturbations */
QStringList StabTimeCtrls::flightVariableNames(bool bLongitudinal)
{
    if(bLongitudinal)
        return {"V ("+Units::speedUnitQLabel()+")",
                ALPHAch+" ("+DEGch+")",
                THETAch+" pitch attitude ("+DEGch+")",
                GAMMAch+" flight path ("+DEGch+")",
                "q pitch rate ("+DEGch+"/s)",
                "n_z load factor (-)",
                DELTAch+"h height change ("+Units::lengthUnitQLabel()+")"};
    else
        return {BETAch+" ("+DEGch+")",
                PHIch+" bank angle ("+DEGch+")",
                "p body roll rate ("+DEGch+"/s)",
                "r body yaw rate ("+DEGch+"/s)",
                QString(QChar(0x03C8))+" heading change ("+DEGch+")",
                "n_y lateral load factor (-)"};
}


/** The graph variables offered after the default perturbation: flight parameters, then active flap angles */
QStringList StabTimeCtrls::extraVariableNames(bool bLongitudinal) const
{
    QStringList names = flightVariableNames(bLongitudinal);
    for(ActiveFlap const &flap : activeFlaps()) names.append(flap.m_Label);
    return names;
}


/** Computes the response for the current input and stores it under the curve's name */
void StabTimeCtrls::computeTimeResponse(PlaneOpp const*pPOpp, QString const &curvename)
{
    bool bForced = m_ResponseType==FORCEDRESPONSE;
    bool bLong   = isStabLongitudinal();

    TimeResponse response;
    std::vector<double> x[4]; // u,w,q,theta or v,p,r,phi, SI units, stability axes
    if(bForced) solveForcedResponse(      pPOpp, response.m_t, x);
    else        solvePerturbationResponse(pPOpp, response.m_t, x);

    std::vector<double> const &time = response.m_t;
    int n = int(time.size());

    // the perturbations, as displayed by default
    for(int i=0; i<n; i++)
    {
        response.m_State[0].push_back(x[0][i]*Units::mstoUnit());
        response.m_State[1].push_back(bLong ? x[1][i]*Units::mstoUnit() : x[1][i]*180.0/PI);
        response.m_State[2].push_back(x[2][i]*180.0/PI);
        response.m_State[3].push_back(x[3][i]*180.0/PI);
    }

    // control input applied to the state equations
    PlanePolar const *pPolar = s_pXPlane->curPlPolar();
    int iAVLCtrl = m_pcbAVLControls->currentIndex();
    double B[]{0,0,0,0};
    if(bForced && iAVLCtrl>=0)
    {
        if(bLong && iAVLCtrl<int(pPOpp->m_BLong.size())) for(int k=0; k<4; k++) B[k] = pPOpp->m_BLong.at(iAVLCtrl).at(k);
        if(!bLong && iAVLCtrl<int(pPOpp->m_BLat.size())) for(int k=0; k<4; k++) B[k] = pPOpp->m_BLat.at(iAVLCtrl).at(k);
    }
    auto derivative = [&](int row, int i)
    {
        double const (&A)[4][4] = bLong ? pPOpp->m_ALong : pPOpp->m_ALat;
        double d = B[row] * (bForced ? getControlInput(time[i]) : 0.0);
        for(int k=0; k<4; k++) d += A[row][k]*x[k][i];
        return d;
    };

    // flight parameters: the linear model is about level flight in stability axes,
    // so that theta0 = alpha0, gamma0 = 0 and q0 = p0 = r0 = phi0 = beta0 = 0
    double const g      = 9.81;
    double const V0     = pPOpp->QInf();
    double const alpha0 = pPOpp->alpha()*PI/180.0;
    QStringList names = flightVariableNames(bLong);
    std::vector<std::vector<double>> series(names.size());
    for(auto &v : series) v.reserve(n);

    double h=0.0, psi=0.0;
    double hdot_prev=0.0, r_prev=0.0;
    for(int i=0; i<n; i++)
    {
        double dt = i>0 ? time[i]-time[i-1] : 0.0;
        if(bLong)
        {
            double u=x[0][i], w=x[1][i], q=x[2][i], dtheta=x[3][i];
            double V     = sqrt((V0+u)*(V0+u) + w*w);
            double alpha = alpha0 + atan2(w, V0+u);
            double theta = alpha0 + dtheta;
            double gamma = theta - alpha;
            double nz    = 1.0 + (V0*q - derivative(1, i))/g;
            double hdot  = V*sin(gamma);
            if(i>0) h += 0.5*(hdot+hdot_prev)*dt;
            hdot_prev = hdot;

            series[0].push_back(V*Units::mstoUnit());
            series[1].push_back(alpha*180.0/PI);
            series[2].push_back(theta*180.0/PI);
            series[3].push_back(gamma*180.0/PI);
            series[4].push_back(q*180.0/PI);
            series[5].push_back(nz);
            series[6].push_back(h*Units::mtoUnit());
        }
        else
        {
            double v=x[0][i], p=x[1][i], r=x[2][i], phi=x[3][i];
            double beta  = V0>0.0 ? asin(std::clamp(v/V0, -1.0, 1.0)) : 0.0;
            double pbody = p*cos(alpha0) - r*sin(alpha0);
            double rbody = p*sin(alpha0) + r*cos(alpha0);
            if(i>0) psi += 0.5*(r+r_prev)*dt;
            r_prev = r;
            // specific force along y: v' + V0.r - g.phi, i.e. the side force over the weight
            double ny = (derivative(0, i) + V0*r)/g - phi;

            series[0].push_back(beta*180.0/PI);
            series[1].push_back(phi*180.0/PI);
            series[2].push_back(pbody*180.0/PI);
            series[3].push_back(rbody*180.0/PI);
            series[4].push_back(psi*180.0/PI);
            series[5].push_back(ny);
        }
    }
    for(int iv=0; iv<names.size(); iv++) response.m_Series[names.at(iv)] = series[iv];

    // flap angle = built-in angle + trim angle at the opp's control value + AVL gain x control input
    for(ActiveFlap const &flap : activeFlaps())
    {
        double trim = 0.0;
        if(pPolar && flap.m_iWing<pPolar->nFlapCtrls())
            trim = pPolar->flapCtrls(flap.m_iWing).value(flap.m_iFlap) * pPOpp->ctrl();
        double gain = (bForced && pPolar) ? pPolar->AVLGain(iAVLCtrl, flap.m_iGlobal) : 0.0;

        std::vector<double> &angle = response.m_Series[flap.m_Label];
        for(double t : time)
            angle.push_back(flap.m_GeomAngle + trim + gain*getControlInput(t));
    }

    m_TimeResponse[curvename] = response;
}


/** Fills every curve of the time graphs with the variable selected in each graph */
void StabTimeCtrls::fillTimeGraphCurves()
{
    QVector<Graph*> const &graphs = s_pXPlane->m_TimeGraph;
    if(graphs.isEmpty()) return;

    // discard the responses of deleted curves
    for(auto it=m_TimeResponse.begin(); it!=m_TimeResponse.end();)
    {
        if(!graphs.front()->curve(it.key())) it = m_TimeResponse.erase(it);
        else                                 ++it;
    }

    for(int ig=0; ig<graphs.size() && ig<4; ig++)
    {
        Graph *pGraph = graphs.at(ig);
        bool bState = pGraph->yVariable(0)<=0;
        QString varname = pGraph->yVariableName(0);
        for(int ic=0; ic<pGraph->curveCount(); ic++)
        {
            Curve *pCurve = pGraph->curve(ic);
            auto it = m_TimeResponse.constFind(pCurve->name());
            if(it==m_TimeResponse.constEnd()) continue;
            TimeResponse const &response = it.value();
            if(bState)
                pCurve->setPoints(response.m_t, response.m_State[ig]);
            else if(response.m_Series.contains(varname))
                pCurve->setPoints(response.m_t, response.m_Series.value(varname));
            else
                pCurve->clear(); // e.g. flap not active when this response was computed
        }
        pGraph->invalidate();
    }
}


void StabTimeCtrls::renameTimeResponse(QString const &oldname, QString const &newname)
{
    if(oldname==newname || !m_TimeResponse.contains(oldname)) return;
    m_TimeResponse[newname] = m_TimeResponse.take(oldname);
}


void StabTimeCtrls::solveForcedResponse(PlaneOpp const*pPOpp, std::vector<double> &time, std::vector<double> (&x)[4])
{
    // Builds the forced response from the state matrix and the forced input matrix
    // using a RK4 integration scheme.
    // The forced input is interpolated in the control history defined in the input table.

    if(pPOpp->m_BLong.size()==0) return;

    bool bLongitudinal = isStabLongitudinal();

    double A[4][4];    memset(A, 0, 16*sizeof(double));
    double m[5][4];    memset(m, 0, 20*sizeof(double));
    double B[]{0,0,0,0};
    double y[]{0,0,0,0};
    double yp[]{0,0,0,0};


    if(!pPOpp || !pPOpp->isType7()) return;//nothing to plot

    int iAVLCtrl = m_pcbAVLControls->currentIndex();
    if(iAVLCtrl<0||iAVLCtrl>=m_pcbAVLControls->count()) return;

    if(bLongitudinal)
    {
        memcpy(A, pPOpp->m_ALong, 4*4*sizeof(double));
        if(pPOpp->m_BLong.size())
        {
            B[0] = pPOpp->m_BLong.at(iAVLCtrl).at(0);
            B[1] = pPOpp->m_BLong.at(iAVLCtrl).at(1);
            B[2] = pPOpp->m_BLong.at(iAVLCtrl).at(2);
            B[3] = pPOpp->m_BLong.at(iAVLCtrl).at(3);
        }
    }
    else
    {
        memcpy(A, pPOpp->m_ALat, 4*4*sizeof(double));
        if(pPOpp->m_BLat.size())
        {
            B[0] = pPOpp->m_BLat.at(iAVLCtrl).at(0);
            B[1] = pPOpp->m_BLat.at(iAVLCtrl).at(1);
            B[2] = pPOpp->m_BLat.at(iAVLCtrl).at(2);
            B[3] = pPOpp->m_BLat.at(iAVLCtrl).at(3);
        }
    }

    m_Deltat    = deltaT();
    m_TotalTime = totalTime();

    double  dt = m_TotalTime/1000.;
    if(dt<m_Deltat) dt = m_Deltat;

    int TotalPoints  = std::min(1000, int(m_TotalTime/dt));
    int PlotInterval = std::max(1,    int(TotalPoints/200));

    // we are considering forced response from initial steady state, so set
    // initial conditions to 0
    double t=0.0, ctrl_t=0.0;
    y[0] = y[1] = y[2] = y[3] = 0.0;
    time.push_back(0.0);
    for(int iv=0; iv<4; iv++) x[iv].push_back(y[iv]);

    // RK4
    for(int i=0; i<TotalPoints; i++)
    {
        //initial slope m1
        m[0][0] = A[0][0]*y[0] + A[0][1]*y[1] + A[0][2]*y[2] + A[0][3]*y[3];
        m[0][1] = A[1][0]*y[0] + A[1][1]*y[1] + A[1][2]*y[2] + A[1][3]*y[3];
        m[0][2] = A[2][0]*y[0] + A[2][1]*y[1] + A[2][2]*y[2] + A[2][3]*y[3];
        m[0][3] = A[3][0]*y[0] + A[3][1]*y[1] + A[3][2]*y[2] + A[3][3]*y[3];

        ctrl_t = getControlInput(t);
        m[0][0] += B[0] * ctrl_t;
        m[0][1] += B[1] * ctrl_t;
        m[0][2] += B[2] * ctrl_t;
        m[0][3] += B[3] * ctrl_t;

        //middle point m2
        yp[0] = y[0] + dt/2.0 * m[0][0];
        yp[1] = y[1] + dt/2.0 * m[0][1];
        yp[2] = y[2] + dt/2.0 * m[0][2];
        yp[3] = y[3] + dt/2.0 * m[0][3];

        m[1][0] = A[0][0]*yp[0] + A[0][1]*yp[1] + A[0][2]*yp[2] + A[0][3]*yp[3];
        m[1][1] = A[1][0]*yp[0] + A[1][1]*yp[1] + A[1][2]*yp[2] + A[1][3]*yp[3];
        m[1][2] = A[2][0]*yp[0] + A[2][1]*yp[1] + A[2][2]*yp[2] + A[2][3]*yp[3];
        m[1][3] = A[3][0]*yp[0] + A[3][1]*yp[1] + A[3][2]*yp[2] + A[3][3]*yp[3];

        ctrl_t = getControlInput(t+dt/2.0);
        m[1][0] += B[0] * ctrl_t;
        m[1][1] += B[1] * ctrl_t;
        m[1][2] += B[2] * ctrl_t;
        m[1][3] += B[3] * ctrl_t;

        //second point m3
        yp[0] = y[0] + dt/2.0 * m[1][0];
        yp[1] = y[1] + dt/2.0 * m[1][1];
        yp[2] = y[2] + dt/2.0 * m[1][2];
        yp[3] = y[3] + dt/2.0 * m[1][3];

        m[2][0] = A[0][0]*yp[0] + A[0][1]*yp[1] + A[0][2]*yp[2] + A[0][3]*yp[3];
        m[2][1] = A[1][0]*yp[0] + A[1][1]*yp[1] + A[1][2]*yp[2] + A[1][3]*yp[3];
        m[2][2] = A[2][0]*yp[0] + A[2][1]*yp[1] + A[2][2]*yp[2] + A[2][3]*yp[3];
        m[2][3] = A[3][0]*yp[0] + A[3][1]*yp[1] + A[3][2]*yp[2] + A[3][3]*yp[3];

        ctrl_t = getControlInput(t+dt/2.0);

        m[2][0] += B[0] * ctrl_t;
        m[2][1] += B[1] * ctrl_t;
        m[2][2] += B[2] * ctrl_t;
        m[2][3] += B[3] * ctrl_t;

        //third point m4
        yp[0] = y[0] + dt * m[2][0];
        yp[1] = y[1] + dt * m[2][1];
        yp[2] = y[2] + dt * m[2][2];
        yp[3] = y[3] + dt * m[2][3];

        m[3][0] = A[0][0]*yp[0] + A[0][1]*yp[1] + A[0][2]*yp[2] + A[0][3]*yp[3];
        m[3][1] = A[1][0]*yp[0] + A[1][1]*yp[1] + A[1][2]*yp[2] + A[1][3]*yp[3];
        m[3][2] = A[2][0]*yp[0] + A[2][1]*yp[1] + A[2][2]*yp[2] + A[2][3]*yp[3];
        m[3][3] = A[3][0]*yp[0] + A[3][1]*yp[1] + A[3][2]*yp[2] + A[3][3]*yp[3];

        ctrl_t = getControlInput(t+dt);

        m[3][0] += B[0] * ctrl_t;
        m[3][1] += B[1] * ctrl_t;
        m[3][2] += B[2] * ctrl_t;
        m[3][3] += B[3] * ctrl_t;

        //final slope m5
        m[4][0] = 1./6. * (m[0][0] + 2.0*m[1][0] + 2.0*m[2][0] + m[3][0]);
        m[4][1] = 1./6. * (m[0][1] + 2.0*m[1][1] + 2.0*m[2][1] + m[3][1]);
        m[4][2] = 1./6. * (m[0][2] + 2.0*m[1][2] + 2.0*m[2][2] + m[3][2]);
        m[4][3] = 1./6. * (m[0][3] + 2.0*m[1][3] + 2.0*m[2][3] + m[3][3]);

        y[0] += m[4][0] * dt;
        y[1] += m[4][1] * dt;
        y[2] += m[4][2] * dt;
        y[3] += m[4][3] * dt;
        t +=dt;
        if(fabs(y[0])>1.e10 || fabs(y[1])>1.e10 || fabs(y[2])>1.e10  || fabs(y[3])>1.e10 ) break;

        if(i%PlotInterval==0)
        {
            time.push_back(t);
            for(int iv=0; iv<4; iv++) x[iv].push_back(y[iv]); // SI units
        }
    }
}


void StabTimeCtrls::solvePerturbationResponse(PlaneOpp const*pPOpp, std::vector<double> &time, std::vector<double> (&x)[4])
{
    // The time response is calculated analytically based on the eigenvalues and eigenvectors
    std::complex<double> M[16];// the modal matrix
    std::complex<double> InvM[16];// the inverse of the modal matrix
    std::complex<double> q[4],q0[4],y[4];//the part of each mode in the solution

    std::complex<double> in[4];

    if(!pPOpp || !pPOpp->isType7()) return;

    bool bLongitudinal = isStabLongitudinal();

    m_Deltat    = deltaT();
    m_TotalTime = totalTime();
    double dt = m_TotalTime/1000.;
    if(dt<m_Deltat) dt = m_Deltat;

    int TotalPoints = std::min(1000, int(m_TotalTime/dt));
    //read the initial state condition
    m_TimeInput[0] = m_pfeStabVar1->value();
    m_TimeInput[1] = m_pfeStabVar2->value();
    m_TimeInput[2] = m_pfeStabVar3->value();
    m_TimeInput[3] = 0.0;//we start with an initial 0.0 value for pitch or bank angles

    if(m_ResponseType==INITIALCONDITIONS)
    {
        //start with the user input initial conditions, converted from display units to SI
        in[0] = std::complex<double>(m_TimeInput[0]/Units::mstoUnit(), 0.0);
        if(bLongitudinal) in[1] = std::complex<double>(m_TimeInput[1]/Units::mstoUnit(), 0.0);
        else              in[1] = std::complex<double>(m_TimeInput[1]*PI/180.0, 0.0);
        in[2] = std::complex<double>(m_TimeInput[2]*PI/180.0, 0.0);
        in[3] = std::complex<double>(m_TimeInput[3]*PI/180.0, 0.0);
    }
    else if(m_ResponseType==MODALRESPONSE)
    {
        //start with the initial conditions which will excite only the requested mode
        in[0] = pPOpp->m_EigenVector[m_iCurrentMode][0];
        in[1] = pPOpp->m_EigenVector[m_iCurrentMode][1];
        in[2] = pPOpp->m_EigenVector[m_iCurrentMode][2];
        in[3] = pPOpp->m_EigenVector[m_iCurrentMode][3];
    }
    else return;

    //fill the modal matrix
    int k=0;
    if(bLongitudinal) k=0; else k=1;
    for (int i=0; i<4; i++)
    {
        for(int j=0;j<4;j++)
        {
            *(M+4*j+i) = pPOpp->m_EigenVector[k*4+i][j];
        }
    }

    //Invert the matrix
    if(!matrix::invert44(M, InvM))
    {
    }
    else
    {
        //calculate the modal coefficients at t=0
        q0[0] = InvM[0] * in[0] + InvM[1] * in[1] + InvM[2] * in[2] + InvM[3] * in[3];
        q0[1] = InvM[4] * in[0] + InvM[5] * in[1] + InvM[6] * in[2] + InvM[7] * in[3];
        q0[2] = InvM[8] * in[0] + InvM[9] * in[1] + InvM[10]* in[2] + InvM[11]* in[3];
        q0[3] = InvM[12]* in[0] + InvM[13]* in[1] + InvM[14]* in[2] + InvM[15]* in[3];

        for(int i=0; i<TotalPoints; i++)
        {
            double t = i * dt;
            q[0] = q0[0] * exp(pPOpp->m_EigenValue[0+k*4]*t);
            q[1] = q0[1] * exp(pPOpp->m_EigenValue[1+k*4]*t);
            q[2] = q0[2] * exp(pPOpp->m_EigenValue[2+k*4]*t);
            q[3] = q0[3] * exp(pPOpp->m_EigenValue[3+k*4]*t);
            y[0] = *(M+4*0+0) * q[0] +*(M+4*0+1) * q[1] +*(M+4*0+2) * q[2] +*(M+4*0+3) * q[3];
            y[1] = *(M+4*1+0) * q[0] +*(M+4*1+1) * q[1] +*(M+4*1+2) * q[2] +*(M+4*1+3) * q[3];
            y[2] = *(M+4*2+0) * q[0] +*(M+4*2+1) * q[1] +*(M+4*2+2) * q[2] +*(M+4*2+3) * q[3];
            y[3] = *(M+4*3+0) * q[0] +*(M+4*3+1) * q[1] +*(M+4*3+2) * q[2] +*(M+4*3+3) * q[3];
            if(std::abs(q[0])>1.e10 || std::abs(q[1])>1.e10 || std::abs(q[2])>1.e10  || std::abs(q[3])>1.e10 ) break;

            time.push_back(t);
            for(int iv=0; iv<4; iv++) x[iv].push_back(y[iv].real()); // SI units
        }
    }
}


void StabTimeCtrls::loadSettings(QSettings &settings)
{
    settings.beginGroup("StabTimeControls");
    {
        QString strong;
        int functionsize = settings.value("timefunctsize", 5).toInt();
        std::vector<Node2d> pts(functionsize);
        for(int i=0; i<functionsize; i++)
        {
            strong = QString("ForcedTime%1").arg(i);
            pts[i].x =settings.value(strong, double(i)).toDouble();
        }
        for(int i=0; i<functionsize; i++)
        {
            strong = QString("ForcedAmplitude%1").arg(i);
            pts[i].y =settings.value(strong, double(i)).toDouble();
        }
        m_pSplGraphWt->spline().setCtrlPoints(pts);

        m_bLinearInput      = settings.value("LinearInput",   m_bLinearInput).toBool();
        m_bHoldLastInput    = settings.value("HoldLastInput", m_bHoldLastInput).toBool();
        m_prbInputLinear->setChecked(m_bLinearInput);
        m_prbInputSmooth->setChecked(!m_bLinearInput);
        m_pchHoldLastInput->setChecked(m_bHoldLastInput);
        setInputInterpolation();

        m_TotalTime         = settings.value("TotalTime",1.0).toDouble();
        m_Deltat            = settings.value("Delta_t",0.001).toDouble();

        m_TimeInput[0]      = settings.value("TimeIn0",0.0).toDouble();
        m_TimeInput[1]      = settings.value("TimeIn1",0.0).toDouble();
        m_TimeInput[2]      = settings.value("TimeIn2",0.0).toDouble();
        m_TimeInput[3]      = settings.value("TimeIn3",0.0).toDouble();

    }
    settings.endGroup();
}


void StabTimeCtrls::saveSettings(QSettings &settings)
{
    settings.beginGroup("StabTimeControls");
    {
        QString strong;
        BSpline const &bSpline = m_pSplGraphWt->spline();
        settings.setValue("timefunctsize", bSpline.ctrlPointCount());

        for(int i=0; i<int(bSpline.ctrlPointCount()); i++)
        {
            strong = QString("ForcedTime%1").arg(i);
            settings.setValue(strong, bSpline.controlPoint(i).x);
        }
        for(int i=0; i<int(bSpline.ctrlPointCount()); i++)
        {
            strong = QString("ForcedAmplitude%1").arg(i);
            settings.setValue(strong, bSpline.controlPoint(i).y);
        }
        settings.setValue("LinearInput",   m_bLinearInput);
        settings.setValue("HoldLastInput", m_bHoldLastInput);
        settings.setValue("TotalTime", m_TotalTime);
        settings.setValue("Delta_t", m_Deltat);

        settings.setValue("TimeIn0", m_TimeInput[0]);
        settings.setValue("TimeIn1", m_TimeInput[1]);
        settings.setValue("TimeIn2", m_TimeInput[2]);
        settings.setValue("TimeIn3", m_TimeInput[3]);
    }
    settings.endGroup();
}
