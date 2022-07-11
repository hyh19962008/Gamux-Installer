//
// Created by maicss on 22-6-24.
//
#include <QMessageBox>
#include <QFileInfo>
#include <QTextStream>
#include <QFileDialog>
#include <QCryptographicHash>

#include "utils.h"

#include <cstdlib>
#include <sys/stat.h>

QString getDirPath(const QString& filename) {
    return filename.left(filename.lastIndexOf('/'));
}

QString HomeDir() {
    return getenv("HOME");
}

void MessageBoxExec(const QString& title, const QString& content, QMessageBox::Icon icon) {
    QMessageBox messageBox;
    messageBox.setWindowTitle(title);
    messageBox.setText(content);
    messageBox.setIcon(icon);
    messageBox.exec();
}
bool mkdirP(QString path) {
    QFileInfo info(path);
    if (info.exists()){
        return true;
    }
    QVector<QString> paths;
    do {
        paths.push_front(path);
        path = getDirPath(path);
        info.setFile(path);
    } while (!info.exists());

    for (auto& d : paths) {
        if(mkdir(d.toStdString().c_str(),0755)!=0) return false;
    }
    return true;
}

QString getMd5(const QString& path) {
    QFile file(path);
    file.open(QFile::ReadOnly);
    QByteArray ba = QCryptographicHash::hash(file.readAll(), QCryptographicHash::Md5);
    file.close();
    QString md5 = ba.toHex().constData();
    return md5;
}

QString chooseDirectory() {
    return QFileDialog::getExistingDirectory();
}

QString chooseFile(const QString& filter) {
    return QFileDialog::getOpenFileName(nullptr,"",HomeDir(),filter);
}
void lsDir(QString path, QVector<Entry> *list, QString front, Category category) {
    QDir dir(path);
    for(auto entry : dir.entryList()){
        if (entry == "." || entry == "..") continue;
        QFileInfo info(path + "/" + entry);
        if (info.isDir()){
            list->append(Entry{front+entry,DIR,"",category});
            lsDir(path+"/"+entry,list,front+entry+"/",category);
            continue;
        }
        if (info.isSymLink()){
            list->append(Entry{front+entry,SYMLINK,info.symLinkTarget(),category});
            continue;
        }
        list->append(Entry{front+entry,NORMAL_FILE,"",category});
    }
}
QString getValidParentPath(QString path) {
    QFileInfo info(path);
    if (info.exists()){
        return path;
    }
    do {
        path = getDirPath(path);
        info.setFile(path);
        if (path.isEmpty()){
            return "/";
        }
    } while (!info.exists());

    return path;
}

int getFileLineCount(const QString &path) {
    QFile file(path);
    if(!file.open(QFile::ReadOnly)){
        return 0;
    }
    int line = 0;
    while (!file.atEnd()) {
        file.readLine();
        line++;
    }
    file.close();
    return line-1;
}
