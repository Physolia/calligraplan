/* This file is part of the KDE project
  Copyright (C) 2009, 2012 Dag Andersen danders@get2net>

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
#include "kptresourceallocationmodel.h"

#include "AlternativeResourceDelegate.h"
#include "RequieredResourceDelegate.h"
#include <kptresourcerequest.h>
#include "kptcommonstrings.h"
#include "kptitemmodelbase.h"
#include "kptcalendar.h"
#include "kptduration.h"
#include "kptnode.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptresource.h"
#include "kptdatetime.h"
#include "kptdebug.h"

#include <KLocalizedString>

#include <QStringList>


using namespace KPlato;

//--------------------------------------

ResourceAllocationModel::ResourceAllocationModel(QObject *parent)
    : QObject(parent),
    m_project(0),
    m_task(0)
{
}

ResourceAllocationModel::~ResourceAllocationModel()
{
}

const QMetaEnum ResourceAllocationModel::columnMap() const
{
    return metaObject()->enumerator(metaObject()->indexOfEnumerator("Properties"));
}

void ResourceAllocationModel::setProject(Project *project)
{
    m_project = project;
}

void ResourceAllocationModel::setTask(Task *task)
{
    m_task = task;
}

int ResourceAllocationModel::propertyCount() const
{
    return columnMap().keyCount();
}

QVariant ResourceAllocationModel::name(const Resource *res, int role) const
{
    //debugPlan<<res->name()<<","<<role;
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        case Qt::ToolTipRole:
            return res->name();
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant ResourceAllocationModel::type(const Resource *res, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            return res->typeToString(true);
        case Qt::EditRole:
            return res->typeToString(false);
        case Role::EnumList:
            return res->typeToStringList(true);
        case Role::EnumListValue:
            return (int)res->type();
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant ResourceAllocationModel::allocation(const Resource *res, int role) const
{
    if (m_project == nullptr || m_task == nullptr) {
        return QVariant();
    }
    const ResourceRequest *rr = m_task->requests().find(res);
    switch (role) {
        case Qt::DisplayRole: {
            int units = rr ? rr->units() : 0;
            // xgettext: no-c-format
            return i18nc("<value>%", "%1%", units);
        }
        case Qt::EditRole:
            return rr ? rr->units() : 0;
        case Qt::ToolTipRole: {
            int units = rr ? rr->units() : 0;
            if (units == 0) {
                return xi18nc("@info:tooltip", "Not allocated");
            }
            // xgettext: no-c-format
            return xi18nc("@info:tooltip", "Allocated units: %1%", units);
        }
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Role::Minimum:
            return 0;
        case Role::Maximum:
            return 100;
        case Qt::CheckStateRole:
            return Qt::Unchecked;

    }
    return QVariant();
}

QVariant ResourceAllocationModel::maximum(const Resource *res, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            // xgettext: no-c-format
            return i18nc("<value>%", "%1%", res->units());
        case Qt::EditRole:
            return res->units();
        case Qt::ToolTipRole:
            // xgettext: no-c-format
            return i18n("Maximum units available: %1%", res->units());
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant ResourceAllocationModel::required(const Resource *res, int role) const
{
    switch (role) {
        case Qt::DisplayRole: {
            QStringList lst;
            foreach (Resource *r, res->requiredResources()) {
                lst << r->name();
            }
            return lst.join(",");
        }
        case Qt::EditRole:
            return QVariant();//Not used
        case Qt::ToolTipRole:
            return QVariant();
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::StatusTipRole:
            return QVariant();
        case Qt::WhatsThisRole:
            return xi18nc("@info:whatsthis", "<title>Required Resources</title>"
            "<para>A working resource can be assigned to one or more required resources."
            " A required resource is a material resource that the working resource depends on"
            " in order to do the work.</para>"
            "<para>To be able to use a material resource as a required resource, the material resource"
            " must be part of a group of type <emphasis>Material</emphasis>.</para>");
    }
    return QVariant();
}

QVariant ResourceAllocationModel::alternative(const Resource *res, int role) const
{
    switch (role) {
        case Qt::DisplayRole: {
            QStringList lst;
            if (m_task) {
                ResourceRequest *rr = m_task->requests().find(res);
                if (rr) {
                    for (ResourceRequest *r : rr->alternativeRequests()) {
                        lst << r->resource()->name();
                    }
                }
            }
            return lst.join(",");
        }
        case Qt::EditRole:
            return QVariant();//Not used
        case Qt::ToolTipRole:
            return QVariant();
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::StatusTipRole:
            return QVariant();
        case Qt::WhatsThisRole:
            return xi18nc("@info:whatsthis", "<title>Alternative Resource Allocation</title>"
            "<para>A working resource can be assigned to one or more required resources."
            " A required resource is a material resource that the working resource depends on"
            " in order to do the work.</para>"
            "<para>To be able to use a material resource as a required resource, the material resource"
            " must be part of a group of type <emphasis>Material</emphasis>.</para>");
        case Qt::CheckStateRole:
            break;
    }
    return QVariant();
}

QVariant ResourceAllocationModel::data(const Resource *resource, int property, int role) const
{
    QVariant result;
    if (resource == nullptr) {
        return result;
    }
    switch (property) {
        case RequestName: result = name(resource, role); break;
        case RequestType: result = type(resource, role); break;
        case RequestAllocation: result = allocation(resource, role); break;
        case RequestMaximum: result = maximum(resource, role); break;
        case RequestAlternative: result = alternative(resource, role); break;
        case RequestRequired: result = required(resource, role); break;
        default:
            debugPlan<<"data: invalid display value: property="<<property;
            break;
    }
    return result;
}

QVariant ResourceAllocationModel::headerData(int section, int role)
{
    if (role == Qt::DisplayRole) {
        switch (section) {
            case RequestName: return i18n("Name");
            case RequestType: return i18n("Type");
            case RequestAllocation: return i18n("Allocation");
            case RequestMaximum: return xi18nc("@title:column", "Available");
            case RequestAlternative: return xi18nc("@title:column", "Alternative");
            case RequestRequired: return xi18nc("@title:column", "Required Resources");
            default: return QVariant();
        }
    } else if (role == Qt::TextAlignmentRole) {
        switch (section) {
            case 0: return QVariant();
            default: return Qt::AlignCenter;
        }
    } else if (role == Qt::ToolTipRole) {
        switch (section) {
            case RequestName: return ToolTip::resourceName();
            case RequestType: return ToolTip::resourceType();
            case RequestAllocation: return i18n("Resource allocation");
            case RequestMaximum: return xi18nc("@info:tootip", "Available resources or resource units");
            case RequestAlternative: return xi18nc("@info:tooltip", "List of resources that can be used as an alternative");
            case RequestRequired: return xi18nc("@info:tootip", "Required material resources");
            default: return QVariant();
        }
    } else if (role == Qt::WhatsThisRole) {
        switch (section) {
            case RequestRequired:
                return xi18nc("@info:whatsthis", "<title>Required Resources</title>"
                "<para>A working resource can be assigned to one or more required resources."
                " A required resource is a material resource that the working resource depends on"
                " in order to do the work.</para>"
                "<para>To be able to use a material resource as a required resource, the material resource"
                " must be part of a group of type Material.</para>");
            default: return QVariant();
        }
    }
    return QVariant();
}
//--------------------------------------

ResourceAllocationItemModel::ResourceAllocationItemModel(QObject *parent)
    : ItemModelBase(parent)
{
}

ResourceAllocationItemModel::~ResourceAllocationItemModel()
{
}

void ResourceAllocationItemModel::slotResourceToBeAdded(KPlato::Project *project, int row)
{
    Q_UNUSED(project)
    beginInsertRows(QModelIndex(), row, row);
}

void ResourceAllocationItemModel::slotResourceAdded(KPlato::Resource *resource)
{
    Q_UNUSED(resource)
    endInsertRows();
}

void ResourceAllocationItemModel::slotResourceToBeRemoved(KPlato::Project *project, int row, KPlato::Resource *resource)
{
    Q_UNUSED(project)
    Q_UNUSED(resource)
    beginRemoveRows(QModelIndex(), row, row);
}

void ResourceAllocationItemModel::slotResourceRemoved()
{
    endRemoveRows();
}

void ResourceAllocationItemModel::setProject(Project *project)
{
    beginResetModel();
    if (m_project) {
        disconnect(m_project, &Project::aboutToBeDeleted, this, &ResourceAllocationItemModel::projectDeleted);

        disconnect(m_project, &Project::resourceChanged, this, &ResourceAllocationItemModel::slotResourceChanged);
        disconnect(m_project, &Project::resourceToBeAdded, this, &ResourceAllocationItemModel::slotResourceToBeAdded);
        disconnect(m_project, &Project::resourceAdded, this, &ResourceAllocationItemModel::slotResourceAdded);
        disconnect(m_project, &Project::resourceToBeRemoved, this, &ResourceAllocationItemModel::slotResourceToBeRemoved);
        disconnect(m_project, &Project::resourceRemoved, this, &ResourceAllocationItemModel::slotResourceRemoved);
    }
    m_project = project;
    if (m_project) {
        disconnect(m_project, &Project::aboutToBeDeleted, this, &ResourceAllocationItemModel::projectDeleted);
        
        disconnect(m_project, &Project::resourceChanged, this, &ResourceAllocationItemModel::slotResourceChanged);
        disconnect(m_project, &Project::resourceToBeAdded, this, &ResourceAllocationItemModel::slotResourceToBeAdded);
        disconnect(m_project, &Project::resourceAdded, this, &ResourceAllocationItemModel::slotResourceAdded);
        disconnect(m_project, &Project::resourceToBeRemoved, this, &ResourceAllocationItemModel::slotResourceToBeRemoved);
        disconnect(m_project, &Project::resourceRemoved, this, &ResourceAllocationItemModel::slotResourceRemoved);
    }
    m_model.setProject(m_project);
    endResetModel();
}

void ResourceAllocationItemModel::setTask(Task *task)
{
    if (task == m_model.task()) {
        return;
    }
    if (m_model.task() == nullptr) {
        beginResetModel();
        filldata(task);
        m_model.setTask(task);
        endResetModel();
        return;
    }
    if (task) {
        emit layoutAboutToBeChanged();
        filldata(task);
        m_model.setTask(task);
        emit layoutChanged();
    }
}

void ResourceAllocationItemModel::filldata(Task *task)
{
    qDeleteAll(m_resourceCache);
    m_resourceCache.clear();
    m_requiredChecked.clear();
    if (m_project && task) {
        for (ResourceRequest *rr : task->requests().resourceRequests()) {
            const Resource *r = rr->resource();
            m_resourceCache[r] = new ResourceRequest(*rr);
            if (!m_resourceCache[r]->requiredResources().isEmpty()) {
                m_requiredChecked[r] = Qt::Checked;
            }
        }
    }
}

bool ResourceAllocationItemModel::hasMaterialResources() const
{
    if (!m_project) {
        return false;
    }
    foreach (const Resource *r, m_project->resourceList()) {
        if (r->type() == Resource::Type_Material) {
            return true;
        }
    }
    return false;
}

Qt::ItemFlags ResourceAllocationItemModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = ItemModelBase::flags(index);
    if (!m_readWrite) {
        //debugPlan<<"read only"<<flags;
        return flags &= ~Qt::ItemIsEditable;
    }
    if (!index.isValid()) {
        //debugPlan<<"invalid"<<flags;
        return flags;
    }
    switch (index.column()) {
        case ResourceAllocationModel::RequestAllocation:
            flags |= (Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
            break;
        case ResourceAllocationModel::RequestAlternative: {
            flags |= (Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
            break;
        }
        case ResourceAllocationModel::RequestRequired: {
            Resource *r = resource(index);
            if (r && r->type() != Resource::Type_Work) {
                flags &= ~(Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
            } else if (m_resourceCache.contains(r) && m_resourceCache[r]->units() > 0) {
                flags |= (Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
                if (!hasMaterialResources()) {
                    flags &= ~Qt::ItemIsEnabled;
                }
            }
            break;
        }
        default:
            flags &= ~Qt::ItemIsEditable;
            break;
    }
    return flags;
}


QModelIndex ResourceAllocationItemModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return QModelIndex();
}

QModelIndex ResourceAllocationItemModel::index(int row, int column, const QModelIndex &parent) const
{
    if (m_project == nullptr || column < 0 || column >= columnCount() || row < 0) {
        return QModelIndex();
    }
    if (parent.isValid()) {
        return QModelIndex();
    }
    Resource *r = m_project->resourceAt(row);
    if (r) {
        return createIndex(row, column, r);
    }
    return QModelIndex();
}

QModelIndex ResourceAllocationItemModel::index(Resource *resource) const
{
    if (m_project == nullptr || resource == nullptr) {
        return QModelIndex();
    }
    int row = m_project->indexOf(resource);
    if (row >= 0) {
        return createIndex(row, 0, resource);
    }
    return QModelIndex();
}

int ResourceAllocationItemModel::columnCount(const QModelIndex &/*parent*/) const
{
    return m_model.propertyCount();
}

