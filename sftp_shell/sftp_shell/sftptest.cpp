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

#include "sftptest.h"
#include <QThread>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <iostream>

using namespace QSsh;

SftpTest::SftpTest(const Parameters &params)
    : m_parameters(params), m_state(Inactive), m_error(false), m_connection(0),
      m_mkdirJob(SftpInvalidJob),
      m_statDirJob(SftpInvalidJob),
      m_lsDirJob(SftpInvalidJob),
      m_rmDirJob(SftpInvalidJob)
{
}

SftpTest::~SftpTest()
{
    removeFiles(true);
    delete m_connection;
}

void SftpTest::run()
{
    m_connection = new SshConnection(m_parameters.sshParams);
    connect(m_connection, SIGNAL(connected()), SLOT(handleConnected()));
    connect(m_connection, SIGNAL(error(QSsh::SshError)), SLOT(handleError()));
    connect(m_connection, SIGNAL(disconnected()), SLOT(handleDisconnected()));
    std::cout << "Connecting to host "
        << qPrintable(m_parameters.sshParams.host) << "'..." << std::endl;
    m_state = Connecting;
    m_connection->connectToHost();
}

void SftpTest::handleConnected()
{
    if (m_state != Connecting) {
        std::cerr << "Unexpected state " << m_state << " in function "
            << Q_FUNC_INFO << "." << std::endl;
        earlyDisconnectFromHost();
    } else {
        std::cout << "Connected. Initializing SFTP channel..." << std::endl;
        m_channel = m_connection->createSftpChannel();
        connect(m_channel.data(), SIGNAL(initialized()), this,
           SLOT(handleChannelInitialized()));
        connect(m_channel.data(), SIGNAL(channelError(QString)), this,
            SLOT(handleChannelInitializationFailure(QString)));
        connect(m_channel.data(), SIGNAL(finished(QSsh::SftpJobId,QString)),
            this, SLOT(handleJobFinished(QSsh::SftpJobId,QString)));
        connect(m_channel.data(),
            SIGNAL(fileInfoAvailable(QSsh::SftpJobId,QList<QSsh::SftpFileInfo>)),
            SLOT(handleFileInfo(QSsh::SftpJobId,QList<QSsh::SftpFileInfo>)));
        connect(m_channel.data(), SIGNAL(closed()), this,
            SLOT(handleChannelClosed()));
        m_state = InitializingChannel;
        m_channel->initialize();
    }
}

void SftpTest::handleDisconnected()
{
    if (m_state != Disconnecting) {
        std::cerr << "Unexpected state " << m_state << " in function "
            << Q_FUNC_INFO << std::endl;
        m_error = true;
    } else {
        std::cout << "Connection closed." << std::endl;
    }
   // std::cout << "Test finished. ";
    if (m_error)
        std::cout << "There were errors.";
    else
        std::cout << "No errors encountered.";
    std::cout << std::endl;
    qApp->exit(m_error ? EXIT_FAILURE : EXIT_SUCCESS);
}

void SftpTest::handleError()
{
    std::cerr << "Encountered SSH error: "
        << qPrintable(m_connection->errorString()) << "." << std::endl;
    m_error = true;
    m_state = Disconnecting;
    qApp->exit(EXIT_FAILURE);
}


