#ifndef LPFFTPMODEL_H
#define LPFFTPMODEL_H

#include <QStandardItemModel>
#include <QObject>
#include <QFileIconProvider>

#include "FTP/include/LPTF_Net/LPTF_Socket.hpp"

class LpfFTPModel : public QStandardItemModel
{
    Q_OBJECT
public:
    LpfFTPModel(QObject *parent,
                QString &ip,
                int port,
                QString &username,
                QString &password);

    ~LpfFTPModel();

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    bool isRootFolder(const QModelIndex &index);
    bool isFolder(const QModelIndex &index);
    bool isFile(const QModelIndex &index);
    QString getFullPath(const QModelIndex &index);

    void uploadFile(const QString &file, const QString &path);
    void downloadFile(const QString &outfile, const QString &path);
    void deleteFile(const QString &filepath);
    void createFolder(const QString &name, const QString &path);
    void deleteFolder(const QString &path);
    void renameFolder(const QString &name, const QString &path);

public Q_SLOTS:
    void queryData();

signals:
    void onCommandFail(const QString &message);
    void onConnectionError(const QString &message);

private:
    QFileIconProvider *iconProvider;

    QString &ip;
    int port;
    QString &username;
    QString &password;

    LPTF_Socket *connect_to_server();
    bool login(LPTF_Socket *clientSocket);
};

#endif // LPFFTPMODEL_H
