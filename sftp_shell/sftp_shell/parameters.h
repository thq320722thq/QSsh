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

#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <sshconnection.h>

typedef enum
{
    e_opt_get = 1,
    e_opt_put,
    e_opt_del,
    e_opt_mkdir,
    e_opt_rmdir,
    e_opt_updir,
    e_opt_downdir,
    e_opt_statfile
}SFTP_OPT_TYPE;


struct Parameters {
    QSsh::SshConnectionParameters sshParams;
    int oper_type;     /*操作类型@SFTP_OPT_TYPE*/
    QString source;
    QString dest;
    int smallFileCount;
    int bigFileSize;
};

#endif // PARAMETERS_H
