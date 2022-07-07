//
// Created by maicss on 22-7-6.
//

#include "TarFile.h"

#include <cstring>
#include <fstream>
#include <iostream>

#include <QDir>
#include <QDebug>

#include "utils.h"
#include <unistd.h>
#include <sys/stat.h>
using namespace std;

TarFile::TarFile() : file(nullptr)
{
}
bool TarFile::open(const char *tar_name, unsigned long tarSize) {
    tar_size=tarSize;
    file = fopen(tar_name, "rb");

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    this->offset = size - tar_size ;
}

TarFile::~TarFile()
{
    if (file) {
        fclose(file);
        file = nullptr;
    }
}

size_t TarFile::getNodeCount(const QString& filterPath) {
    if (!file) return 0;
    const int block_size{ 512 };
    unsigned char buf[block_size];
    auto* header = (tar_posix_header*)buf;
    memset(buf, 0, block_size);

    if (tar_size % block_size != 0) {
        fprintf(stderr, "tar file size should be a multiple of 512 bytes: %zu\n", offset);
        return 0;
    }

    size_t pos{ unsigned (offset) };
    size_t res = 0;
    fseek(file, offset, SEEK_SET);
    while (true) {
        size_t read_size = fread(buf, block_size, 1, file);
        if (read_size != 1) break;
        if (strncmp(header->magic, TMAGIC, 5) != 0) break;
        pos += block_size;
        size_t file_size{0};
        sscanf(header->size, "%lo", &file_size);
        size_t file_block_count = (file_size + block_size - 1) / block_size;
        pos += file_block_count * block_size;
        if (QString(header->name).left(filterPath.length()) == filterPath){
            res++;
        }
        fseek(file, pos, SEEK_SET);
    }
    fseek(file, offset, SEEK_SET);
    return res;
}

bool TarFile::unpack(const QString& target, const QString& filterPath) {
    if (!file) return false;
    mkdirP(target);
    const int block_size{ 512 };
    unsigned char buf[block_size];
    auto* header = (tar_posix_header*)buf;
    memset(buf, 0, block_size);

    size_t pos{ unsigned (offset) };
    size_t now = 0;
    fseek(file, offset, SEEK_SET);
    while (true) {
        now++;
        emit progressReady(now,header->name);
        size_t read_size = fread(buf, block_size, 1, file);
        if (read_size != 1) break;
        if (strncmp(header->magic, TMAGIC, 5) != 0) break;
        pos += block_size;
        size_t file_size{0};
        sscanf(header->size, "%lo", &file_size);
        size_t file_block_count = (file_size + block_size - 1) / block_size;
        mode_t mode;
        sscanf(header->mode, "%o", &mode);
        auto targetFilename = target +"/"+ QString(header->name).right(QString(header->name).length()-filterPath.length());
        qDebug()<<targetFilename;
        FILE *outFile;
        char* content;
        switch (header->typeflag) {
            case REGTYPE: // intentionally dropping through
            case AREGTYPE:
                // normal file
                if (QString(header->name).left(filterPath.length()) == filterPath){
                    outFile = fopen(targetFilename.toStdString().c_str() , "w+");
                    content = new char[file_size];
                    fread(content, file_size,1,file);
                    for (int i = 0; i < file_size; ++i) {
                        fputc(content[i],outFile);
                    }
                    delete content;
                    fflush(outFile);
                    fclose(outFile);
                    chmod(targetFilename.toStdString().c_str(), mode);
                }
                break;
            case LNKTYPE:
                // hard link
                break;
            case SYMTYPE:
                // symbolic link
                symlink(header->linkname,targetFilename.toStdString().c_str());
                break;
            case CHRTYPE:
                // device file/special file
                break;
            case BLKTYPE:
                // block device
                break;
            case DIRTYPE:
                mkdirP(targetFilename);
                // directory
                break;
            case FIFOTYPE:
                // named pipe
                break;
            case CONTTYPE:
                break;
            default:
                break;
        }
        pos += file_block_count * block_size;
        fseek(file, pos, SEEK_SET);
    }
    fseek(file, offset, SEEK_SET);

    return true;
}

QString TarFile::readTextFile(const QString& filename) {
    if (!file) return "";
    const int block_size{ 512 };
    unsigned char buf[block_size];
    auto* header = (tar_posix_header*)buf;
    memset(buf, 0, block_size);

    size_t pos{ unsigned (offset) };
    fseek(file, offset, SEEK_SET);
    while (true) {
        size_t read_size = fread(buf, block_size, 1, file);
        if (read_size != 1) break;
        if (strncmp(header->magic, TMAGIC, 5) != 0) break;
        pos += block_size;
        size_t file_size{0};
        sscanf(header->size, "%lo", &file_size);
        size_t file_block_count = (file_size + block_size - 1) / block_size;
        char* content;
        switch (header->typeflag) {
            case REGTYPE: // intentionally dropping through
            case AREGTYPE:
                if (filename==header->name){
                    content = new char[file_size];
                    fread(content, file_size,1,file);
                    fseek(file, offset, SEEK_SET);
                    return {content};
                }
            default:
                break;
        }
        pos += file_block_count * block_size;
        fseek(file, pos, SEEK_SET);
    }
    fseek(file, offset, SEEK_SET);

    return "";
}