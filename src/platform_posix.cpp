//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#ifndef _WIN32

#include <noz/noz.h>
#include <noz/platform.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

void thread_sleep_ms(int milliseconds)
{
    usleep(milliseconds * 1000);
}

bool file_stat(const path_t* file_path, file_stat_t* out_stat)
{
    struct stat st;
    if (stat(file_path->value, &st) != 0)
        return false;
    
    out_stat->size = st.st_size;
    out_stat->modified_time = st.st_mtime;
    out_stat->is_directory = S_ISDIR(st.st_mode);
    out_stat->is_regular_file = S_ISREG(st.st_mode);
    
    return true;
}

bool directory_create(const path_t* dir_path)
{
    // Try to create the directory
    if (mkdir(dir_path->value, 0755) == 0)
        return true;
    
    // If it failed because directory already exists, that's success
    if (errno == EEXIST)
    {
        // Verify it's actually a directory
        return directory_exists(dir_path);
    }
    
    return false;
}

bool directory_create_recursive(const path_t* dir_path)
{
    path_t parent;
    path_dir(dir_path, &parent);
    
    // If parent is not the same as current (not root), create parent first
    if (!path_eq(&parent, dir_path))
    {
        if (!directory_exists(&parent))
        {
            if (!directory_create_recursive(&parent))
                return false;
        }
    }
    
    return directory_create(dir_path);
}

bool directory_exists(const path_t* dir_path)
{
    file_stat_t stat;
    return file_stat(dir_path, &stat) && stat.is_directory;
}

bool directory_enum_files(const path_t* dir_path, directory_enum_files_callback_t callback, void* user_data)
{
    DIR* dir = opendir(dir_path->value);
    if (!dir)
        return false;
    
    struct dirent* entry;
    path_t full_path;
    
    while ((entry = readdir(dir)) != NULL)
    {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        
        // Create full path
        path_copy(&full_path, dir_path);
        path_append(&full_path, entry->d_name);
        
        // Get file stats
        file_stat_t stat;
        if (file_stat(&full_path, &stat))
        {
            // Call the callback
            callback(&full_path, &stat, user_data);
            
            // If it's a directory, recurse
            if (stat.is_directory)
            {
                directory_enum_files(&full_path, callback, user_data);
            }
        }
    }
    
    closedir(dir);
    return true;
}

bool path_current_directory(path_t* dst)
{
    if (!dst)
        return false;
    
    char buffer[1024];
    if (getcwd(buffer, sizeof(buffer)) == NULL)
        return false;
    
    path_set(dst, buffer);
    return true;
}

#endif