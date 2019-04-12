/* This file is part of the KDE project
 * Copyright (C) 2019 Dag Andersen <danders@get2net.dk>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB. If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

// clazy:excludeall=qstring-arg
#include "InsertProjectXmlCommand.h"

#include "kptaccount.h"
#include "kptappointment.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptcalendar.h"
#include "kptrelation.h"
#include "kptresource.h"
#include "kptdocuments.h"
#include "kptlocale.h"
#include "kptdebug.h"

#include <QApplication>


const QLoggingCategory &PLANCMDINSPROJECT_LOG()
{
    static const QLoggingCategory category("calligra.plan.command.insertProjectXml");
    return category;
}

#define debugPlanInsertProjectXml qCDebug(PLANCMDINSPROJECT_LOG)
#define warnPlanInsertProjectXml qCWarning(PLANCMDINSPROJECT_LOG)
#define errorPlanInsertProjectXml qCCritical(PLANCMDINSPROJECT_LOG)

using namespace KPlato;


AddTaskCommand::AddTaskCommand(Project *project, Node *parent, Node *node, Node *after, const KUndo2MagicString& name)
    : NamedCommand(name)
    , m_project(project)
    , m_parent(parent)
    , m_node(node)
    , m_after(after)
    , m_added(false)
{
}

AddTaskCommand::~AddTaskCommand()
{
    qInfo()<<Q_FUNC_INFO<<m_node<<m_added;
    if (!m_added)
        delete m_node;
}

void AddTaskCommand::execute()
{
    qInfo()<<Q_FUNC_INFO<<m_node->name()<<m_parent<<m_parent->indexOf(m_node);
    m_project->addSubTask(m_node, m_parent->indexOf(m_node), m_parent, true);
    m_added = true;
}

void AddTaskCommand::unexecute()
{
    m_project->takeTask(m_node);
    m_added = false;
}

InsertProjectXmlCommand::InsertProjectXmlCommand(Project *project, const QByteArray &data, Node *parent, Node *after, const KUndo2MagicString& name)
        : MacroCommand(name)
        , m_project(project)
        , m_data(data)
        , m_parent(parent)
        , m_after(after)
        , m_first(true)
{
    //debugPlan<<cal->name();
    Q_ASSERT(project != 0);
    m_context.setProject(project);
    m_context.setProjectTimeZone(project->timeZone()); // from xml doc?
    m_context.setLoadTaskChildren(false);
}

InsertProjectXmlCommand::~InsertProjectXmlCommand()
{
}

void InsertProjectXmlCommand::execute()
{
    if (m_first) {
        // create and execute commands
        KoXmlDocument doc;
        doc.setContent(m_data);
        KoXmlElement projectElement = doc.documentElement().namedItem("project").toElement();

        createCmdAccounts(projectElement);
        createCmdCalendars(projectElement);
        createCmdResources(projectElement);
        createCmdTasks(projectElement);
        createCmdRelations(projectElement);
        m_first = false;
        m_data.clear();
    } else {
        MacroCommand::execute();
    }
}

void InsertProjectXmlCommand::unexecute()
{
    MacroCommand::unexecute();
}

void InsertProjectXmlCommand::createCmdAccounts(const KoXmlElement &projectElement)
{
    if (projectElement.isNull()) {
        return;
    }
}

void InsertProjectXmlCommand::createCmdCalendars(const KoXmlElement &projectElement)
{
    if (projectElement.isNull()) {
        return;
    }
}

void InsertProjectXmlCommand::createCmdResources(const KoXmlElement &projectElement)
{
    if (projectElement.isNull()) {
        return;
    }
}

void InsertProjectXmlCommand::createCmdTasks(const KoXmlElement &projectElement)
{
    if (projectElement.isNull()) {
        return;
    }
    createCmdTask(projectElement, m_project, m_after);
}

void InsertProjectXmlCommand::createCmdTask(const KoXmlElement &parentElement, Node *parent, Node *after)
{
    KoXmlElement taskElement;
    forEachElement(taskElement, parentElement) {
        if (taskElement.tagName() != "task") {
            continue;
        }
        Task *task = m_project->createTask();
        QString id = task->id();
        m_oldIds.insert(id, task);
        task->load(taskElement, m_context);
        task->setId(id);
        NamedCommand *cmd = new AddTaskCommand(m_project, parent, task, after);
        cmd->execute();
        addCommand(cmd);
//         createCmdRequests(taskElement, task);

        createCmdTask(taskElement, task); // add children
    }
}

void InsertProjectXmlCommand::createCmdRelations(const KoXmlElement &projectElement)
{
    if (projectElement.isNull()) {
        return;
    }
    KoXmlElement relationElement;
    forEachElement(relationElement, projectElement) {
        if (relationElement.tagName() != "relation") {
            continue;
        }
        Node *parent = m_oldIds.value(relationElement.attribute("parent"));
        Node *child = m_oldIds.value(relationElement.attribute("child"));
        if (parent && child) {
            Relation *relation = new Relation(parent, child);
            relation->setType(relationElement.attribute("type"));
            relation->setLag(Duration::fromString(relationElement.attribute("lag")));
            AddRelationCmd *cmd = new AddRelationCmd(*m_project, relation);
            cmd->execute();
            addCommand(cmd);
        }
    }
}
