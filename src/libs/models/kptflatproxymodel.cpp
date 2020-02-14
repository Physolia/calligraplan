/* This file is part of the KDE project
  Copyright (C) 2010, 2012 Dag Andersen <danders@get2net.dk>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

// clazy:excludeall=qstring-arg
#include "kptflatproxymodel.h"

#include "kptglobal.h"

#include <KLocalizedString>

#include <QModelIndex>

#include "kptdebug.h"

using namespace KPlato;


FlatProxyModel::FlatProxyModel(QObject *parent)
    : KDescendantsProxyModel(parent)
{
}

int FlatProxyModel::columnCount(const QModelIndex &parent) const
{
    int count = KDescendantsProxyModel::columnCount(parent);
    return count ? count + 1 : 0;
}

QVariant FlatProxyModel::data(const QModelIndex &index, int role) const
{
    if (index.column() == columnCount() - 1) {
        // get parent name
        QModelIndex source_index = mapToSource(index.sibling(index.row(), 0)).parent();
        return source_index.data(role);
    }
    return KDescendantsProxyModel::data(index, role);
}

QVariant FlatProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section == columnCount() - 1) {
        if (orientation == Qt::Vertical) {
            return QVariant();
        }
        return role == Role::ColumnTag ? "Parent" : i18n("Parent");
    }
    return KDescendantsProxyModel::headerData(section, orientation, role);
}

QMimeData *FlatProxyModel::mimeData(const QModelIndexList &indexes) const
{
    if (sourceModel() == 0) {
        return 0;
    }
    QModelIndexList source_indexes;
    for (int i = 0; i < indexes.count(); ++i) {
        source_indexes << mapToSource(indexes.at(i));
    }
    return sourceModel()->mimeData(source_indexes);
}

QStringList FlatProxyModel::mimeTypes() const
{
    if (sourceModel() == 0) {
        return QStringList();
    }
    return sourceModel()->mimeTypes();
}

Qt::DropActions FlatProxyModel::supportedDropActions() const
{
    if (sourceModel() == 0) {
        return 0;
    }
    return sourceModel()->supportedDropActions();
}

bool FlatProxyModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
                                         int row, int column, const QModelIndex &parent)
{
    if (sourceModel() == 0) {
        return false;
    }
    if ((row == -1) && (column == -1))
        return sourceModel()->dropMimeData(data, action, -1, -1, mapToSource(parent));
    int source_destination_row = -1;
    int source_destination_column = -1;
    QModelIndex source_parent;
    if (row == rowCount(parent)) {
        source_parent = mapToSource(parent);
        source_destination_row = sourceModel()->rowCount(source_parent);
    } else {
        QModelIndex proxy_index = index(row, column, parent);
        QModelIndex source_index = mapToSource(proxy_index);
        source_destination_row = source_index.row();
        source_destination_column = source_index.column();
        source_parent = source_index.parent();
    }
    return sourceModel()->dropMimeData(data, action, source_destination_row,
                                  source_destination_column, source_parent);
}

QModelIndex FlatProxyModel::mapToSource(const QModelIndex &proxyIndex) const
{
    if (! proxyIndex.isValid() || proxyIndex.column() == columnCount() - 1) {
        return QModelIndex();
    }
    return KDescendantsProxyModel::mapToSource(proxyIndex);
}
