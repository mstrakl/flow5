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

#include <QLayout>
#include <QList>

/**
 * @brief A layout which places its items left to right and wraps them to a new row when the width is exhausted.
 * Used to display the graph legends as a strip below the graphs.
 * Adapted from Qt's flow layout example.
 */
class FlowLayout : public QLayout
{
    public:
        explicit FlowLayout(QWidget *pParent=nullptr, int hSpacing=-1, int vSpacing=-1);
        ~FlowLayout() override;

        void addItem(QLayoutItem *pItem) override;
        int count() const override {return int(m_Items.size());}
        QLayoutItem *itemAt(int index) const override;
        QLayoutItem *takeAt(int index) override;

        Qt::Orientations expandingDirections() const override {return Qt::Orientations();}
        bool hasHeightForWidth() const override {return true;}
        int heightForWidth(int width) const override;
        QSize minimumSize() const override;
        QSize sizeHint() const override {return minimumSize();}
        void setGeometry(QRect const &rect) override;

        /** inserts a forced line break before the next item */
        void addLineBreak();

    private:
        int doLayout(QRect const &rect, bool bTestOnly) const;

        QList<QLayoutItem*> m_Items;
        QList<int> m_Breaks;  /**< indexes of the items which start a new row */
        int m_hSpace, m_vSpace;
};