int ResourceAllocationItemModel::rowCount(const QModelIndex &parent) const
{
    if (m_project == nullptr || m_model.task() == nullptr) {
        return 0;
    }
    return parent.isValid() ? 0 : m_project->resourceCount();
}

QVariant ResourceAllocationItemModel::allocation(const Resource *res, int role) const
{
    if (m_model.task() == nullptr) {
        return QVariant();
    }
    if (!m_resourceCache.contains(res)) {
        if (role == Qt::EditRole) {
            ResourceRequest *req = m_model.task()->requests().find(res);
            if (req == nullptr) {
                req = new ResourceRequest(const_cast<Resource*>(res), 0);
            }
            const_cast<ResourceAllocationItemModel*>(this)->m_resourceCache.insert(res, req);
            return req->units();
        }
        return m_model.allocation(res, role);
    }
    switch (role) {
        case Qt::DisplayRole: {
            // xgettext: no-c-format
            return i18nc("<value>%", "%1%", m_resourceCache[res]->units());
        }
        case Qt::EditRole:
            return m_resourceCache[res]->units();
        case Qt::ToolTipRole: {
            if (res->units() == 0) {
                return xi18nc("@info:tooltip", "Not allocated");
            }
            return xi18nc("@info:tooltip", "%1 allocated out of %2 available", allocation(res, Qt::DisplayRole).toString(), m_model.maximum(res, Qt::DisplayRole).toString());
        }
        case Qt::CheckStateRole:
            return m_resourceCache[res]->units() == 0 ? Qt::Unchecked : Qt::Checked;
        default:
            return m_model.allocation(res, role);
    }
    return QVariant();
}

