/* This file is part of the KDE project
 * Copyright (C) 2009, 2011, 2012 Dag Andersen <danders@get2net.dk>
 * Copyright (C) 2019 Dag Andersen <danders@get2net.dk>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

// clazy:excludeall=qstring-arg
#include "icalendarexport.h"
#include "config.h"

#include <kptmaindocument.h>
#include <kpttask.h>
#include <kptnode.h>
#include <kptresource.h>
#include <kptdocuments.h>
#include "kptdebug.h"

#if 0
#include <kcalcore/attendee.h>
#include <kcalcore/attachment.h>
#include <kcalcore/icalformat.h>
#include <kcalcore/memorycalendar.h>
#endif

#include <QTextCodec>
#include <QByteArray>
#include <QString>
#include <QFile>

#include <kpluginfactory.h>

#include <KoFilterChain.h>
#include <KoFilterManager.h>
#include <KoDocument.h>


using namespace KPlato;

K_PLUGIN_FACTORY_WITH_JSON(ICalendarExportFactory, "plan_icalendar_export.json",
                           registerPlugin<ICalendarExport>();)

#ifdef HAVE_QDATETIME_KCALCORE
#define KQDT QDateTime
#else
#define KQDT KDateTime
#endif

ICalendarExport::ICalendarExport(QObject* parent, const QVariantList &)
        : KoFilter(parent)
{
}

KoFilter::ConversionStatus ICalendarExport::convert(const QByteArray& from, const QByteArray& to)
{
    debugPlan << from << to;
    if ( ( from != "application/x-vnd.kde.plan" ) || ( to != "text/calendar" ) ) {
        return KoFilter::NotImplemented;
    }
    bool batch = false;
    if ( m_chain->manager() ) {
        batch = m_chain->manager()->getBatchMode();
    }
    if ( batch ) {
        //TODO
        debugPlan << "batch";
        return KoFilter::UsageError;
    }
    debugPlan<<"online:"<<m_chain->inputDocument();
    MainDocument *doc = dynamic_cast<MainDocument*>( m_chain->inputDocument() );
    if (doc == 0) {
        errorPlan << "Cannot open Plan document";
        return KoFilter::InternalError;
    }
    if (m_chain->outputFile().isEmpty()) {
        errorPlan << "Output filename is empty";
        return KoFilter::InternalError;
    }
    QFile file(m_chain->outputFile());
    if (! file.open(QIODevice::WriteOnly)) {
        errorPlan << "Failed to open output file:" << file.fileName();
        return KoFilter::StorageCreationError;
    }

    KoFilter::ConversionStatus status = convert(doc->getProject(), file);
    file.close();
    //debugPlan << "Finished with status:"<<status;
    return status;
}

long scheduleId(const Project &project)
{
    //TODO: schedule selection dialog
    long sid = ANYSCHEDULED;
    bool baselined = project.isBaselined(sid);
    QList<ScheduleManager*> lst = project.allScheduleManagers();
    foreach(const ScheduleManager *m, lst) {
        if (! baselined) {
            sid = lst.last()->scheduleId();
            //debugPlan<<"last:"<<sid;
            break;
        }
        if (m->isBaselined()) {
            sid = m->scheduleId();
            //debugPlan<<"baselined:"<<sid;
            break;
        }
    }
    return sid;
}

QString beginCalendar()
{
    QString s;
    s += QString("BEGIN:VCALENDAR") + "\r\n";
    s += QString("PRODID:-//K Desktop Environment//NONSGML libkcal 4.3//EN") + "\r\n";
    s += QString("VERSION:2.0") + "\r\n";
    s += QString("X-KDE-ICAL-IMPLEMENTATION-VERSION:1.0") + "\r\n";
    return s;
}
QString endCalendar()
{
    return QString() + QString("END:VCALENDAR") + "\r\n";
}

QString dtToString(const QDateTime &dt)
{
    return dt.toUTC().toString("yyyyMMddTHHmmssZ"); // 20160707T010000Z
}

QString doAttendees(const Node &node, long sid)
{
    QString s;
    Schedule *schedule = node.schedule(sid);
    if (schedule) {
        foreach (const Resource *r, schedule->resources()) {
            if (r->type() == Resource::Type_Work) {
                s += QString("ATTENDEE;CN=") + r->name() + "\r\n";
                s += QString(";RSVP=FALSE;PARTSTAT=NEEDS-ACTION;ROLE=REQ-PARTICIPANT;") + "\r\n";
                s += QString("CUTYPE=INDIVIDUAL;") + "\r\n";
                s += QString("X-UID=") + r->id().right(10) + ':' + r->email() + "\r\n";
            }
        }
    } else {
        const QList<Resource*> lst = static_cast<const Task&>(node).requestedResources();
        foreach(const Resource *r, lst) {
            if (r->type() == Resource::Type_Work) {
                s += QString("ATTENDEE;CN=") + r->name() + "\r\n";
                s += QString(";RSVP=FALSE;PARTSTAT=NEEDS-ACTION;ROLE=REQ-PARTICIPANT;") + "\r\n";
                s += QString("CUTYPE=INDIVIDUAL;") + "\r\n";
                s += QString("X-UID=") + r->id().right(10) + ':' + r->email() + "\r\n";
            }
        }
    }
    return s;
}

QString doAttachment(const Documents &docs)
{
    QString s;
    foreach(const Document *doc, docs.documents()) {
        s += QString("ATTACH:") + doc->url().url() + "\r\n";
    }
    return s;
}

QString doDescription(const QString &description)
{
    QString s = QString("DESCRIPTION;X-KDE-TEXTFORMAT=HTML:") + description;
    if (!s.contains("\r\n")) {
        s.replace("\n", "\r\n");
    }
    if (!s.endsWith('\n')) {
        s += "\r\n";
    }
    return s;
}

QString createTodo(const Node &node, long sid)
{
    QString s;
    s += QString("BEGIN:VTODO") + "\r\n";
    s += QString("SUMMARY:") + node.name() + "\r\n";
    s += doDescription(node.description());
    s += QString("UID:") + node.id().right(10) + "\r\n";
    s += QString("DTSTAMP:") + dtToString(QDateTime::currentDateTime()) + "\r\n";
    s += QString("CREATED:") + dtToString(QDateTime::currentDateTime()) + "\r\n";
    s += QString("LAST-MODIFIED:") + dtToString(QDateTime::currentDateTime()) + "\r\n";
    s += QString("CATEGORIES:Plan") + "\r\n";
    s += QString("DTSTART:") + dtToString(node.startTime(sid)) + "\r\n";
    s += QString("DUE:") + dtToString(node.endTime(sid)) + "\r\n";
    if (node.parentNode()) {
        s += QString("RELATED-TO:") + node.parentNode()->id().right(10) + "\r\n";
    }
    if (node.type() == Node::Type_Task) {
        s += QString("PERCENT-COMPLETE:") + QString::number(static_cast<const Task&>(node).completion().percentFinished()) + "\r\n";
        s += doAttendees(node, sid);
    } else if (node.type() == Node::Type_Milestone) {
        s += QString("PERCENT-COMPLETE:") + QString::number(static_cast<const Task&>(node).completion().percentFinished()) + "\r\n";
    } else if (node.type() == Node::Type_Project) {
        if (!node.leader().isEmpty()) {
            s += QString("ORGANIZER:") + node.leader() + "\r\n";
        }
    }
    s += doAttachment(node.documents());
    s += QString("END:VTODO") + "\r\n";
    return s;
}

QString doNode(const Node *node, long sid)
{
    QString s = createTodo(*node, sid);
    for (int i = 0; i < node->numChildren(); ++i) {
        s += doNode(node->childNode(i), sid);
    }
    return s;
}

void formatData(QString &data, int pos = 0)
{
    if (pos < data.length() - 70) {
        int next = data.indexOf("\r\n", pos);
        if (next > pos && next < pos + 70) {
            pos = next;
        } else if (next > pos + 70 || data.length() > pos + 70) {
            pos += 70;
            data.insert(pos, "\r\n");
        }
        formatData(data, pos+2);
    }
    while (data.contains("\r\n\r\n")) {
        data.replace("\r\n\r\n", "\r\n");
    }
}

KoFilter::ConversionStatus ICalendarExport::convert(const Project &project, QFile &file)
{
    long sid = scheduleId(project);
    QString data = beginCalendar();
    data += doNode(&project, sid);
    data += endCalendar();

    formatData(data);

    qint64 n = file.write(data.toUtf8());
    if (n < 0) {
        return KoFilter::InternalError;
    }
    return KoFilter::OK;
}

#if 0
KoFilter::ConversionStatus ICalendarExport::convert(const Project &project, QFile &file)
{
    KCalCore::Calendar::Ptr cal(new KCalCore::MemoryCalendar("UTC"));

    //TODO: schedule selection dialog
    long id = ANYSCHEDULED;
    bool baselined = project.isBaselined(id);
    QList<ScheduleManager*> lst = project.allScheduleManagers();
    foreach(const ScheduleManager *m, lst) {
        if (! baselined) {
            id = lst.last()->scheduleId();
            //debugPlan<<"last:"<<id;
            break;
        }
        if (m->isBaselined()) {
            id = m->scheduleId();
            //debugPlan<<"baselined:"<<id;
            break;
        }
    }
    //debugPlan<<id;
    createTodos(cal, &project, id);

    KCalCore::ICalFormat format;
    qint64 n = file.write(format.toString(cal).toUtf8());
    if (n < 0) {
        return KoFilter::InternalError;
    }
    return KoFilter::OK;
}

void ICalendarExport::createTodos(KCalCore::Calendar::Ptr cal, const Node *node, long id, KCalCore::Todo::Ptr parent)
{
    KCalCore::Todo::Ptr todo(new KCalCore::Todo());
    todo->setUid( node->id() );
    todo->setSummary(node->name());
    todo->setDescription(node->description());
    todo->setCategories(QLatin1String("Plan"));
    if (! node->projectNode()->leader().isEmpty()) {
        todo->setOrganizer(node->projectNode()->leader());
    }
    if ( node->type() != Node::Type_Project && ! node->leader().isEmpty()) {
#if KCALCORE_HAVE_NO_PERSION_PTR
        KCalCore::Person p = KCalCore::Person::fromFullName(node->leader());
        KCalCore::Attendee::Ptr a(new KCalCore::Attendee(p.name(), p.email()));
#else
        KCalCore::Person::Ptr p = KCalCore::Person::fromFullName(node->leader());
        KCalCore::Attendee::Ptr a(new KCalCore::Attendee(p->name(), p->email()));
#endif
        a->setRole(KCalCore::Attendee::NonParticipant);
        todo->addAttendee(a);
    }
    DateTime st = node->startTime(id);
    DateTime et = node->endTime(id);
    if (st.isValid()) {
        todo->setDtStart( KQDT( st ) );
    }
    if (et.isValid()) {
        todo->setDtDue( KQDT( et ) );
    }
    if (node->type() == Node::Type_Task) {
        const Task *task = qobject_cast<Task*>(const_cast<Node*>(node));
        Schedule *s = task->schedule(id);
        if (id < 0 || s == 0) {
            // Not scheduled, use requests
            const QList<Resource*> lst = task->requestedResources();
            foreach(const Resource *r, lst) {
                if (r->type() == Resource::Type_Work) {
                    todo->addAttendee(KCalCore::Attendee::Ptr(new KCalCore::Attendee(r->name(), r->email())));
                }
            }
        } else {
            foreach(const Resource *r, s->resources()) {
                if (r->type() == Resource::Type_Work) {
                    todo->addAttendee(KCalCore::Attendee::Ptr(new KCalCore::Attendee(r->name(), r->email())));
                }
            }

        }
    } else if (node->type() == Node::Type_Milestone) {
        const Task *task = qobject_cast<Task*>(const_cast<Node*>(node));
        todo->setDtStart(KQDT());
        todo->setPercentComplete(task->completion().percentFinished());
    }
    foreach(const Document *doc, node->documents().documents()) {
        todo->addAttachment(KCalCore::Attachment::Ptr(new KCalCore::Attachment(doc->url().url())));
    }
    if (! parent.isNull()) {
        todo->setRelatedTo(parent->uid(), KCalCore::Incidence::RelTypeParent);
    }
    cal->addTodo(todo);
    foreach(const Node *n, node->childNodeIterator()) {
        createTodos(cal, n, id, todo);
    }
}
#endif
#include "icalendarexport.moc"