/*目前只支持单个文件操作*/
void SftpTest::handleChannelInitialized()
{
    if (m_state != InitializingChannel) {
        std::cerr << "Unexpected state " << m_state << "in function "
            << Q_FUNC_INFO << "." << std::endl;
        earlyDisconnectFromHost();
        return;
    }

    switch(m_parameters.oper_type)
    {
        case e_opt_put:
        {
            std::cout << "Files initialized. Now uploading..." << std::endl;
            const QString localFile = m_parameters.source;
            const QString remoteFp
                = m_parameters.dest + "/" + QFileInfo(localFile).fileName();
            const SftpJobId uploadJob = m_channel->uploadFile(localFile,
                remoteFp, SftpOverwriteExisting);
            if (uploadJob == SftpInvalidJob)
            {
                std::cerr << "Error uploading local file "
                    << qPrintable(localFile) << " to remote file "
                    << qPrintable(remoteFp) << "." << std::endl;
                earlyDisconnectFromHost();
                return;
            }

            m_smallFilesUploadJobs.insert(uploadJob, remoteFp);
            m_state = UploadingSmall;
            break;
        }
        case e_opt_get:
        {
            sftp_file_get();
            break;
        }
        case e_opt_del:
        {
            m_localSmallFiles.clear();

            m_smallFilesRemovalJobs.insert(m_channel->removeFile(m_parameters.dest), m_parameters.dest);
            m_state = RemovingSmall;
            break;
        }
        case e_opt_mkdir:
        {
            m_remoteDirPath = m_parameters.dest;
            m_mkdirJob = m_channel->createDirectory(m_parameters.dest);
            m_state = CreatingDir;
            break;
        }
        case e_opt_rmdir:
        {
            m_rmDirJob = m_channel->removeDirectory(m_parameters.dest);
            m_state = RemovingDir;
            break;
        }
        case e_opt_updir:
        {
            m_upDirJob = m_channel->uploadDir(m_parameters.source, m_parameters.dest);
            m_dirUploadJobs.insert(m_upDirJob, m_parameters.source);
            m_state = UploadingDir;
            break;
        }
        case e_opt_downdir:
        {
            m_downDirJob = m_channel->listDirectory(m_parameters.source);
            m_dirDownloadJobs.insert(m_downDirJob, m_parameters.source);
            m_state = DownloadingDir;
            break;
        }
        case e_opt_statfile:
        {
            m_statJob = m_channel->statFile(m_parameters.source);
            m_statFilesJobs.insert(m_statJob, m_parameters.source);
            m_state = StatFiles;
            break;
        }
    }



}

void SftpTest::sftp_file_get()
{
    const QString localFilePath = m_parameters.dest + "/" + QFileInfo(m_parameters.source).fileName();
    const QString remoteFp = m_parameters.source;

    std::cout << "Now downloading " << remoteFp.toStdString() << std::endl;

    const SftpJobId downloadJob = m_channel->downloadFile(remoteFp,
        localFilePath, SftpOverwriteExisting);
    if (downloadJob == SftpInvalidJob) {
        std::cerr << "Error downloading remote file "
            << qPrintable(remoteFp) << " to local file "
            << qPrintable(localFilePath) << "." << std::endl;
        earlyDisconnectFromHost();
        return;
    }
    m_smallFilesDownloadJobs.insert(downloadJob, remoteFp);

    m_state = DownloadingSmall;

}

void SftpTest::handleChannelInitializationFailure(const QString &reason)
{
    std::cerr << "Could not initialize SFTP channel: " << qPrintable(reason)
        << "." << std::endl;
    earlyDisconnectFromHost();
}

void SftpTest::handleChannelClosed()
{
    if (m_state != ChannelClosing) {
        std::cerr << "Unexpected state " << m_state << " in function "
            << Q_FUNC_INFO << "." << std::endl;
    } else {
        std::cout << "SFTP channel closed. Now disconnecting..." << std::endl;
    }
    m_state = Disconnecting;
    m_connection->disconnectFromHost();

    qApp->exit(m_error ? EXIT_FAILURE : EXIT_SUCCESS);
}

