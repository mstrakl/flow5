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


#include <QVBoxLayout>
#include <QButtonGroup>
#include <QCheckBox>
#include <QFileInfo>
#include <QFileDialog>


#include "wingexportdlg.h"

#include <api/units.h>
#include <api/wingxfl.h>
#include <api/occ_globals.h>
#include <api/flow5-io.h>

#include <core/xflcore.h>
#include <core/saveoptions.h>
#include <interfaces/widgets/customwts/intedit.h>
#include <interfaces/widgets/customwts/floatedit.h>
#include <interfaces/widgets/customwts/plaintextoutput.h>

int WingExportDlg::s_SurfaceType=0;
int WingExportDlg::s_iChordRes=37;

double WingExportDlg::s_StitchPrecision(1.e-4);
int WingExportDlg::s_SplineDegree(3);
int WingExportDlg::s_nSplineCtrlPts(11);

bool WingExportDlg::s_bSecFaces(true);
bool WingExportDlg::s_bSecSplines(true);
bool WingExportDlg::s_bSecRightHalf(false);
bool WingExportDlg::s_bSecPlaneFrame(false);


WingExportDlg::WingExportDlg(QWidget *pParent) : CADExportDlg(pParent)
{
    setWindowTitle("Wing export");
    setupLayout();
}


