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


void LpfFTPModel::uploadFile(const QString &file, const QString &targetdir)
{
    fs::path targetfile (targetdir.toStdString());
    targetfile /= fs::path(file.toStdString()).filename();

    // qDebug() << "Target File: " << targetfile.string();
    // qDebug() << "File: " << file.toStdString();

    LPTF_Socket *socket = nullptr;

    try {
        socket = connect_to_server();
        if (!socket) { return; }

        if (upload_file(socket, targetfile.string(), file.toStdString())) {
            // FIXME update tree localy without querying data
        } else {
            emit onCommandFail("upload_file");
        }
    } catch (const std::runtime_error &ex) {
        if (socket)
            delete socket;
        emit onConnectionError(ex.what());
    }
}

void LpfFTPModel::downloadFile(const QString &outfile, const QString &filepath)
{
    LPTF_Socket *socket = nullptr;

    try {
        socket = connect_to_server();
        if (!socket) { return; }

        if (!download_file(socket, outfile.toStdString(), filepath.toStdString())) {
            emit onCommandFail("download_file");
        }
    } catch (const std::runtime_error &ex) {
        if (socket)
            delete socket;
        emit onConnectionError(ex.what());
    }
}

void LpfFTPModel::deleteFile(const QString &filepath)
{
    LPTF_Socket *socket = nullptr;

    try {
        socket = connect_to_server();
        if (!socket) { return; }

        if (delete_file(socket, filepath.toStdString())) {
            // FIXME update tree localy without querying data
        } else {
            emit onCommandFail("filepath");
        }
    } catch (const std::runtime_error &ex) {
        if (socket)
            delete socket;
        emit onConnectionError(ex.what());
    }
}

void LpfFTPModel::createFolder(const QString &name, const QString &path)
{
    LPTF_Socket *socket = nullptr;

    try {
        socket = connect_to_server();
        if (!socket) { return; }

        QString filepath = path;
        if(!filepath.isEmpty())
            filepath.append("/");
        filepath.append(name);

        if (create_directory(socket, filepath.toStdString())) {
            // FIXME update tree localy without querying data
        } else {
            emit onCommandFail("create_directory");
        }
    } catch (const std::runtime_error &ex) {
        if (socket)
            delete socket;
        emit onConnectionError(ex.what());
    }
}

void LpfFTPModel::deleteFolder(const QString &path)
{
    LPTF_Socket *socket = nullptr;

    try {
        socket = connect_to_server();
        if (!socket) { return; }

        if (remove_directory(socket, path.toStdString())) {
            // FIXME update tree localy without querying data
        } else {
            emit onCommandFail("remove_directory");
        }
    } catch (const std::runtime_error &ex) {
        if (socket)
            delete socket;
        emit onConnectionError(ex.what());
    }
}

void LpfFTPModel::renameFolder(const QString &name, const QString &path)
{
    LPTF_Socket *socket = nullptr;

    try {
        socket = connect_to_server();
        if (!socket) { return; }

        if (rename_directory(socket, name.toStdString(), path.toStdString())) {
            // FIXME update tree localy without querying data
        } else {
            emit onCommandFail("rename_directory");
        }
    } catch (const std::runtime_error &ex) {
        if (socket)
            delete socket;
        emit onConnectionError(ex.what());
    }
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

    // QStandardItem *folder = new QStandardItem("MyFolder/");
    // QStandardItem *folder2 = new QStandardItem("My Folder 2/");
    // QStandardItem *folder3 = new QStandardItem("empty/");
    // QStandardItem *file = new QStandardItem("hello.txt");

    // QStandardItem *subfolder = new QStandardItem("sub/");
    // QStandardItem *subfolder2 = new QStandardItem("sub2/");
    // QStandardItem *subfile = new QStandardItem("hello2.txt");

    // folder->appendRows({subfolder, subfile});
    // folder2->appendRows({subfolder2});

    this->invisibleRootItem()->appendRows({root});

    // root->appendRows({folder, folder2, folder3, file});
}
