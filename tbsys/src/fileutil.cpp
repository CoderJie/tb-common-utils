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

#include "fileutil.h"

namespace tbsys {

    bool CFileUtil::mkdirs(char *szDirPath) 
    {
        struct stat stats;
        /**
         * 如果目录存在，则直接返回
         **/
        if (lstat (szDirPath, &stats) == 0 && S_ISDIR (stats.st_mode)) 
            return true;
    
        /* 得到系统原来的umask值 */
        mode_t umask_value = umask (0);
        /* 还原系统umask值 */ 
        umask (umask_value);
        /* 得到用户的umask值 */
        mode_t mode = (S_IRWXUGO & (~ umask_value)) | S_IWUSR | S_IXUSR;
        
        char *slash = szDirPath;
        /**
         * 去掉开头重复的'/'
         **/
    	while (*slash == '/')
    	    slash++;
        
        /**
         * 比如要创建aaa/bbb/ccc，下面的代码就是模拟了如下步骤
         * mkdir aaa
         * mkdir aaa/bbb
         * mkdir aaa/bbb/ccc
         **/
        while (1)
    	{
    	    slash = strchr (slash, '/');
    	    if (slash == NULL)
    	        break;
    	    
            /**
             * 实现逐级创建目录
             **/
            *slash = '\0';
            int ret = mkdir(szDirPath, mode);
            *slash++ = '/';
            if (ret && errno != EEXIST) {
                return false;
            }
            
            /**
             * 去掉下一级目录开头重复的'/'
             **/
    	    while (*slash == '/')
    	        slash++;
        }
        
        /**
         * 创建最后一级目录
         **/
        if (mkdir(szDirPath, mode)) {
            return false;
        }
        return true;
    }


    bool CFileUtil::isDirectory(const char *szDirPath)
    {
        struct stat stats;
        if (lstat (szDirPath, &stats) == 0 && S_ISDIR (stats.st_mode))
            return true;
        return false;
    }


    bool CFileUtil::isSymLink(const char *szDirPath)
    {
        struct stat stats;
        if (lstat (szDirPath, &stats) == 0 && S_ISLNK (stats.st_mode)) 
            return true;
        return false; 
    }
}

//////////////////END
