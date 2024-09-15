#include "lpfftpmodel.h"
#include <QVariant>
#include <QModelIndex>
#include <Qt>
#include <QString>
#include <QMetaType>
#include <QStandardItem>
#include <QTimer>

#include <iostream>
#include <string>
#include <stdexcept>

#include "FTP/include/LPTF_Net/LPTF_Packet.hpp"
#include "FTP/include/LPTF_Net/LPTF_Utils.hpp"
#include "FTP/include/client_actions.hpp"
#include <sstream>

#include <map>

#include <filesystem>

namespace fs = std::filesystem;

LpfFTPModel::LpfFTPModel(QObject *parent,
                         QString &ip,
                         int port,
                         QString &username,
                         QString &password)
    : QStandardItemModel{parent}
    , iconProvider{new QFileIconProvider}
    , ip{ip}
    , port{port}
    , username{username}
    , password{password}
{
    QStandardItem *root = new QStandardItem(QString("()").insert(1, username));
    this->invisibleRootItem()->appendRows({root});
}

LpfFTPModel::~LpfFTPModel()
{
    delete iconProvider;
}

LPTF_Socket *LpfFTPModel::connect_to_server()
{
    LPTF_Socket *socket = new LPTF_Socket();

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(ip.toStdString().c_str());
    serverAddr.sin_port = htons(port);

    try {
        socket->connect(reinterpret_cast<struct sockaddr *>(&serverAddr), sizeof(serverAddr));
    } catch(std::runtime_error &ex) {
        delete socket;
        emit onConnectionError("Unable to connect to the server !");
        return nullptr;
    }

    if (!login(socket)) {
        delete socket;
        emit onConnectionError("Unable to connect to the server with provided username and password !");
        return nullptr;
    }

    return socket;
}

bool LpfFTPModel::login(LPTF_Socket *clientSocket)
{
    LPTF_Packet pckt(LOGIN_PACKET, (void *)username.toStdString().c_str(), username.size());
    clientSocket->write(pckt);

    pckt = clientSocket->read();

    // Password Reply or Password Create Message
    if ((pckt.type() == REPLY_PACKET && get_refered_packet_type_from_reply_packet(pckt) == LOGIN_PACKET)
        || (pckt.type() == MESSAGE_PACKET)) {
        LPTF_Packet password_packet = LPTF_Packet(MESSAGE_PACKET, (void *)password.toStdString().c_str(), password.size());
        clientSocket->write(password_packet);

        LPTF_Packet reply = clientSocket->read();
        if (reply.type() == REPLY_PACKET && get_refered_packet_type_from_reply_packet(reply) == LOGIN_PACKET) {
            qDebug() << "Login successful.";
            return true;
        } else if (reply.type() == ERROR_PACKET) {
            qDebug() << "Unable to log in: " << get_error_content_from_error_packet(reply);
            return false;
        }
    } else {
        qDebug() << "Unexpected server packet ! Could not log in !";
    }

    return false;
}


QVariant LpfFTPModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DecorationRole && index.isValid()) {

        // if root item
        if (index.row() == 0 && index.column() == 0 && !index.parent().isValid()) {
            return iconProvider->icon(QFileIconProvider::Folder);
        }

        // get data to check if it is a folder or a file
        QString data = QStandardItemModel::data(index, Qt::DisplayRole).toString();

        if (data.endsWith("/")) {
            return iconProvider->icon(QFileIconProvider::Folder);
        } else {
            return iconProvider->icon(QFileIconProvider::File);
        }
    } else if (role == Qt::DisplayRole && index.isValid()) {
        QString data = QStandardItemModel::data(index, Qt::DisplayRole).toString();

        // hide the slash for folders
        if (data.endsWith("/")) {
            return QVariant(data.removeLast());
        } else {
            return QVariant(data);
        }
    }

    return QStandardItemModel::data(index, role);
}

bool LpfFTPModel::isRootFolder(const QModelIndex &index)
{
    return (index.row() == 0 && index.column() == 0 && !index.parent().isValid()) /* root item */;
}

bool LpfFTPModel::isFolder(const QModelIndex &index)
{
    return this->isRootFolder(index) || QStandardItemModel::data(index, Qt::DisplayRole).toString().endsWith("/");
}

bool LpfFTPModel::isFile(const QModelIndex &index)
{
    return !isFolder(index);
}

