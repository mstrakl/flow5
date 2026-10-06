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

#include <QWidget>
#include <QLabel>
#include <QRadioButton>
#include <QPushButton>
#include <QModelIndex>
#include <QComboBox>
#include <QStackedWidget>
#include <QMap>

#include <interfaces/graphs/graph/graph.h>
#include <interfaces/graphs/containers/splinedgraphwt.h>
#include <interfaces/widgets/customwts/floatedit.h>

class XPlane;
class FloatEdit;
class Curve;
class PlaneOpp;
class PlanePolar;

class CPTableView;
class XflDelegate;
class QCheckBox;
class QStandardItemModel;
class ActionItemModel;

class StabTimeCtrls : public QFrame
{
    Q_OBJECT

    public:
        StabTimeCtrls(QWidget *pParent=nullptr);
        ~StabTimeCtrls()  override;

        QString selectedCurveName();

        void selectCurve(Curve const *pCurve);

        void setControls();
        void fillAVLcontrols(PlanePolar const*pWPolar);
        QString forcedResponseError(PlaneOpp const *pPOpp) const;

        bool isStabLongitudinal() const {return m_prbLongitudinal->isChecked();}

        void computeTimeResponse(PlaneOpp const*pPOpp, QString const &curvename);
        void fillTimeGraphCurves();
        QStringList extraVariableNames(bool bLongitudinal) const;
        void setMode(int iMode=-1);

        double deltaT() const {return m_pfeDeltat->value();}
        double totalTime() const {return m_pfeTotalTime->value();}

        void loadSettings(QSettings &settings);
        void saveSettings(QSettings &settings);

        static void setXPlane(XPlane *pXPlane) {s_pXPlane = pXPlane;}

    private:
        QSize sizeHint() const  override {return QSize(50,50);}
        void showEvent(QShowEvent *pEvent)  override;
        void resizeEvent(QResizeEvent *pEvent)  override;
        void keyPressEvent(QKeyEvent *pEvent)  override;
        void makeTable();
        void setupLayout();
        void connectSignals();
        void appendRow(const Curve *pCurve);

        double getControlInput(const double &time) const;
        void solveForcedResponse(PlaneOpp const *pPOpp, std::vector<double> &time, std::vector<double> (&x)[4]);
        void solvePerturbationResponse(PlaneOpp const *pPOpp, std::vector<double> &time, std::vector<double> (&x)[4]);
        void addCurve();
        void fillCurveList();
        Curve *selectedCurve();
        void renameTimeResponse(QString const &oldname, QString const &newname);

        void fillInputTable();
        void setInputPoints(std::vector<Node2d> pts);
        void setInputInterpolation();
        void updateGainHint();

        /** a control surface which is deflected in the T7 polar, either by the trim control or by an AVL-type control set */
        struct ActiveFlap
        {
            QString m_Name;       /**< wing name and flap number */
            QString m_Label;      /**< the graph variable name */
            int m_iWing{0};       /**< the wing index in the plane */
            int m_iFlap{0};       /**< the flap index in the wing */
            int m_iGlobal{0};     /**< the flap index in the plane, used by the AVL-type controls */
            double m_GeomAngle{0}; /**< the flap angle built into the wing's foils, in degrees */
        };
        std::vector<ActiveFlap> activeFlaps() const;
        static QStringList flightVariableNames(bool bLongitudinal);

        /** the time history of a response curve, cached so that each graph can display any of its variables */
        struct TimeResponse
        {
            std::vector<double> m_t;
            std::vector<double> m_State[4];                       /**< u,w,q,theta or v,p,r,phi, in display units */
            QMap<QString, std::vector<double>> m_Series;          /**< flight parameters and flap angles in display units, keyed by the graph variable name */
        };
        QMap<QString, TimeResponse> m_TimeResponse; /**< keyed by curve name */

    private slots:
        void onDataChanged(QModelIndex,QModelIndex);
        void onCurveTableClicked(QModelIndex index);
        void onModeSelection();
        void onPlotStabilityGraph();
        void onReadData();
        void onResponseType();
        void onAddCurve();
        void onDeleteCurve();
        void onRenameCurve();
        void onSelChangeCurve(int sel);
        void onStabilityDirection();
        void onResizeColumns();
        void onInputTableChanged();
        void onInputSplineModified();
        void onInputInterpolation();
        void onInputTableContextMenu(QPoint pos);

    private:

        static XPlane *s_pXPlane;

        QRadioButton *m_prbTimeMode[4];

        QLabel *m_plabStab1, *m_plabStab2, *m_plabStab3;
        FloatEdit  *m_pfeStabVar1, *m_pfeStabVar2, *m_pfeStabVar3;
        FloatEdit *m_pfeTotalTime, *m_pfeDeltat;
        QPushButton *m_ppbAddCurve;

        CPTableView *m_pcpCurveTable;
        ActionItemModel *m_pCurveModel;

        QComboBox *m_pcbAVLControls;

        QLabel *m_plabUnit1, *m_plabUnit2, *m_plabUnit3;
        QStackedWidget *m_pswInitialConditions;

        QRadioButton *m_prbModalResponse, *m_prbInitCondResponse, *m_prbForcedResponse;

        QAction *m_pRenameAct, *m_pDeleteAct, *m_pMoveUp, *m_pMoveDown;


        QRadioButton *m_prbLongitudinal, *m_prbLateral;

        int m_iCurrentMode;

        Graph m_InputGraph;
        SplinedGraphWt *m_pSplGraphWt;

        // control input table
        CPTableView *m_pcptInputTable;
        QStandardItemModel *m_pInputModel;
        XflDelegate *m_pInputDelegate;
        QRadioButton *m_prbInputLinear, *m_prbInputSmooth;
        QCheckBox *m_pchHoldLastInput;
        QLabel *m_plabGainHint;
        bool m_bLinearInput;     /**< if true, the control input is interpolated linearly between the points; otherwise it is the B-spline */
        bool m_bHoldLastInput;   /**< if true, the control input keeps the last point's value after the last point's time */

        // time curve data
        double m_TimeInput[4];
        double m_TotalTime;
        double m_Deltat;

        enum enumStabTimeResponse {MODALRESPONSE, INITIALCONDITIONS, FORCEDRESPONSE};
        enumStabTimeResponse m_ResponseType;

};

