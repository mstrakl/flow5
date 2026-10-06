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

#include <QPainter>
#include <QVBoxLayout>
#include <QLabel>


#include "legendwt.h"
#include <interfaces/widgets/customwts/flowlayout.h>

#include <core/displayoptions.h>
#include <core/xflcore.h>
#include <interfaces/graphs/graph/curve.h>
#include <interfaces/graphs/graph/graph.h>
#include <interfaces/widgets/customdlg/newnamedlg.h>
#include <interfaces/widgets/line/legendbtn.h>
#include <interfaces/widgets/line/linemenu.h>
#include <interfaces/widgets/globals/wt_globals.h>

LegendWt::LegendWt(QWidget *pParent) : QWidget(pParent)
{
    setCursor(Qt::ArrowCursor);
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    m_pGraph = nullptr;
    m_pActiveCurve = nullptr;

    setAutoFillBackground(false);
}


/** Returns the legend's flow layout, emptied; replaces any other type of layout */
FlowLayout *LegendWt::flowLayout()
{
    FlowLayout *pFlow = dynamic_cast<FlowLayout*>(layout());
    if(pFlow)
    {
        wt::clearLayout(pFlow);
        return pFlow;
    }
    if(layout())
    {
        wt::clearLayout(layout());
        delete layout();
    }
    pFlow = new FlowLayout;
    setLayout(pFlow);
    return pFlow;
}


/** A group heading in the legend: compact font, truncated to the legend's maximum length, full text as a tooltip */
QLabel *LegendWt::makeHeading(QString const &text, bool bBold) const
{
    QLabel *pLab = new QLabel(LegendBtn::legendText(text));
    pLab->setAttribute(Qt::WA_NoSystemBackground);
    QFont fnt = LegendBtn::legendFont();
    fnt.setBold(bBold);
    pLab->setFont(fnt);
    pLab->setStyleSheet(QString("color: %1;").arg(DisplayOptions::textColor().name(QColor::HexRgb)));
    if(text.length()>LegendBtn::s_MaxLegendChars) pLab->setToolTip(text);
    return pLab;
}


void LegendWt::makeGraphLegendBtns(bool bHighlight)
{
    if(!m_pGraph) return;

    QPalette s_Palette;
    s_Palette.setColor(QPalette::WindowText, DisplayOptions::textColor());
    s_Palette.setColor(QPalette::Window,     DisplayOptions::backgroundColor());
    s_Palette.setColor(QPalette::Base,       DisplayOptions::backgroundColor());
    setPalette(s_Palette);

    m_XflObjectMap.clear();
    m_CurveMap.clear();

    FlowLayout *pLegendLayout = flowLayout();
    for (int j=0; j<m_pGraph->curveCount(); j++)
    {
        Curve *pCurve = m_pGraph->curve(j);

        // a right axis curve which duplicates a left axis curve of the same name has a single legend entry
        if(pCurve->isRightAxis())
        {
            bool bTwin = false;
            for(int k=0; k<m_pGraph->curveCount() && !bTwin; k++)
                bTwin = m_pGraph->curve(k)->isLeftAxis() && m_pGraph->curve(k)->name()==pCurve->name();
            if(bTwin) continue;
        }

        LegendBtn *pLegendBtn = new LegendBtn;
        LineStyle ls(true, pCurve->stipple(), pCurve->width(),
                     pCurve->fl5Clr(), pCurve->symbol(), pCurve->name().toStdString());
        pLegendBtn->setStyle(ls);
        pLegendBtn->setBackground(true);

        pLegendBtn->setHighLight(m_pGraph->isCurveSelected(pCurve) && Graph::isHighLighting() && bHighlight);
        m_CurveMap.insert(pLegendBtn, pCurve);

        connect(pLegendBtn, SIGNAL(clickedLB(LineStyle)),      SLOT(onClickedCurveBtn()));
        connect(pLegendBtn, SIGNAL(clickedRightLB(LineStyle)), SLOT(onRightClickedCurveBtn(LineStyle)));
        connect(pLegendBtn, SIGNAL(clickedLine(LineStyle)),    SLOT(onClickedCurveLine(LineStyle)));
        pLegendLayout->addWidget(pLegendBtn);

    }
}


void LegendWt::onClickedCurveBtn()
{
//    for(int i=0; i<m_CurveMap.size(); i++) m_CurveMap.keys().at(i)->setHighLight(false);
    QMap<LegendBtn*, Curve*>::iterator it;
    for (it=m_CurveMap.begin(); it!=m_CurveMap.end(); it++)
        it.key()->setHighLight(false);

    if(Graph::isHighLighting())
    {
        LegendBtn *pLegendBtn = dynamic_cast<LegendBtn*>(sender());
        if(pLegendBtn) pLegendBtn->setHighLight(Graph::isHighLighting());
    }

    update();
    emit updateGraphs();
}


