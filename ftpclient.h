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
              QString &ip,
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
    void onDeleteRootAction();

    void queryUserTree();

    void onCommandFail(const QString &message);
    void onConnectionError(const QString &error);

    void askChangeConnection();

    // drag n drop related
    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dragMoveEvent(QDragMoveEvent *event);
    virtual void dropEvent(QDropEvent *event);

private:
    QTreeView *user_tree;
    QMenu *fileMenu;
    QMenu *folderMenu;
    QMenu *rootFolderMenu;

    QAction *uploadFileAction;
    QAction *downloadFileAction;
    QAction *deleteFileAction;
    QAction *createFolderAction;
    QAction *deleteFolderAction;
    QAction *renameFolderAction;

    QAction *deleteRootAction;

    LpfFTPModel *ftp_model;
};
#endif // FTPCLIENT_H