QString LpfFTPModel::getFullPath(const QModelIndex &index)
{
    QString path;

    QStandardItem *root = this->item(0, 0);
    QStandardItem *item = this->itemFromIndex(index);

    while (item != nullptr && item != root) {
        path.insert(0, item->text());
        item = item->parent();
    }

    // remove separator for folders
    if (path.endsWith("/"))
        path.removeLast();

    return path;
}


bool LpfFTPModel::uploadFile(const QString &file, const QModelIndex &targetdir_index)
{
    if (!targetdir_index.isValid() || !this->isFolder(targetdir_index)) {
        emit onCommandFail("Selected item is not a folder !");
        return false;
    }

    QString targetdir = this->getFullPath(targetdir_index);

    fs::path targetfile (targetdir.toStdString());
    targetfile /= fs::path(file.toStdString()).filename();

    // qDebug() << "Target File: " << targetfile.string();
    // qDebug() << "File: " << file.toStdString();

    LPTF_Socket *socket = nullptr;

    try {
        socket = connect_to_server();
        if (!socket) { return false; }

        if (upload_file(socket, targetfile.string(), file.toStdString())) {
            // update tree localy without querying data

            QString file_item_name = QString(targetfile.filename().string().c_str());

            QStandardItem *diritem = this->itemFromIndex(targetdir_index);

            QList<QStandardItem*> items = findItems(file_item_name, Qt::MatchFlags(Qt::MatchExactly|Qt::MatchRecursive));
            // check if file item already exists
            for (QStandardItem *it : items) {
                if (it->parent() == diritem) {
                    return true;
                }
            }

            // if not create it
            QStandardItem *newfileitem = new QStandardItem(file_item_name);
            diritem->appendRows({newfileitem});
            emit layoutChanged({targetdir_index});
            return true;

        } else {
            QString error = QString("Unable to upload file \"");
            error.append(file);
            error.append("\" at \"");
            error.append(targetfile.string().c_str());
            error.append("\" !");
            emit onCommandFail(error);
        }
    } catch (const std::runtime_error &ex) {
        if (socket)
            delete socket;
        emit onConnectionError(ex.what());
    }

    return false;
}

bool LpfFTPModel::downloadFile(const QString &outfile, const QString &filepath)
{
    LPTF_Socket *socket = nullptr;

    try {
        socket = connect_to_server();
        if (!socket) { return false; }

        if (!download_file(socket, outfile.toStdString(), filepath.toStdString())) {
            emit onCommandFail("Unable to download file !");
        } else {
            return true;
        }
    } catch (const std::runtime_error &ex) {
        if (socket)
            delete socket;
        emit onConnectionError(ex.what());
    }

    return false;
}

bool LpfFTPModel::deleteFile(const QModelIndex &file_index)
{
    if (!file_index.isValid() || !this->isFile(file_index)) {
        emit onCommandFail("Selected item is not a file !");
        return false;
    }

    QString filepath = this->getFullPath(file_index);

    LPTF_Socket *socket = nullptr;

    try {
        socket = connect_to_server();
        if (!socket) { return false; }

        if (delete_file(socket, filepath.toStdString())) {
            // update tree localy without querying data

            QStandardItem *parent_dir_item = this->itemFromIndex(file_index.parent());
            parent_dir_item->removeRow(file_index.row());
            emit layoutChanged({parent_dir_item->index()});
            return true;
        } else {
            emit onCommandFail("filepath");
        }
    } catch (const std::runtime_error &ex) {
        if (socket)
            delete socket;
        emit onConnectionError(ex.what());
    }

    return false;
}

bool LpfFTPModel::createFolder(const QString &name, const QModelIndex &parent_dir_index)
{
    if (!parent_dir_index.isValid() || !this->isFolder(parent_dir_index)) {
        emit onCommandFail("Selected item is not a folder !");
        return false;
    }

    LPTF_Socket *socket = nullptr;

    try {
        socket = connect_to_server();
        if (!socket) { return false; }

        QString filepath = this->getFullPath(parent_dir_index);
        if(!filepath.isEmpty())
            filepath.append("/");
        filepath.append(name);

        if (create_directory(socket, filepath.toStdString())) {
            // update tree localy without querying data

            QStandardItem *parent_dir_item = this->itemFromIndex(parent_dir_index);
            QStandardItem *newdiritem = new QStandardItem(name + "/");
            parent_dir_item->appendRows({newdiritem});
            emit layoutChanged({parent_dir_index});
            return true;
        } else {
            emit onCommandFail("create_directory");
        }
    } catch (const std::runtime_error &ex) {
        if (socket)
            delete socket;
        emit onConnectionError(ex.what());
    }

    return false;
}