void SftpTest::handleJobFinished(SftpJobId job, const QString &error)
{
    switch (m_state)
    {
    case UploadingSmall:
        if (!handleJobFinished(job, m_smallFilesUploadJobs, error, "uploading"))
            return;
        if (m_smallFilesUploadJobs.isEmpty()) {
            std::cout << "Uploading finished."
                << std::endl;

            m_channel->closeChannel();
        }

        break;
    case DownloadingSmall:
        if (!handleJobFinished(job, m_smallFilesDownloadJobs, error, "downloading"))
            return;
        if (m_smallFilesDownloadJobs.isEmpty()) {
            std::cout << "Downloading finished, now comparing..." << std::endl;
            foreach (const FilePtr &ptr, m_localSmallFiles) {
                if (!ptr->open(QIODevice::ReadOnly)) {
                    std::cerr << "Error opening local file "
                        << qPrintable(ptr->fileName()) << "." << std::endl;
                    earlyDisconnectFromHost();
                    return;
                }
                const QString downloadedFilePath = cmpFileName(ptr->fileName());
                QFile downloadedFile(downloadedFilePath);
                if (!downloadedFile.open(QIODevice::ReadOnly)) {
                    std::cerr << "Error opening downloaded file "
                        << qPrintable(downloadedFilePath) << "." << std::endl;
                    earlyDisconnectFromHost();
                    return;
                }
                if (!compareFiles(ptr.data(), &downloadedFile))
                    return;
            }

            std::cout << "Comparisons successful."
                << std::endl;

            m_state = ChannelClosing;
            m_channel->closeChannel();
        }
        break;
    case RemovingSmall:
        if (!handleJobFinished(job, m_smallFilesRemovalJobs, error, "removing"))
            return;
        if (m_smallFilesRemovalJobs.isEmpty()) {
            std::cout << "Files successfully removed. "<< std::endl;

            m_state = ChannelClosing;
            m_channel->closeChannel();
        }

        break;

    case CreatingDir:
        if (!handleJobFinished(job, m_mkdirJob, error, "creating remote directory"))
            return;
        std::cout << "Directory successfully created"
            << std::endl;
        m_state = ChannelClosing;
        m_channel->closeChannel();
        break;

    case RemovingDir:
        if (!handleJobFinished(job, m_rmDirJob, error, "removing directory"))
            return;
        std::cout << "Directory successfully removed. Now closing the SFTP channel..." << std::endl;
        m_state = ChannelClosing;
        m_channel->closeChannel();
        break;
    case UploadingDir:
        if (!handleJobFinished(job, m_upDirJob, error, "uploading directory"))
            return;
        std::cout << "Uploading directory successfully. Now closing the SFTP channel..." << std::endl;
        m_state = ChannelClosing;
        m_channel->closeChannel();
        break;
    case DownloadingDir:
    {
        if (!handleJobFinished(job, m_dirDownloadJobs, error, "list files infomations"))
            return;
        std::cout << "List files infomations successful: Total-" << m_dirContents.count() << std::endl;
        std::cout << "Now downloading files..." << std::endl;
        QString type;
        QString file = m_parameters.source;

        foreach (QSsh::SftpFileInfo fileInfo, m_dirContents) {
            switch(fileInfo.type)
            {
                case QSsh::SftpFileType::FileTypeRegular:
                {
                    type = "file";

                    m_parameters.source = file + "/" + fileInfo.name;
                    sftp_file_get();
                    break;
                }
                case QSsh::SftpFileType::FileTypeDirectory:
                {
                    type = "dir";
                    std::cout << "Skip directory: " << fileInfo.name.toStdString() << std::endl;
                    break;
                }
                case QSsh::SftpFileType::FileTypeOther:
                {
                    type = "other";
                    break;
                }
                case QSsh::SftpFileType::FileTypeUnknown:
                {
                    type = "unknow";
                    break;
                }
                default:
                type = "unknow";
                break;
            }
           // std::cout <<"file:" << fileInfo.name.toStdString() << " size:" << fileInfo.size << " type:" << type.toStdString() << std::endl;

        }

        break;
    }
    case Disconnecting:
        break;
    default:
        if (!m_error) {
            std::cerr << "Unexpected state " << m_state << " in function "
                << Q_FUNC_INFO << "." << std::endl;
            earlyDisconnectFromHost();
        }
    }
}