void LegendWt::onRightClickedCurveBtn(LineStyle )
{
    LegendBtn *pLegendBtn = dynamic_cast<LegendBtn*>(sender());
    m_pActiveCurve = dynamic_cast<Curve*>(m_CurveMap[pLegendBtn]);

    QAction *pRenameCurve = new QAction(tr("Rename"), this);
    QAction *pDeleteCurve = new QAction(tr("Delete"), this);
    connect(pRenameCurve, SIGNAL(triggered(bool)), SLOT(onRenameActiveCurve()));
    connect(pDeleteCurve, SIGNAL(triggered(bool)), SLOT(onDeleteActiveCurve()));
    QMenu *pCtxMenu = new QMenu;
    pCtxMenu->addAction(pRenameCurve);
    pCtxMenu->addAction(pDeleteCurve);
    pCtxMenu->exec(QCursor::pos());

    update();
    emit updateGraphs();
}


void LegendWt::onClickedCurveLine(LineStyle ls)
{
    LegendBtn *pLegendBtn = dynamic_cast<LegendBtn*>(sender());

    Curve* pCurve = dynamic_cast<Curve*>(m_CurveMap[pLegendBtn]);
    LineMenu *pLineMenu = new LineMenu(nullptr);
    pLineMenu->initMenu(ls);
    pLineMenu->exec(QCursor::pos());
    ls = pLineMenu->theStyle();
    pCurve->setTheStyle(ls);
    pLegendBtn->setStyle(ls);
    update();
    emit updateGraphs();
 }


void LegendWt::onDeleteActiveCurve()
{
    if(!m_pGraph || !m_pActiveCurve) return;

    m_pGraph->deleteCurve(m_pActiveCurve);
    makeLegend(false);
    update();
    emit updateGraphs();
}


void LegendWt::onRenameActiveCurve()
{
    if(!m_pGraph || !m_pActiveCurve) return;
    NewNameDlg dlg(m_pActiveCurve->name(), this);

    if(dlg.exec()==QDialog::Accepted)
    {
        m_pActiveCurve->setName(dlg.newName());
        LegendBtn *pLegendBtn = m_CurveMap.key(m_pActiveCurve);
        if(pLegendBtn)
        {
            pLegendBtn->setTag(m_pActiveCurve->name());
            pLegendBtn->update();
            emit updateGraphs();
        }
    }
}

/** default legend using curve names */
void LegendWt::makeLegend(bool bHighlight)
{
    setStyleSheet(QString::asprintf("background: %s;", DisplayOptions::backgroundColor().name(QColor::HexRgb).toStdString().c_str()));
    setAutoFillBackground(true);

    if(!m_pGraph) return;

    m_XflObjectMap.clear();
    m_CurveMap.clear();

    FlowLayout *pLegendLayout = flowLayout();

    for (int j=0; j<m_pGraph->curveCount(); j++)
    {
        Curve *pCurve = m_pGraph->curve(j);

        // a right axis curve which duplicates a left axis curve of the same name has a single legend entry
        if(pCurve->isRightAxis())
        {
            bool bTwin = false;
            for(int k=0; k<m_pGraph->curveCount() && !bTwin; k++)
                bTwin = m_pGraph->curve(k)->isLeftAxis() && m_pGraph->curve(k)->name()==pCurve->name();
            if(bTwin) continue;
        }

        LegendBtn *pLegendBtn = new LegendBtn;
        LineStyle ls(true, pCurve->stipple(), pCurve->width(),
                     pCurve->fl5Clr(), pCurve->symbol(), pCurve->name().toStdString());
        pLegendBtn->setStyle(ls);
        pLegendBtn->setBackground(true);

        pLegendBtn->setHighLight(m_pGraph->isCurveSelected(pCurve) && Graph::isHighLighting() && bHighlight);
        m_CurveMap.insert(pLegendBtn, pCurve);

        connect(pLegendBtn, SIGNAL(clickedLB(LineStyle)),      SLOT(onClickedCurveBtn()));
        connect(pLegendBtn, SIGNAL(clickedRightLB(LineStyle)), SLOT(onRightClickedCurveBtn(LineStyle)));
        connect(pLegendBtn, SIGNAL(clickedLine(LineStyle)),    SLOT(onClickedCurveLine(LineStyle)));
        pLegendLayout->addWidget(pLegendBtn);

    }
}



