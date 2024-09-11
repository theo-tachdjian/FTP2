#include "ftpclient.h"
#include <QTreeView>
#include <QVBoxLayout>
#include <QMenu>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>

FTPClient::FTPClient(QWidget *parent,
                     const char * ip,
                     int port,
                     QString &username,
                     QString &password)
    : QWidget(parent)
    , user_tree(new QTreeView(this))
    , ftp_model(new LpfFTPModel(this, ip, port, username, password))
{
    fileMenu = new QMenu(user_tree);
    folderMenu = new QMenu(user_tree);

    QAction *uploadFileAction = new QAction("Upload File...", folderMenu);
    connect(uploadFileAction, SIGNAL(triggered()), this, SLOT(onUploadFileAction()));

    QAction *downloadFileAction = new QAction("Download File", fileMenu);
    connect(downloadFileAction, SIGNAL(triggered()), this, SLOT(onDownloadFileAction()));

    QAction *deleteFileAction = new QAction("Delete File", fileMenu);
    connect(deleteFileAction, SIGNAL(triggered()), this, SLOT(onDeleteFileAction()));

    QAction *createFolderAction = new QAction("Create Folder", folderMenu);
    connect(createFolderAction, SIGNAL(triggered()), this, SLOT(onCreateFolderAction()));

    QAction *deleteFolderAction = new QAction("Delete Folder", folderMenu);
    connect(deleteFolderAction, SIGNAL(triggered()), this, SLOT(onDeleteFolderAction()));

    QAction *renameFolderAction = new QAction("Rename Folder", folderMenu);
    connect(renameFolderAction, SIGNAL(triggered()), this, SLOT(onRenameFolderAction()));

    fileMenu->addActions({downloadFileAction, deleteFileAction});
    folderMenu->addActions({uploadFileAction, createFolderAction, renameFolderAction, deleteFolderAction});

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(user_tree);

    user_tree->setHeaderHidden(true);
    user_tree->setSelectionMode(QTreeView::SingleSelection);
    user_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(user_tree, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(onCustomContextMenu(QPoint)));

    user_tree->setEditTriggers(QTreeView::NoEditTriggers);
    user_tree->setModel(ftp_model);

    this->setLayout(layout);

    connect(this, SIGNAL(onActionPerformed()), this, SLOT(queryUserTree()));
    connect(ftp_model, SIGNAL(onConnectionError(QString)), this, SLOT(onConnectionError(QString)));
    connect(ftp_model, SIGNAL(onCommandFail(QString)), this, SLOT(onCommandFail(QString)));

    this->installEventFilter(this); // TODO -> on widget close event for disconnect

    ftp_model->queryData();
}

void FTPClient::onCommandFail(const QString &message)
{
    QMessageBox::warning(this, "Command Failed", message);
}

void FTPClient::onConnectionError(const QString &error)
{
    QMessageBox::critical(this, "Connection Error", error);
}

void FTPClient::queryUserTree()
{
    ftp_model->queryData();
    user_tree->setExpanded(user_tree->model()->index(0, 0), true);
}


void FTPClient::onCustomContextMenu(const QPoint &point)
{
    QModelIndex index = user_tree->indexAt(point);
    if (index.isValid()) {

        if (ftp_model->isFolder(index)) {
            qDebug() << "Opening Folder menu";
            folderMenu->exec(user_tree->viewport()->mapToGlobal(point));
        } else {
            qDebug() << "Opening File menu";
            fileMenu->exec(user_tree->viewport()->mapToGlobal(point));
        }
    }
}

void FTPClient::onUploadFileAction()
{
    qDebug() << "onUploadFileAction";
    QModelIndex index = user_tree->currentIndex();

    if (index.isValid() && ftp_model->isFolder(index)) {
        qDebug() << ftp_model->getFullPath(index);

        QString file = QFileDialog::getOpenFileName(this, "Select File to Upload");
        if (file.isEmpty()) { return ; }

        QString path = ftp_model->getFullPath(index);

        ftp_model->uploadFile(file, path);

        emit onActionPerformed();
    }
}

void FTPClient::onDownloadFileAction()
{
    qDebug() << "onDownloadFileAction";
    QModelIndex index = user_tree->currentIndex();

    if (index.isValid() && ftp_model->isFile(index)) {
        qDebug() << ftp_model->getFullPath(index);

        QString outfile = QFileDialog::getSaveFileName(this, "Download as...", ftp_model->data(index).toString());
        if (outfile.isEmpty()) { return ; }
        QString fpath = ftp_model->getFullPath(index);

        ftp_model->downloadFile(outfile, fpath);

        emit onActionPerformed();
    }
}

void FTPClient::onDeleteFileAction()
{
    qDebug() << "onDeleteFileAction";
    QModelIndex index = user_tree->currentIndex();

    if (index.isValid() && ftp_model->isFile(index)) {
        qDebug() << ftp_model->getFullPath(index);

        ftp_model->deleteFile(ftp_model->getFullPath(index));

        emit onActionPerformed();
    }
}

void FTPClient::onCreateFolderAction()
{
    qDebug() << "onCreateFolderAction";
    QModelIndex index = user_tree->currentIndex();

    if (index.isValid() && ftp_model->isFolder(index)) {
        qDebug() << ftp_model->getFullPath(index);

        QString name = QInputDialog::getText(this, "Create Directory", "Name:");
        if (name.isEmpty()) { return; };
        QString path = ftp_model->getFullPath(index);

        ftp_model->createFolder(name, path);

        emit onActionPerformed();
    }
}

void FTPClient::onDeleteFolderAction()
{
    qDebug() << "onDeleteFolderAction";
    QModelIndex index = user_tree->currentIndex();

    if (index.isValid() && ftp_model->isFolder(index)) {
        qDebug() << ftp_model->getFullPath(index);

        ftp_model->deleteFolder(ftp_model->getFullPath(index));

        emit onActionPerformed();
    }
}

void FTPClient::onRenameFolderAction()
{
    qDebug() << "onRenameFolderAction";
    QModelIndex index = user_tree->currentIndex();

    if (index.isValid() && ftp_model->isFolder(index)) {
        qDebug() << ftp_model->getFullPath(index);

        QString name = QInputDialog::getText(this, "Rename Directory", "Rename To:");
        if (name.isEmpty()) { return; };
        QString path = ftp_model->getFullPath(index);

        ftp_model->renameFolder(name, path);

        emit onActionPerformed();
    }
}


FTPClient::~FTPClient()
{
    delete user_tree;
    delete ftp_model;
}
