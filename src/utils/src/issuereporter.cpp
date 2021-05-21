/*
 * Copyright (C) 2021 Anton Filimonov
 *
 * This file is part of klogg.
 *
 * klogg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * klogg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with klogg.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QDesktopServices>
#include <QMessageBox>
#include <QUrl>
#include <QUrlQuery>

#include "klogg_version.h"

#include "issuereporter.h"

static constexpr auto DetailsFooter
    = "-------------------------\n"
      "Useful extra information\n"
      "-------------------------\n"
      "> Klogg version %1 (built on %2 from commit %3) [built for %4]\n"
      "> running on %5 (%6/%7) [%8]\n"
      "> and Qt %9";

static constexpr auto DetailsHeader = "Details for the issue\n"
                                      "--------------------\n\n";

static constexpr auto CrashTemplate = "#### What did you do?\n\n\n"

                                      "-------------------------\n"
                                      "Crash id:\n"
                                      "%1\n";

static constexpr auto ExceptionTemplate = "#### What did you do?\n\n\n"

                                          "-------------------------\n"
                                          "Exception:\n"
                                          "%1\n";

static constexpr auto BugTemplate = "#### What did you do?\n\n\n"
                                    "#### What did you expect to see?\n\n\n"
                                    "#### What did you see instead?\n\n\n";

static constexpr auto BugLabel = "type: bug";
static constexpr auto CrashLabel = "type: crash";

static constexpr auto ExceptionAskUserAction
    = "Ooops! Something unexpected happend. Create issue on Github?";

static constexpr auto AskUserAction = "Create issue on Github?";

void IssueReporter::askUserAndReportIssue( IssueTemplate issueTemplate, const QString& information )
{
    const auto askAction
        = issueTemplate == IssueTemplate::Exception ? ExceptionAskUserAction : AskUserAction;

    if ( QMessageBox::Yes
         == QMessageBox::question( nullptr, "Klogg", askAction, QMessageBox::Yes,
                                   QMessageBox::No ) ) {
        IssueReporter::reportIssue( issueTemplate, information );
    }
}

void IssueReporter::reportIssue( IssueTemplate issueTemplate, const QString& information )
{

    QString body = DetailsHeader;
    QString label;
    switch ( issueTemplate ) {
    case IssueTemplate::Bug:
        body.append( BugTemplate );
        label = BugLabel;
        break;
    case IssueTemplate::Crash:
        body.append( QString( CrashTemplate ).arg( information ) );
        label = CrashLabel;
        break;
    case IssueTemplate::Exception:
        body.append( QString( ExceptionTemplate ).arg( information ) );
        label = CrashLabel;
        break;
    }

    const auto version = kloggVersion();
    const auto buildDate = kloggBuildDate();
    const auto commit = kloggCommit();

    const auto os = QSysInfo::prettyProductName();
    const auto kernelType = QSysInfo::kernelType();
    const auto kernelVersion = QSysInfo::kernelVersion();
    const auto arch = QSysInfo::currentCpuArchitecture();
    const auto builtAbi = QSysInfo::buildAbi();

    body.append( QString( DetailsFooter )
                     .arg( version, buildDate, commit, builtAbi, os, kernelType, kernelVersion,
                           arch, qVersion() ) );

    QUrlQuery query;
    query.addQueryItem( "labels", label );
    query.addQueryItem( "body", body );

    QUrl url( "https://github.com/variar/klogg/issues/new" );
    url.setQuery( query );
    QDesktopServices::openUrl( url );
}