void WingExportDlg::setupLayout()
{
    QFrame *pWingFrame = new QFrame;
    {
       QVBoxLayout *pWingLayout = new QVBoxLayout;
       {
            QHBoxLayout *pExportTypeLayout = new QHBoxLayout;
            {
                QLabel *plabType = new QLabel(tr("<b>Wing surfaces as:</b>"));
                m_prbFacets = new QRadioButton(tr("Facets"));
                m_prbNURBS  = new QRadioButton("NURBS");
                m_prbNURBS->setToolTip(tr("flow5 will create one NURBS for each top and bottom surface between two wing sections"));
                m_prbSwept  = new QRadioButton(tr("Swept splines"));
                m_prbSwept->setToolTip(tr("flow5 will first convert wing sections to splines,<br>then create a swept surface between the splines"));
                m_prbSections = new QRadioButton(tr("Section profiles"));
                m_prbSections->setToolTip(tr("flow5 will export only the airfoil profile of each wing section,<br>"
                                             "in its position in the wing: offset, twist and dihedral included"));

                m_prbFacets->setChecked(s_SurfaceType==0);
                m_prbNURBS->setChecked(s_SurfaceType==1);
                m_prbSwept->setChecked(s_SurfaceType==2);
                m_prbSections->setChecked(s_SurfaceType==3);

                connect(m_prbFacets,   SIGNAL(clicked(bool)), SLOT(onExportType()));
                connect(m_prbNURBS,    SIGNAL(clicked(bool)), SLOT(onExportType()));
                connect(m_prbSwept,    SIGNAL(clicked(bool)), SLOT(onExportType()));
                connect(m_prbSections, SIGNAL(clicked(bool)), SLOT(onExportType()));

                pExportTypeLayout->addWidget(plabType);
                pExportTypeLayout->addWidget(m_prbFacets);
                pExportTypeLayout->addWidget(m_prbNURBS);
                pExportTypeLayout->addWidget(m_prbSwept);
                pExportTypeLayout->addWidget(m_prbSections);
                pExportTypeLayout->addStretch();
            }

            m_pfrSections = new QFrame;
            {
                QGridLayout *pSectionsLayout = new QGridLayout;
                {
                    m_prbSecFaces    = new QRadioButton(tr("Faces"));
                    m_prbSecFaces->setToolTip(tr("One planar face bounded by the airfoil outline for each section"));
                    m_prbSecOutlines = new QRadioButton(tr("Outlines"));
                    m_prbSecOutlines->setToolTip(tr("Only the closed airfoil curve of each section"));
                    QButtonGroup *pShapeGroup = new QButtonGroup(this);
                    pShapeGroup->addButton(m_prbSecFaces);
                    pShapeGroup->addButton(m_prbSecOutlines);

                    m_prbSecSplines   = new QRadioButton(tr("Splines"));
                    m_prbSecSplines->setToolTip(tr("Top and bottom splines fitted through the chordwise points,<br>"
                                                   "using the spline degree and number of control points below"));
                    m_prbSecPolylines = new QRadioButton(tr("Polylines"));
                    m_prbSecPolylines->setToolTip(tr("Straight segments between the chordwise points"));
                    QButtonGroup *pCurveGroup = new QButtonGroup(this);
                    pCurveGroup->addButton(m_prbSecSplines);
                    pCurveGroup->addButton(m_prbSecPolylines);

                    m_prbSecWingFrame  = new QRadioButton(tr("Wing frame"));
                    m_prbSecWingFrame->setToolTip(tr("Coordinates relative to the wing's root leading edge,<br>as in the other export types"));
                    m_prbSecPlaneFrame = new QRadioButton(tr("Plane frame"));
                    m_prbSecPlaneFrame->setToolTip(tr("Coordinates in the plane, i.e. including the wing's position and tilt angles"));
                    QButtonGroup *pFrameGroup = new QButtonGroup(this);
                    pFrameGroup->addButton(m_prbSecWingFrame);
                    pFrameGroup->addButton(m_prbSecPlaneFrame);
                    m_plabPlanePos = new QLabel;

                    m_pchSecRightHalf = new QCheckBox(tr("Right half only"));
                    m_pchSecRightHalf->setToolTip(tr("For two-sided wings, export only the root section and the sections on the right side"));

                    m_prbSecFaces->setChecked(s_bSecFaces);
                    m_prbSecOutlines->setChecked(!s_bSecFaces);
                    m_prbSecSplines->setChecked(s_bSecSplines);
                    m_prbSecPolylines->setChecked(!s_bSecSplines);
                    m_prbSecWingFrame->setChecked(!s_bSecPlaneFrame);
                    m_prbSecPlaneFrame->setChecked(s_bSecPlaneFrame);
                    m_pchSecRightHalf->setChecked(s_bSecRightHalf);

                    connect(m_prbSecSplines,   SIGNAL(clicked(bool)), SLOT(onExportType()));
                    connect(m_prbSecPolylines, SIGNAL(clicked(bool)), SLOT(onExportType()));

                    pSectionsLayout->addWidget(new QLabel(tr("Export as:")),     1, 1);
                    pSectionsLayout->addWidget(m_prbSecFaces,                    1, 2);
                    pSectionsLayout->addWidget(m_prbSecOutlines,                 1, 3);
                    pSectionsLayout->addWidget(new QLabel(tr("Profile curves:")),2, 1);
                    pSectionsLayout->addWidget(m_prbSecSplines,                  2, 2);
                    pSectionsLayout->addWidget(m_prbSecPolylines,                2, 3);
                    pSectionsLayout->addWidget(new QLabel(tr("Coordinates:")),   3, 1);
                    pSectionsLayout->addWidget(m_prbSecWingFrame,                3, 2);
                    pSectionsLayout->addWidget(m_prbSecPlaneFrame,               3, 3);
                    pSectionsLayout->addWidget(m_plabPlanePos,                   3, 4);
                    pSectionsLayout->addWidget(m_pchSecRightHalf,                4, 2, 1, 2);
                    pSectionsLayout->setColumnStretch(5,1);
                }
                m_pfrSections->setLayout(pSectionsLayout);
            }

            QGridLayout *pCommonLayout = new QGridLayout;
            {
                QLabel *plabRes = new QLabel(tr("Chordwise points:"));
                m_pieChordRes = new IntEdit(s_iChordRes);
                m_pieChordRes->setToolTip("This parameter defines the discretization level of each wing section before it is used "
                                          "to build facets or as the base points on which the splines are built.<br>"
                                          "<b>flow5 default setting:</b> 30");

                QLabel *plabStitch   = new QLabel(tr("Stitch precision:"));
                m_pfeStitchPrecision = new FloatEdit(s_StitchPrecision*Units::mtoUnit());
                m_pfeStitchPrecision->setToolTip("<b>OpenCascade documentation:</b><br>"
                                                 "The working tolerance defines the maximal distance between topological elements "
                                                 "which can be sewn. It is not ultimate that such elements will be actually sewn "
                                                 "as many other criteria are applied to make the final decision.<br>"
                                                 "<b>flow5 default setting:</b> &le; 0.1 mm");
                QLabel *plabLen      = new QLabel(Units::lengthUnitQLabel());

                QString tip(tr("These parameters control the end splines on which the NURBS or swept surfaces are built.<br>"
                            "The splines are constructed as approximations of the wing's chordwise points.<br>"
                            "<b>flow5 default settings:</b><br>"
                            "degree = 3<br>"
                            "nbr. of ctrl points = 11"));
                QLabel *plabDegree   = new QLabel(tr("Spline degree:"));
                m_pieSplineDegre     = new IntEdit(s_SplineDegree);
                m_pieSplineDegre->setToolTip(tip);
                QLabel *plabCtrl     = new QLabel(tr("Nbr. of spline ctrl points:"));
                m_pieSplineCtrlPts   = new IntEdit(s_nSplineCtrlPts);
                m_pieSplineCtrlPts->setToolTip(tip);

                pCommonLayout->addWidget(plabRes,               1, 1);
                pCommonLayout->addWidget(m_pieChordRes,         1, 2);
                pCommonLayout->addWidget(plabStitch,            2, 1);
                pCommonLayout->addWidget(m_pfeStitchPrecision,  2, 2);
                pCommonLayout->addWidget(plabLen,               2, 3);

                pCommonLayout->addWidget(plabDegree,            3, 1);
                pCommonLayout->addWidget(m_pieSplineDegre,      3, 2);
                pCommonLayout->addWidget(plabCtrl,              4, 1);
                pCommonLayout->addWidget(m_pieSplineCtrlPts,    4, 2);

                pCommonLayout->setColumnStretch(4,1);
            }


            pWingLayout->addLayout(pExportTypeLayout);
            pWingLayout->addWidget(m_pfrSections);
            pWingLayout->addLayout(pCommonLayout);
       }
        pWingFrame->setLayout(pWingLayout);
    }

    QVBoxLayout *pMainLayout = new QVBoxLayout;
    {
        pMainLayout->addWidget(pWingFrame);
        pMainLayout->addWidget(m_pfrControls);
        pMainLayout->addWidget(m_ppto);
        pMainLayout->addWidget(m_pButtonBox);
;
        pMainLayout->setStretchFactor(m_pfrControls, 1);
        pMainLayout->setStretchFactor(m_ppto,  1);
    }
    setLayout(pMainLayout);
}