bool ResourceAllocationItemModel::setAllocation(Resource *res, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole: {
            m_resourceCache[res]->setUnits(value.toInt());
            QModelIndex idx = index(res);
            emit dataChanged(idx, idx.sibling(idx.row(), ResourceAllocationModel::RequestAllocation));
            return true;
        }
        case Qt::CheckStateRole: {
            if (!m_resourceCache.contains(res)) {
                m_resourceCache[res] = new ResourceRequest(res, 0);
            }
            if (m_resourceCache[res]->units() == 0) {
                m_resourceCache[res]->setUnits(100);
            } else {
                m_resourceCache[res]->setUnits(0);
            }
            QModelIndex idx = index(res);
            emit dataChanged(idx, idx);
            return true;
        }
    }
    return false;
}

QVariant ResourceAllocationItemModel::required(const QModelIndex &idx, int role) const
{
    if (m_model.task() == nullptr) {
        return QVariant();
    }
    Resource *res = resource(idx);
    if (res == nullptr) {
        return QVariant();
    }
    switch (role) {
        case Qt::DisplayRole: {
            if (res->type() == Resource::Type_Work) {
                QStringList lst;
                if (m_requiredChecked[res]) {
                    foreach (const Resource *r, required(idx)) {
                        lst << r->name();
                    }
                }
                return lst.isEmpty() ? i18n("None") : lst.join(",");
            }
            break;
        }
        case Qt::EditRole: break;
        case Qt::ToolTipRole: 
            switch (res->type()) {
                case Resource::Type_Work: {
                    if (!hasMaterialResources()) {
                        return xi18nc("@info:tooltip", "No material resources available");
                    }
                    QStringList lst;
                    if (m_requiredChecked[res]) {
                        foreach (const Resource *r, required(idx)) {
                            lst << r->name();
                        }
                    }
                    return lst.isEmpty() ? xi18nc("@info:tooltip", "No required resources") : lst.join("\n");
                }
                case Resource::Type_Material:
                    return xi18nc("@info:tooltip", "Material resources cannot have required resources");
                case Resource::Type_Team:
                    return xi18nc("@info:tooltip", "Team resources cannot have required resources");
            }
            break;
        case Qt::CheckStateRole:
            if (res->type() == Resource::Type_Work) {
                return m_requiredChecked[res];
            }
            break;
        default:
            return m_model.required(res, role);
    }
    return QVariant();
}

