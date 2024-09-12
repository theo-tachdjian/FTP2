#include "ftpclient.h"
#include <QTreeView>
#include <QVBoxLayout>
#include <QMenu>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QApplication>
#include <Qt>

FTPClient::FTPClient(QWidget *parent,
                     QString &ip,
                     int port,
                     QString &username,
                     QString &password)
    : QWidget(parent)
    , user_tree(new QTreeView(this))
    , ftp_model(new LpfFTPModel(this, ip, port, username, password))
{
    fileMenu = new QMenu(user_tree);
    folderMenu = new QMenu(user_tree);
    rootFolderMenu = new QMenu(user_tree);

    uploadFileAction = new QAction("Upload File...");
    connect(uploadFileAction, SIGNAL(triggered()), this, SLOT(onUploadFileAction()));

    downloadFileAction = new QAction("Download File...");
    connect(downloadFileAction, SIGNAL(triggered()), this, SLOT(onDownloadFileAction()));

    deleteFileAction = new QAction("Delete File");
    connect(deleteFileAction, SIGNAL(triggered()), this, SLOT(onDeleteFileAction()));

    createFolderAction = new QAction("Create Directory...");
    connect(createFolderAction, SIGNAL(triggered()), this, SLOT(onCreateFolderAction()));

    deleteFolderAction = new QAction("Delete Directory");
    connect(deleteFolderAction, SIGNAL(triggered()), this, SLOT(onDeleteFolderAction()));

    renameFolderAction = new QAction("Rename Directory...");
    connect(renameFolderAction, SIGNAL(triggered()), this, SLOT(onRenameFolderAction()));

    deleteRootAction = new QAction("Delete All Content...");
    connect(deleteRootAction, SIGNAL(triggered()), this, SLOT(onDeleteRootAction()));

    fileMenu->addActions({downloadFileAction, deleteFileAction});
    folderMenu->addActions({uploadFileAction, createFolderAction, renameFolderAction, deleteFolderAction});
    rootFolderMenu->addActions({uploadFileAction, createFolderAction, deleteRootAction});

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

    this->setMinimumSize(400, 300);

    ftp_model->queryData();
    user_tree->setExpanded(user_tree->model()->index(0, 0), true);
}

void FTPClient::onCommandFail(const QString &message)
{
    QMessageBox::warning(this, "Command Failed", message);
}

void FTPClient::onConnectionError(const QString &error)
{
    QApplication::restoreOverrideCursor();
    QMessageBox::critical(this, "Connection Error", error);

    // disable User Tree on error
    user_tree->setDisabled(true);
}

void FTPClient::queryUserTree()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    ftp_model->queryData();
    QApplication::restoreOverrideCursor();
    user_tree->setExpanded(user_tree->model()->index(0, 0), true);
}


void FTPClient::onCustomContextMenu(const QPoint &point)
{
    QModelIndex index = user_tree->indexAt(point);
    if (index.isValid()) {

        if (ftp_model->isRootFolder(index)) {
            qDebug() << "Opening Root Folder menu";
            rootFolderMenu->exec(user_tree->viewport()->mapToGlobal(point));
        } else if (ftp_model->isFolder(index)) {
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

    if (index.isValid() && ftp_model->isFolder(index) && !ftp_model->isRootFolder(index)) {
        qDebug() << ftp_model->getFullPath(index);

        QString name = QInputDialog::getText(this, "Rename Directory", "Rename To:");
        if (name.isEmpty()) { return; };
        QString path = ftp_model->getFullPath(index);

        ftp_model->renameFolder(name, path);

        emit onActionPerformed();
    }
}


void FTPClient::onDeleteRootAction()
{
    qDebug() << "onDeleteRootAction";
    QModelIndex index = user_tree->currentIndex();

    if (index.isValid() && ftp_model->isRootFolder(index)) {
        qDebug() << ftp_model->getFullPath(index);

        QMessageBox::StandardButton button = QMessageBox::question(this,
                                                                   QString("Delete All Content ?"),
                                                                   QString("Are you sure you want to delete everything in your User folder ?"));

        if (button != QMessageBox::Yes) { return; }

        ftp_model->deleteFolder(ftp_model->getFullPath(index));

        emit onActionPerformed();
    }
}


FTPClient::~FTPClient()
{
    delete user_tree;
    delete ftp_model;
}