void WingExportDlg::onExportType()
{
    if(m_prbFacets->isChecked())
    {
        s_SurfaceType=0;
    }
    else if(m_prbNURBS->isChecked())
    {
        s_SurfaceType=1;
    }
    else if(m_prbSwept->isChecked())
    {
        s_SurfaceType=2;
    }
    else if(m_prbSections->isChecked())
    {
        s_SurfaceType=3;
    }

    bool bSections = s_SurfaceType==3;
    bool bSplines = (s_SurfaceType==1 || s_SurfaceType==2) || (bSections && m_prbSecSplines->isChecked());
    m_pfrSections->setVisible(bSections);
    m_pieSplineDegre->setEnabled(bSplines);
    m_pieSplineCtrlPts->setEnabled(bSplines);
    m_pfeStitchPrecision->setEnabled(!bSections); // nothing is sewn
    m_pchSecRightHalf->setEnabled(m_pWing && m_pWing->isTwoSided());
    readParams();
}


void WingExportDlg::hideEvent(QHideEvent*pEvent)
{
    onExportType();
    CADExportDlg::hideEvent(pEvent);
}


void WingExportDlg::init(WingXfl const*pWing)
{
    m_PartName = QString::fromStdString(pWing->name());
    m_pWing = pWing;

    Vector3d const &LE = pWing->position();
    m_plabPlanePos->setText(QString::asprintf("LE = (%g, %g, %g) ", LE.x*Units::mtoUnit(), LE.y*Units::mtoUnit(), LE.z*Units::mtoUnit())
                            + Units::lengthUnitQLabel()
                            + QString::asprintf(",  rx = %g°,  ry = %g°", pWing->rx(), pWing->ry()));

    onExportType();
}


