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

#include <QWidget>

#include "flowlayout.h"


FlowLayout::FlowLayout(QWidget *pParent, int hSpacing, int vSpacing) : QLayout(pParent), m_hSpace(hSpacing), m_vSpace(vSpacing)
{
    setContentsMargins(4, 2, 4, 2);
}


FlowLayout::~FlowLayout()
{
    QLayoutItem *pItem = nullptr;
    while ((pItem = takeAt(0)))
        delete pItem;
}


void FlowLayout::addItem(QLayoutItem *pItem)
{
    m_Items.append(pItem);
}


void FlowLayout::addLineBreak()
{
    m_Breaks.append(int(m_Items.size()));
}


QLayoutItem *FlowLayout::itemAt(int index) const
{
    return (index>=0 && index<m_Items.size()) ? m_Items.at(index) : nullptr;
}


QLayoutItem *FlowLayout::takeAt(int index)
{
    if(index<0 || index>=m_Items.size()) return nullptr;
    // shift the line breaks which follow the removed item
    for(int &ib : m_Breaks) if(ib>index) ib--;
    if(m_Items.size()==1) m_Breaks.clear();
    return m_Items.takeAt(index);
}


int FlowLayout::heightForWidth(int width) const
{
    return doLayout(QRect(0, 0, width, 0), true);
}


QSize FlowLayout::minimumSize() const
{
    QSize size;
    for(QLayoutItem const *pItem : m_Items)
        size = size.expandedTo(pItem->minimumSize());
    QMargins const margins = contentsMargins();
    size += QSize(margins.left()+margins.right(), margins.top()+margins.bottom());
    return size;
}


void FlowLayout::setGeometry(QRect const &rect)
{
    QLayout::setGeometry(rect);
    doLayout(rect, false);
}


int FlowLayout::doLayout(QRect const &rect, bool bTestOnly) const
{
    QMargins const margins = contentsMargins();
    QRect const effective = rect.adjusted(margins.left(), margins.top(), -margins.right(), -margins.bottom());
    int hspace = m_hSpace>=0 ? m_hSpace : 12;
    int vspace = m_vSpace>=0 ? m_vSpace : 2;

    int x = effective.x();
    int y = effective.y();
    int lineHeight = 0;

    for(int i=0; i<m_Items.size(); i++)
    {
        QLayoutItem *pItem = m_Items.at(i);
        if(pItem->widget() && pItem->widget()->isHidden()) continue;
        QSize const hint = pItem->sizeHint();

        bool bBreak = m_Breaks.contains(i) && x>effective.x();
        int nextX = x + hint.width();
        if(bBreak || (nextX>effective.right() && lineHeight>0))
        {
            x = effective.x();
            y = y + lineHeight + vspace;
            nextX = x + hint.width();
            lineHeight = 0;
        }

        if(!bTestOnly)
            pItem->setGeometry(QRect(QPoint(x, y), hint));

        x = nextX + hspace;
        lineHeight = qMax(lineHeight, hint.height());
    }
    return y + lineHeight - rect.y() + margins.bottom();
}