bool LpfFTPModel::deleteFolder(const QModelIndex &dir_index)
{
    if (!dir_index.isValid() || !this->isFolder(dir_index)) {
        emit onCommandFail("Selected item is not a folder !");
        return false;
    }

    QString path = this->getFullPath(dir_index);

    LPTF_Socket *socket = nullptr;

    try {
        socket = connect_to_server();
        if (!socket) { return false; }

        if (remove_directory(socket, path.toStdString())) {
            // update tree localy without querying data

            if (!this->isRootFolder(dir_index)) {
                QStandardItem *dir_item = this->itemFromIndex(dir_index);
                QModelIndex parent_dir_index = dir_item->parent()->index();
                this->removeRow(dir_index.row(), parent_dir_index);
                emit layoutChanged({parent_dir_index});
                return true;
            } else {
                this->clear();
                QStandardItem *root = new QStandardItem(QString("()").insert(1, username));
                this->invisibleRootItem()->appendRows({root});
                emit layoutChanged({this->invisibleRootItem()->index()});
            }
        } else {
            emit onCommandFail("remove_directory");
        }
    } catch (const std::runtime_error &ex) {
        if (socket)
            delete socket;
        emit onConnectionError(ex.what());
    }

    return false;
}

bool LpfFTPModel::renameFolder(const QString &name, const QModelIndex &dir_index)
{
    if (!dir_index.isValid() || !this->isFolder(dir_index)) {
        emit onCommandFail("Selected item is not a folder !");
        return false;
    } else if (this->isRootFolder(dir_index)) {
        emit onCommandFail("Cannot rename the root folder !");
        return false;
    }

    QString path = this->getFullPath(dir_index);

    LPTF_Socket *socket = nullptr;

    try {
        socket = connect_to_server();
        if (!socket) { return false; }

        if (rename_directory(socket, name.toStdString(), path.toStdString())) {
            // update tree localy without querying data

            QStandardItem *dir_item = this->itemFromIndex(dir_index);
            dir_item->setText(name + "/");
            emit layoutChanged({dir_index});
            return true;
        } else {
            emit onCommandFail("rename_directory");
        }
    } catch (const std::runtime_error &ex) {
        if (socket)
            delete socket;
        emit onConnectionError(ex.what());
    }

    return false;
}



void LpfFTPModel::queryData()
{
    qDebug() << "Querying folder tree";

    ostringstream rawtree;

    LPTF_Socket *socket = nullptr;

    try {
        socket = connect_to_server();
        if (!socket) { return; }

        rawtree = list_tree(socket);
    } catch (const std::runtime_error &ex) {
        if (socket)
            delete socket;
        emit onConnectionError(ex.what());
        return;
    }

    this->clear();

    QStandardItem *root = new QStandardItem(QString("()").insert(1, username));

    istringstream treestream (rawtree.str());

    std::map<std::string, QStandardItem*> items;    // only for matching dirs
    items[""] = root;

    std::string line;
    while (std::getline(treestream, line))
    {
        fs::path itempath (line);

        bool path_leads_to_folder = line.at(line.size()-1) == '/';

        std::string parent_key = "";
        fs::path::iterator ppath = itempath.begin();
        fs::path::iterator pend = itempath.end();

        while (ppath != pend && !ppath->empty()) {
            fs::path curr_path (parent_key);
            if (parent_key.size() == 0)
                curr_path += *ppath;
            else
                curr_path /= *ppath;

            if (items.find(curr_path.string()) == items.end()) {
                bool is_file = false;
                if (!path_leads_to_folder) {
                    fs::path::iterator nppath = next(ppath);
                    is_file = nppath == pend;
                }

                std::string item_name;
                if (is_file) {
                    item_name = curr_path.filename().string();
                } else {
                    item_name = curr_path.stem().string() + "/";
                }

                QStandardItem *newitem = new QStandardItem(QString(item_name.c_str()));
                // append to parent item
                items[parent_key]->appendRows({newitem});

                // add dir to items
                if (!is_file)
                    items[curr_path.string()] = newitem;
            }

            if (parent_key.size() == 0)
                parent_key += ppath->string();
            else
                parent_key += "/" + ppath->string();

            advance(ppath, 1);
        }

    }

    this->invisibleRootItem()->appendRows({root});
}