void WingExportDlg::loadSettings(QSettings &settings)
{
    settings.beginGroup("WingExportDlg");
    {
        s_SurfaceType     = settings.value("SurfaceType",     s_SurfaceType).toInt();
        s_iChordRes       = settings.value("iChordRes",       s_iChordRes).toInt();
        s_SplineDegree    = settings.value("SplineDegree",    s_SplineDegree).toInt();
        s_nSplineCtrlPts  = settings.value("SplineCtrlPts",   s_nSplineCtrlPts).toInt();
        s_StitchPrecision = settings.value("StitchPrecision", s_StitchPrecision).toDouble();
        s_bSecFaces       = settings.value("SectionFaces",    s_bSecFaces).toBool();
        s_bSecSplines     = settings.value("SectionSplines",  s_bSecSplines).toBool();
        s_bSecRightHalf   = settings.value("SectionRightHalf",  s_bSecRightHalf).toBool();
        s_bSecPlaneFrame  = settings.value("SectionPlaneFrame", s_bSecPlaneFrame).toBool();
    }
    settings.endGroup();
}


void WingExportDlg::saveSettings(QSettings &settings)
{
    settings.beginGroup("WingExportDlg");
    {
        settings.setValue("SurfaceType",     s_SurfaceType);
        settings.setValue("iChordRes",       s_iChordRes);
        settings.setValue("SplineDegree",    s_SplineDegree);
        settings.setValue("SplineCtrlPts",   s_nSplineCtrlPts);
        settings.setValue("StitchPrecision", s_StitchPrecision);
        settings.setValue("SectionFaces",    s_bSecFaces);
        settings.setValue("SectionSplines",  s_bSecSplines);
        settings.setValue("SectionRightHalf",  s_bSecRightHalf);
        settings.setValue("SectionPlaneFrame", s_bSecPlaneFrame);
    }
    settings.endGroup();
}


void WingExportDlg::readParams()
{
    s_iChordRes = m_pieChordRes->value();

    s_SplineDegree    = m_pieSplineDegre->value();
    s_nSplineCtrlPts  = m_pieSplineCtrlPts->value();
    s_StitchPrecision = m_pfeStitchPrecision->value()/Units::mtoUnit();

    s_bSecFaces      = m_prbSecFaces->isChecked();
    s_bSecSplines    = m_prbSecSplines->isChecked();
    s_bSecRightHalf  = m_pchSecRightHalf->isChecked();
    s_bSecPlaneFrame = m_prbSecPlaneFrame->isChecked();
}


void WingExportDlg::onExport()
{
    std::string logmsg;
    TopoDS_Shape wingshape;

    readParams();

    setFocus();
    onExportType();

    WingXfl ExportWing(*m_pWing);
    for(int i=0; i<ExportWing.nSections(); i++)
    {
        ExportWing.setNXPanels(i, s_iChordRes);
        ExportWing.setXPanelDist(i, xfl::COSINE);
    }
    if(s_SurfaceType==3 && s_bSecPlaneFrame)
        ExportWing.createSurfaces(m_pWing->position(), m_pWing->rx(), m_pWing->ry());
    else
        ExportWing.createSurfaces(Vector3d(), 0.0, 0.0);

    m_ShapesToExport.Clear();

    if(s_SurfaceType==3)
    {
        // one shape per section, exported as separate entities
        occ::makeWingSectionShapes(&ExportWing, s_bSecSplines, s_SplineDegree, s_nSplineCtrlPts, s_iChordRes,
                                   s_bSecFaces, s_bSecRightHalf, m_ShapesToExport, logmsg);
        updateStdOutput(logmsg+"\n");
        if(!m_ShapesToExport.IsEmpty()) exportShapes();
        return;
    }

    if      (s_SurfaceType==0) occ::makeWingShape(      &ExportWing, s_StitchPrecision, wingshape, logmsg);
    else if (s_SurfaceType==1) occ::makeWing2NurbsShape(&ExportWing, s_StitchPrecision, s_SplineDegree, s_nSplineCtrlPts, s_iChordRes, wingshape, logmsg);
    else if (s_SurfaceType==2)
//            makeWingSplineSweepSolid(&ExportWing, 1.e-4, wingshape, logmsg);
        occ::makeWingSplineSweep(&ExportWing, s_StitchPrecision, s_SplineDegree, s_nSplineCtrlPts, s_iChordRes, wingshape, logmsg);
//        makeWingSplineSweepMultiSections(&ExportWing, 1.e-4, wingshape, logmsg);

    updateStdOutput(logmsg+"\n");

    if(!wingshape.IsNull())
        m_ShapesToExport.Append(wingshape);

    exportShapes();

}