void SftpTest::handleFileInfo(SftpJobId job, const QList<SftpFileInfo> &fileInfoList)
{
    switch (m_state) {
    case CheckingDirAttributes: {
        static int count = 0;
        if (!checkJobId(job, m_statDirJob, "checking directory attributes"))
            return;
        if (++count > 1) {
            std::cerr << "Error: More than one reply for directory attributes check." << std::endl;
            earlyDisconnectFromHost();
            return;
        }
        m_dirInfo = fileInfoList.first();
        break;
    }
    case CheckingDirContents:
        if (!checkJobId(job, m_lsDirJob, "checking directory contents"))
            return;

        m_dirContents.clear();
        m_dirContents << fileInfoList;
        break;
    case DownloadingDir:
        if (!checkJobId(job, m_downDirJob, "checking files infomations"))
            return;

        m_dirContents.clear();
        m_dirContents << fileInfoList;
        break;
    default:
        std::cerr << "Error: Unexpected file info in state " << m_state << "." << std::endl;
        earlyDisconnectFromHost();
    }
}

void SftpTest::removeFile(const FilePtr &file, bool remoteToo)
{
    if (!file)
        return;
    const QString localFilePath = file->fileName();
    file->remove();
    QFile::remove(cmpFileName(localFilePath));
    if (remoteToo && m_channel
            && m_channel->state() == SftpChannel::Initialized)
        m_channel->removeFile(remoteFilePath(QFileInfo(localFilePath).fileName()));
}

QString SftpTest::cmpFileName(const QString &fileName) const
{
    return fileName + QLatin1String(".cmp");
}

QString SftpTest::remoteFilePath(const QString &localFileName) const
{
    return QLatin1String("/tmp/") + localFileName + QLatin1String(".upload");
}

void SftpTest::earlyDisconnectFromHost()
{
    m_error = true;
    removeFiles(true);
    if (m_channel)
        disconnect(m_channel.data(), 0, this, 0);
    m_state = Disconnecting;
    m_connection->disconnectFromHost();

}

bool SftpTest::checkJobId(SftpJobId job, SftpJobId expectedJob, const char *activity)
{
    if (job != expectedJob) {
        std::cerr << "Error " << activity << ": Expected job id " << expectedJob
           << ", got job id " << job << '.' << std::endl;
        earlyDisconnectFromHost();
        return false;
    }
    return true;
}

void SftpTest::removeFiles(bool remoteToo)
{
    foreach (const FilePtr &file, m_localSmallFiles)
        removeFile(file, remoteToo);
}

bool SftpTest::handleJobFinished(SftpJobId job, JobMap &jobMap,
    const QString &error, const char *activity)
{
    JobMap::Iterator it = jobMap.find(job);
    if (it == jobMap.end()) {
        std::cerr << "Error: Unknown job " << job << "finished."
            << std::endl;
        earlyDisconnectFromHost();
        return false;
    }
    if (!error.isEmpty()) {
        std::cerr << "Error " << activity << " file " << qPrintable(it.value())
            << ": " << qPrintable(error) << "." << std::endl;
        earlyDisconnectFromHost();
        return false;
    }
    jobMap.erase(it);
    return true;
}

bool SftpTest::handleJobFinished(SftpJobId job, SftpJobId expectedJob, const QString &error,
    const char *activity)
{
    if (!checkJobId(job, expectedJob, activity))
        return false;
    if (!error.isEmpty()) {
        std::cerr << "Error " << activity << ": " << qPrintable(error) << "." << std::endl;
        earlyDisconnectFromHost();
        return false;
    }
    return true;
}


bool SftpTest::compareFiles(QFile *orig, QFile *copy)
{
    bool success = orig->size() == copy->size();
    qint64 bytesLeft = orig->size();
    orig->seek(0);
    while (success && bytesLeft > 0) {
        const qint64 bytesToRead = qMin(bytesLeft, Q_INT64_C(1024*1024));
        const QByteArray origBlock = orig->read(bytesToRead);
        const QByteArray copyBlock = copy->read(bytesToRead);
        if (origBlock.size() != bytesToRead || origBlock != copyBlock)
            success = false;
        bytesLeft -= bytesToRead;
    }
    orig->close();
    success = success && orig->error() == QFile::NoError
        && copy->error() == QFile::NoError;
    if (!success) {
        std::cerr << "Error: Original file '" << qPrintable(orig->fileName())
            << "'' differs from downloaded file '"
            << qPrintable(copy->fileName()) << "'." << std::endl;
        earlyDisconnectFromHost();
    }
    return success;
}
