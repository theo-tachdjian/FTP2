#ifndef FTPCLIENT_H
#define FTPCLIENT_H

#include <QWidget>
#include <QTreeView>

#include "lpfftpmodel.h"

class FTPClient : public QWidget
{
    Q_OBJECT

public:
    FTPClient(QWidget *parent,
              const char * ip,
              int port,
              QString &username,
              QString &password);
    ~FTPClient();

public Q_SLOTS:
    void onCustomContextMenu(const QPoint &point);

    void onUploadFileAction();
    void onDownloadFileAction();
    void onDeleteFileAction();
    void onCreateFolderAction();
    void onDeleteFolderAction();
    void onRenameFolderAction();

    void queryUserTree();

    void onCommandFail(const QString &message);
    void onConnectionError(const QString &error);

signals:
    // used when an action completed to query the user tree
    void onActionPerformed();


private:
    QTreeView *user_tree;
    QMenu *fileMenu;
    QMenu *folderMenu;

    QAction *uploadFileAction;
    QAction *downloadFileAction;
    QAction *deleteFileAction;
    QAction *createFolderAction;
    QAction *deleteFolderAction;
    QAction *renameFolderAction;

    LpfFTPModel *ftp_model;
};
#endif // FTPCLIENT_H
