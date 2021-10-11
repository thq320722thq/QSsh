/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "argumentscollector.h"

#include <iostream>

using namespace QSsh;

using namespace std;

ArgumentsCollector::ArgumentsCollector(const QStringList &args)
    : m_arguments(args)
{
}

Parameters ArgumentsCollector::collect(bool &success) const
{
    Parameters parameters;
    parameters.sshParams.options &= ~SshIgnoreDefaultProxy;
    try {
        bool authTypeGiven = false;
        bool optGiven  = false;
        int pos;
        for (pos = 1; pos < m_arguments.count() - 1; ++pos)
        {
            if (checkAndSetStringArg(pos, parameters.sshParams.host, "-h")
                || checkAndSetStringArg(pos, parameters.sshParams.userName, "-u"))
                continue;

            if (checkAndSetStringArg(pos, parameters.sshParams.password, "-pwd")) {
                parameters.sshParams.authenticationType
                    = SshConnectionParameters::AuthenticationTypeTryAllPasswordBasedMethods;
                authTypeGiven = true;
                continue;
            }

            if (checkAndSetIntArg(pos, parameters.oper_type, optGiven, "-o")) {
                continue;
            }

            if (checkAndSetStringArg(pos, parameters.source, "-s")) {
                continue;
            }

            if (checkAndSetStringArg(pos, parameters.dest, "-d")) {
                continue;
            }
        }

        Q_ASSERT(pos <= m_arguments.count());

        if (parameters.sshParams.host.isEmpty())
            throw ArgumentErrorException(QLatin1String("No host given."));
        if (parameters.sshParams.userName.isEmpty())
            throw ArgumentErrorException(QLatin1String("No user name given."));

        parameters.sshParams.port = 22;
        parameters.sshParams.timeout = 30;
        success = true;
    } catch (ArgumentErrorException &ex) {
        cerr << "Error: " << qPrintable(ex.error) << endl;
        printUsage();
        success = false;
    }
    return parameters;
}

void ArgumentsCollector::printUsage() const
{
    cerr << "Usage: " << qPrintable(m_arguments.first())
        << " -h <host> -u <user> "
        << "-pwd <password> "
        << "[ -o <operation> ] [ -s <source file> ] "
        << "[ -d <dest> ]" << endl;
}

bool ArgumentsCollector::checkAndSetStringArg(int &pos, QString &arg, const char *opt) const
{
    if (m_arguments.at(pos) == QLatin1String(opt)) {
        if (!arg.isEmpty()) {
            throw ArgumentErrorException(QLatin1String("option ") + QLatin1String(opt)
                + QLatin1String(" was given twice."));
        }
        arg = m_arguments.at(++pos);
        if (arg.isEmpty() && QLatin1String(opt) != QLatin1String("-pwd"))
            throw ArgumentErrorException(QLatin1String("empty argument not allowed here."));
        return true;
    }
    return false;
}

bool ArgumentsCollector::checkAndSetIntArg(int &pos, int &val,
    bool &alreadyGiven, const char *opt) const
{
    if (m_arguments.at(pos) == QLatin1String(opt)) {
        if (alreadyGiven) {
            throw ArgumentErrorException(QLatin1String("option ") + QLatin1String(opt)
                + QLatin1String(" was given twice."));
        }

        if(m_arguments.at(pos+1) == "get")
        {
            val = e_opt_get;
        }
        else if(m_arguments.at(pos+1) == "put")
        {
            val = e_opt_put;
        }
        else if(m_arguments.at(pos+1) == "del")
        {
            val = e_opt_del;
        }
        else if(m_arguments.at(pos+1) == "mkdir")
        {
            val = e_opt_mkdir;
        }
        else if(m_arguments.at(pos+1) == "rmdir")
        {
            val = e_opt_rmdir;
        }
        else if(m_arguments.at(pos+1) == "updir")
        {
            val = e_opt_updir;
        }
        else if(m_arguments.at(pos+1) == "statf")
        {
            val = e_opt_statfile;
        }
        else if(m_arguments.at(pos+1) == "downdir")
        {
            val = e_opt_downdir;
        }
        else
        {
            val = 0;
        }

        if (!val) {
            throw ArgumentErrorException(QLatin1String("option ") + QLatin1String(opt)
                 + QLatin1String(" invalid"));
        }
        alreadyGiven = true;
        return true;
    }
    return false;
}

bool ArgumentsCollector::checkForNoProxy(int &pos, SshConnectionOptions &options,
                                         bool &alreadyGiven) const
{
    if (m_arguments.at(pos) == QLatin1String("-no-proxy")) {
        if (alreadyGiven)
            throw ArgumentErrorException(QLatin1String("proxy setting given twice."));
        options |= SshIgnoreDefaultProxy;
        alreadyGiven = true;
        return true;
    }
    return false;
}
