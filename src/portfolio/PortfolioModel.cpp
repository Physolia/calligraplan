/* This file is part of the KDE project
* SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
*
* SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "PortfolioModel.h"
#include "ProjectsModel.h"
#include "MainDocument.h"

#include <kptproject.h>

#include <KoDocument.h>

#include <QAbstractItemView>
#include <QDebug>

PortfolioModel::PortfolioModel(QObject *parent)
    : KExtraColumnsProxyModel(parent)
{
    appendColumn(xi18nc("@title:column", "Portfolio"));
    appendColumn(xi18nc("@title:column", "Storage"));
    appendColumn(xi18nc("@title:column", "File"));

    m_baseModel = new ProjectsFilterModel(this);
    // Note: changes might affect methods below
    const QList<int> columns = QList<int>() << KPlato::NodeModel::NodeName;
    m_baseModel->setAcceptedColumns(columns);
    setSourceModel(m_baseModel);
}

PortfolioModel::~PortfolioModel()
{
}

void PortfolioModel::setDelegates(QAbstractItemView *view)
{
    view->setItemDelegateForColumn(1, new KPlato::EnumDelegate(view)); // Portfolio
    view->setItemDelegateForColumn(2, new KPlato::EnumDelegate(view)); // Storage
}

Qt::ItemFlags PortfolioModel::flags(const QModelIndex &idx) const
{
    Qt::ItemFlags flg = QAbstractItemModel::flags(idx);
    KoDocument *doc = documentFromIndex(idx);
    if (doc) {
        int extraColumn = extraColumnForProxyColumn(idx.column());
        switch (extraColumn) {
            case 0:
            case 1: {
                flg |= Qt::ItemIsEditable;
                break;
            }
            default: {
                break;
            }
        }
    }
    return flg;
}

QVariant PortfolioModel::extraColumnData(const QModelIndex &parent, int row, int extraColumn, int role) const
{
    Q_UNUSED(parent)
    KoDocument *doc = documentFromIndex(index(row, 0, parent));
    if (!doc) {
        return QVariant();
    }
    switch (extraColumn) {
        case 0: {
            switch (role) {
                case Qt::DisplayRole:
                    return doc->property(ISPORTFOLIO).toBool() ? i18n("Yes") : i18n("No");
                case Qt::EditRole:
                    return doc->property(ISPORTFOLIO).toBool() ? 1 : 0;
                case KPlato::Role::EnumList: {
                    return  QStringList() << i18n("No") << i18n("Yes");
                }
                case KPlato::Role::EnumListValue: {
                    return doc->property(ISPORTFOLIO).toBool() ? 1 : 0;
                }
                default: break;
            }
            break;
        }
        case 1: {
            switch (role) {
                case Qt::DisplayRole:
                    return doc->property(SAVEEMBEDDED).toBool() ? i18n("Embedded") : i18n("External");
                case Qt::EditRole: {
                    return doc->property(SAVEEMBEDDED).toBool() ? 1 : 0;
                }
                case KPlato::Role::EnumList: {
                    return  QStringList() << i18n("External") << i18n("Embedded");
                }
                case KPlato::Role::EnumListValue: {
                    return doc->property(SAVEEMBEDDED).toBool() ? 1 : 0;
                }
                default: break;
            }
            break;
        }
        case 2: {
            switch (role) {
                case Qt::DisplayRole:
                case Qt::EditRole: {
                    return doc->url().toDisplayString();
                }
                default: break;
            }
            break;
        }
        default: break;
    }
    return QVariant();
}

bool PortfolioModel::setExtraColumnData(const QModelIndex &parent, int row, int extraColumn, const QVariant &value, int role)
{
    if (role == Qt::EditRole) {
        auto doc = documentFromIndex(index(row, 0, parent));
        if (!doc) {
            return false;
        }
        switch (extraColumn) {
            case 0: {
                if (portfolio()->setDocumentProperty(doc, ISPORTFOLIO, value)) {
                    extraColumnDataChanged(parent, row, 0, QVector<int>()<<Qt::DisplayRole);
                    return true;
                }
                break;
            }
            case 1: {
                if (portfolio()->setDocumentProperty(doc, SAVEEMBEDDED, value)) {
                    extraColumnDataChanged(parent, row, 0, QVector<int>()<<Qt::DisplayRole);
                    return true;
                }
                break;
            }
            default: {
                break;
            }
        }
    }
    return false;
}

void PortfolioModel::setPortfolio(MainDocument *doc)
{
    m_baseModel->setPortfolio(doc);
}

MainDocument *PortfolioModel::portfolio() const
{
    return m_baseModel->portfolio();
}

KoDocument *PortfolioModel::documentFromIndex(const QModelIndex &idx) const
{
    const auto i = index(idx.row(), 0, idx.parent());
    return m_baseModel->documentFromIndex(mapToSource(i));
}