bool ResourceAllocationItemModel::setRequired(const QModelIndex &idx, const QVariant &value, int role)
{
    Resource *res = resource(idx);
    if (res == nullptr) {
        return false;
    }
    switch (role) {
        case Qt::CheckStateRole:
            m_requiredChecked[res] = value.toInt();
            if (value.toInt() == Qt::Unchecked) {
                m_resourceCache[res]->setRequiredResources(QList<Resource*>());
            }
            emit dataChanged(idx, idx);
            return true;
    }
    return false;
}

QVariant ResourceAllocationItemModel::alternative(const QModelIndex &idx, int role) const
{
    if (m_model.task() == nullptr) {
        return QVariant();
    }
    Resource *res = resource(idx);
    if (res == nullptr) {
        return QVariant();
    }
    switch (role) {
        case Qt::DisplayRole: {
            QStringList lst;
            ResourceRequest *rr = m_resourceCache.value(res);
            if (rr) {
                for (ResourceRequest *r : rr->alternativeRequests()) {
                    lst << r->resource()->name();
                }
            }
            return lst.isEmpty() ? i18n("None") : lst.join(",");
        }
        case Qt::EditRole: break;
        case Qt::ToolTipRole: break;
        default: break;
    }
    return QVariant();
}

QVariant ResourceAllocationItemModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::TextAlignmentRole) {
        // use same alignment as in header (headers always horizontal)
        return headerData(index.column(), Qt::Horizontal, role);
    }
    QVariant result;
    Resource *r = resource(index);
    if (r) {
        if (index.column() == ResourceAllocationModel::RequestAllocation) {
            return allocation(r, role);
        }
        if (index.column() == ResourceAllocationModel::RequestRequired) {
            return required(index, role);
        }
        if (index.column() == ResourceAllocationModel::RequestAlternative) {
            return alternative(index, role);
        }
        result = m_model.data(r, index.column(), role);
    }
    return result;
}

