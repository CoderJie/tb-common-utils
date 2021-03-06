/*
 * (C) 2007-2010 Taobao Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Version: $Id$
 *
 * Authors:
 *   duolong <duolong@taobao.com>
 *
 */

#include "tblog.h"
#include <string.h>

namespace tbsys 
{

const char * const CLogger::_errstr[] = {"ERROR","WARN","INFO","DEBUG"};
CLogger CLogger::_logger;

CLogger::CLogger() {
    _fd = fileno(stderr);
    _level = 9;
    _name = NULL;
    _check = 0;
    _maxFileSize = 0;
    _maxFileIndex = 0;
    pthread_mutex_init(&_fileSizeMutex, NULL );
    pthread_mutex_init(&_fileIndexMutex, NULL );
    _flag = false;
}

CLogger::~CLogger() {
    if (_name != NULL) {
        free(_name);
        _name = NULL;
        close(_fd);
    }
    pthread_mutex_destroy(&_fileSizeMutex);
    pthread_mutex_destroy(&_fileIndexMutex);
}

void CLogger::setLogLevel(const char *level) {
    if (level == NULL) return;
    int l = sizeof(_errstr)/sizeof(char*);
    for (int i=0; i<l; i++) {
        if (strcasecmp(level, _errstr[i]) == 0) {
            _level = i;
            break;
        }
    }
}

void CLogger::setFileName(const char *filename, bool flag) {
    if (_name) {
        if (_fd!=-1) close(_fd);
        free(_name);
        _name = NULL;
    }
    _name = strdup(filename);
    int fd = open(_name, O_RDWR | O_CREAT | O_APPEND | O_LARGEFILE, 0640);
    _flag = flag;
    if (!_flag)
    {
      /**
       * 写入标准输出和错误的信息会写入到log文件中，下同
       **/
      dup2(fd, _fd);
      dup2(fd, 1);
      if (_fd != 2) dup2(fd, 2);
      close(fd);
    }
    else
    {
      if (_fd != 2)
      { 
        close(_fd);
      }
      _fd = fd;
    }
}

void CLogger::logMessage(int level,const char *file, int line, const char *function,const char *fmt, ...) {
    if (level>_level) return;
    
    if (_check && _name) {
        checkFile();
    }
    
    time_t t;
    time(&t);
    struct tm tm;
    ::localtime_r((const time_t*)&t, &tm);
    
    char data1[4000];
    char buffer[5000];

    va_list args;
    va_start(args, fmt);
    vsnprintf(data1, 4000, fmt, args);
    va_end(args);
    
    int size;
    /**
     * #define TBSYS_LOG_LEVEL_ERROR 0
     * #define TBSYS_LOG_LEVEL_WARN  1
     * #define TBSYS_LOG_LEVEL_INFO  2
     * #define TBSYS_LOG_LEVEL_DEBUG 3
     **/
    if (level < TBSYS_LOG_LEVEL_INFO) {
        size = snprintf(buffer,5000,"[%04d-%02d-%02d %02d:%02d:%02d] %-5s %s (%s:%d) %s\n",
            tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec,
            _errstr[level], function, file, line, data1);
    } else {
        size = snprintf(buffer,5000,"[%04d-%02d-%02d %02d:%02d:%02d] %-5s %s:%d %s\n",
            tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec,
            _errstr[level], file, line, data1);
    }
    
    /**
     * 去掉无用的换行符
     **/
    while (buffer[size-2] == '\n') size --;
    
    buffer[size] = '\0';
    
    while (size > 0) {
        ssize_t success = ::write(_fd, buffer, size);
        if (success == -1) break;    
        size -= success;
    }

    /**
     * 超过单个文件大小则重新搞
     **/
    if ( _maxFileSize ){
        pthread_mutex_lock(&_fileSizeMutex);
        off_t offset = ::lseek(_fd, 0, SEEK_END);
        if ( offset < 0 ){
            // we got an error , ignore for now
        } else {
            if ( static_cast<size_t>(offset) >= _maxFileSize ) {
                rotateLog(NULL);
            }
        }
        pthread_mutex_unlock(&_fileSizeMutex);
    }
}

void CLogger::rotateLog(const char *filename, const char *fmt) {
    if (filename == NULL && _name != NULL) {
        filename = _name;
    }
    
    if (access(filename, R_OK) == 0) {
        char oldLogFile[256];
        time_t t;
        time(&t);
        struct tm tm;
        localtime_r((const time_t*)&t, &tm);
        
        if (fmt != NULL) {
            char tmptime[256];
            strftime(tmptime, sizeof(tmptime), fmt, &tm);
            sprintf(oldLogFile, "%s.%s", filename, tmptime);
        } else {
            sprintf(oldLogFile, "%s.%04d%02d%02d%02d%02d%02d",
                filename, tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
                tm.tm_hour, tm.tm_min, tm.tm_sec);
        }
        
        if ( _maxFileIndex > 0 ) {
            pthread_mutex_lock(&_fileIndexMutex);
            /**
             * 大于文件个数则删除比较旧的，_fileList是一个deque
             **/
            if ( _fileList.size() >= _maxFileIndex ) {
                std::string oldFile = _fileList.front();
                _fileList.pop_front();
                
                unlink(oldFile.c_str());
            }
            /**
             * 当前文件放到deque队尾
             **/
            _fileList.push_back(oldLogFile);
            pthread_mutex_unlock(&_fileIndexMutex);
        }
        
        /**
         * 重命名原来文件
         **/
        rename(filename, oldLogFile);
    }
    
    /**
     * 重新搞
     **/
    int fd = open(filename, O_RDWR | O_CREAT | O_APPEND | O_LARGEFILE, 0640);
    if (!_flag)
    {
      dup2(fd, _fd);
      dup2(fd, 1);
      if (_fd != 2) dup2(fd, 2);
      close(fd);
    }
    else
    {
      if (_fd != 2)
      { 
        close(_fd);
      }
      _fd = fd;
    }
}

void CLogger::checkFile()
{
    struct stat stFile;
    struct stat stFd;

    fstat(_fd, &stFd);
    int err = stat(_name, &stFile);
    
    /**
     * ENOENT: 不存在该文件或目录（No such file or directory）
     *
     * st_dev: This field describes the device on which this file resides.
     *         (The major(3) and minor(3) macros may be useful to decompose
     *         the device ID in this field.)
     * st_ino: This field contains the file's inode number.
     * 确保打开文件的fd和对应name的文件是同一个
     **/
    if ((err == -1 && errno == ENOENT)
        || (err == 0 && (stFile.st_dev != stFd.st_dev || stFile.st_ino != stFd.st_ino))) {
        int fd = open(_name, O_RDWR | O_CREAT | O_APPEND | O_LARGEFILE, 0640);
        if (!_flag)
        {
          dup2(fd, _fd);
          dup2(fd, 1);
          if (_fd != 2) dup2(fd, 2);
          close(fd);
        }
        else
        {
          if (_fd != 2)
          { 
            close(_fd);
          }
          _fd = fd;
        }
    }
}

void CLogger::setMaxFileSize( int64_t maxFileSize)
{
    if ( maxFileSize < 0x0 || maxFileSize > 0x40000000){
        maxFileSize = 0x40000000;//1GB 
    }
    _maxFileSize = maxFileSize;
}

void CLogger::setMaxFileIndex( int maxFileIndex )
{
    if ( maxFileIndex < 0x00 ) {
        maxFileIndex = 0x0F;
    }
    if ( maxFileIndex > 0x400 ) {
        maxFileIndex = 0x400;//1024
    }
    _maxFileIndex = maxFileIndex;
}
}