bool ResourceAllocationItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid()) {
        return ItemModelBase::setData(index, value, role);
    }
    if (!(flags(index) & Qt::ItemIsEditable)) {
        return false;
    }
    QObject *obj = object(index);
    Resource *r = qobject_cast<Resource*>(obj);
    if (r) {
        switch (index.column()) {
            case ResourceAllocationModel::RequestAllocation:
                if (setAllocation(r, value, role)) {
                    emit dataChanged(index, index);
                    QModelIndex idx = this->index(index.row(), ResourceAllocationModel::RequestAllocation, parent(parent(index)));
                    emit dataChanged(idx, idx);
                    return true;
                }
                return false;
            case ResourceAllocationModel::RequestAlternative:
                return false;
            case ResourceAllocationModel::RequestRequired:
                return setRequired(index, value, role);
            default:
                //qWarning("data: invalid display value column %d", index.column());
                return false;
        }
    }
    return false;
}

QVariant ResourceAllocationItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        if (role == Qt::DisplayRole) {
            return m_model.headerData(section, role);
        }
        if (role == Qt::TextAlignmentRole) {
            switch (section) {
                case 0: return QVariant();
                default: return Qt::AlignCenter;
            }
            return Qt::AlignCenter;
        }
    }
    return m_model.headerData(section, role);
}

QAbstractItemDelegate *ResourceAllocationItemModel::createDelegate(int col, QWidget *parent) const
{
    QAbstractItemDelegate *del = nullptr;
    switch (col) {
        case ResourceAllocationModel::RequestAllocation: del = new SpinBoxDelegate(parent); break;
        case ResourceAllocationModel::RequestAlternative: del = new AlternativeResourceDelegate(parent); break;
        case ResourceAllocationModel::RequestRequired: del = new RequieredResourceDelegate(parent); break;
        default: break;
    }
    return del;
}

QObject *ResourceAllocationItemModel::object(const QModelIndex &index) const
{
    QObject *o = 0;
    if (index.isValid()) {
        o = static_cast<QObject*>(index.internalPointer());
        Q_ASSERT(o);
    }
    return o;
}

void ResourceAllocationItemModel::slotResourceChanged(Resource *res)
{
    QModelIndex idx = index(res); 
    emit dataChanged(idx, idx.sibling(idx.row(), columnCount()-1));
}

Resource *ResourceAllocationItemModel::resource(const QModelIndex &idx) const
{
    return qobject_cast<Resource*>(object(idx));
}

void ResourceAllocationItemModel::setRequired(const QModelIndex &idx, const QList<Resource*> &lst)
{
    Resource *r = resource(idx);
    Q_ASSERT(r);
    if (m_resourceCache.contains(r)) {
        m_resourceCache[r]->setRequiredResources(lst);
        emit dataChanged(idx, idx);
    }
}

QList<Resource*> ResourceAllocationItemModel::required(const QModelIndex &idx) const
{
    Resource *r = resource(idx);
    Q_ASSERT(r);
    if (m_resourceCache.contains(r)) {
        ResourceRequest* request = m_resourceCache[r];
        return request->requiredResources();
    }
    return r->requiredResources();
}

void ResourceAllocationItemModel::setAlternativeRequests(const QModelIndex &idx, const QList<Resource*> &lst)
{
    Resource *r = resource(idx);
    Q_ASSERT(r);
    if (m_resourceCache.contains(r)) {
        QList<ResourceRequest*> requests;
        for (Resource *r : lst) {
            requests << new ResourceRequest(r);
        }
        m_resourceCache[r]->setAlternativeRequests(requests);
        emit dataChanged(idx, idx);
    }
}

QList<ResourceRequest*> ResourceAllocationItemModel::alternativeRequests(const QModelIndex &idx) const
{
    QList<ResourceRequest*> alternatives;
    ResourceRequest *request = m_resourceCache.value(resource(idx));
    if (request) {
        alternatives = request->alternativeRequests();
    }
    return alternatives;
}
