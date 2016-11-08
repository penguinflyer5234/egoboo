//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

#include "egolib/FileSystem/OSX/FileSystem.hpp"

#import <Foundation/Foundation.h>

#ifdef ID_OSX
#import <AppKit/AppKit.h>
#else
#import <UIKit/UIKit.h>
#endif

namespace Ego {
namespace OSX {

FileSystem::FileSystem(const std::string& argument0, const std::string& rootPath) {
    @autoreleasepool {
        // rootPath is only usable on OS X; iOS only allows us to access our own data files 
#ifdef ID_OSX
        if (!rootPath.empty() && directoryExists(rootPath + "/basicdat")) {
            m_dataDirectoryPath = rootPath;
        } else {
#else
        {
#endif
            NSBundle *rootBundle = nil;
#ifdef ID_OSX
            if (!rootPath.empty()) {
                rootBundle = [NSBundle bundleWithPath:[NSString stringWithUTF8String:rootPath.c_str()]];
                if (rootBundle != nil && [rootBundle bundleIdentifier] == nil)
                    rootBundle = nil;
                if (rootBundle == nil)
                    NSLog(@"FileSystem warning: rootPath given but it's not a bundle! ('%s')", rootPath.c_str());
            }
#endif
            
            if (rootBundle == nil)
                rootBundle = [NSBundle mainBundle];
            
            if (rootBundle == nil)
                throw Id::RuntimeErrorException(__FILE__, __LINE__, "Cannot find a valid data path!");
            
            m_dataDirectoryPath = [[rootBundle resourcePath] UTF8String];
            if (!directoryExists(m_dataDirectoryPath + "/basicdat"))
                throw Id::RuntimeErrorException(__FILE__, __LINE__, "Cannot find a valid data path!");
        }
        
        auto fileManager = [NSFileManager defaultManager];
        NSError *error = nil;
        NSURL *userURL = [fileManager URLForDirectory:NSApplicationSupportDirectory inDomain:NSUserDomainMask
                                    appropriateForURL:nil create:YES error:&error];
        
        if (!userURL)
            throw Id::EnvironmentErrorException(__FILE__, __LINE__, "OSX FileSystem",
                                                error ? [[error localizedDescription] UTF8String] : "(unknown)");
        
        NSString *executableName = [[[NSBundle mainBundle] infoDictionary] valueForKey: (__bridge NSString *)kCFBundleExecutableKey];
        if (!executableName)
            executableName = @"Egoboo";
        
        userURL = [userURL URLByAppendingPathComponent:executableName isDirectory:YES];
        
        if (![fileManager createDirectoryAtURL:userURL withIntermediateDirectories:YES attributes:nil error:&error])
            throw Id::EnvironmentErrorException(__FILE__, __LINE__, "OSX FileSystem", [[error localizedDescription] UTF8String]);
        
        m_userDirectoryPath = [[userURL path] UTF8String];
        
        m_configurationDirectoryPath = m_dataDirectoryPath;
        
        NSLog(@"Data directory is %s", m_dataDirectoryPath.c_str());
        NSLog(@"User directory is %s", m_userDirectoryPath.c_str());
        NSLog(@"Config directory is %s", m_configurationDirectoryPath.c_str());
    }
}
    
bool FileSystem::exists(const std::string &path) {
    if (path.empty())
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "path is empty");
    
    @autoreleasepool {
        auto fileManager = [NSFileManager defaultManager];
        auto pathString = [NSString stringWithUTF8String:path.c_str()];
        
        return [fileManager fileExistsAtPath:pathString isDirectory:nil];
    }
}
    
bool FileSystem::directoryExists(const std::string &path) {
    if (path.empty())
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "path is empty");
    
    @autoreleasepool {
        auto fileManager = [NSFileManager defaultManager];
        auto pathString = [NSString stringWithUTF8String:path.c_str()];
        BOOL isDirectory = NO;
        
        return [fileManager fileExistsAtPath:pathString isDirectory:&isDirectory] && isDirectory;
    }
}
    
bool FileSystem::fileExists(const std::string &path) {
    if (path.empty())
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "path is empty");
    
    @autoreleasepool {
        auto fileManager = [NSFileManager defaultManager];
        auto pathString = [NSString stringWithUTF8String:path.c_str()];
        BOOL isDirectory = NO;
        
        return [fileManager fileExistsAtPath:pathString isDirectory:&isDirectory] && !isDirectory;
    }
}
    
bool FileSystem::createDirectory(const std::string &path) {
    if (path.empty())
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "path is empty");
    
    @autoreleasepool {
        auto fileManager = [NSFileManager defaultManager];
        auto pathString = [NSString stringWithUTF8String:path.c_str()];
        BOOL isDirectory = NO;
        
        // if the directory already exists, this returns NO.
        [fileManager createDirectoryAtPath:pathString withIntermediateDirectories:NO attributes:nil error:nil];
        
        return [fileManager fileExistsAtPath:pathString isDirectory:&isDirectory] && isDirectory;
    }
}

bool FileSystem::removeDirectory(const std::string &path) {
    if (path.empty())
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "path is empty");
    
    @autoreleasepool {
        auto fileManager = [NSFileManager defaultManager];
        auto pathString = [NSString stringWithUTF8String:path.c_str()];
        BOOL isDirectory = NO;
        
        if ([fileManager fileExistsAtPath:pathString isDirectory:&isDirectory] && isDirectory) {
            auto contents = [fileManager contentsOfDirectoryAtPath:pathString error:nil];
            if ([contents count] > 0)
                return false;
            return [fileManager removeItemAtPath:pathString error:nil];
        }
        return true;
    }
}

bool FileSystem::removeFile(const std::string &path) {
    if (path.empty())
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "path is empty");
    
    @autoreleasepool {
        auto fileManager = [NSFileManager defaultManager];
        auto pathString = [NSString stringWithUTF8String:path.c_str()];
        BOOL isDirectory = NO;
        
        if ([fileManager fileExistsAtPath:pathString isDirectory:&isDirectory] && !isDirectory)
            return [fileManager removeItemAtPath:pathString error:nil];
        return true;
    }
}

bool FileSystem::copyFile(const std::string &sourcePath, const std::string &targetPath) {
    if (sourcePath.empty())
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "sourcePath is empty");
    if (targetPath.empty())
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "targetPath is empty");
    
    @autoreleasepool {
        auto fileManager = [NSFileManager defaultManager];
        auto sourcePathString = [NSString stringWithUTF8String:sourcePath.c_str()];
        auto targetPathString = [NSString stringWithUTF8String:targetPath.c_str()];
        
        return [fileManager copyItemAtPath:sourcePathString toPath:targetPathString error:nil];
    }
}

bool FileSystem::copyDirectory(const std::string &sourcePath, const std::string &targetPath) {
    if (sourcePath.empty())
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "sourcePath is empty");
    if (targetPath.empty())
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "targetPath is empty");
    
    @autoreleasepool {
        auto fileManager = [NSFileManager defaultManager];
        auto sourcePathString = [NSString stringWithUTF8String:sourcePath.c_str()];
        auto targetPathString = [NSString stringWithUTF8String:targetPath.c_str()];
        
        return [fileManager copyItemAtPath:sourcePathString toPath:targetPathString error:nil];
    }
}

bool FileSystem::removeDirectoryAndContents(const std::string &path, bool recursive) {
    // -[NSFileManager removeItemAtPath:error:] removes a directory recursively
    if (!recursive)
        return Ego::FileSystem::removeDirectoryAndContents(path, recursive);
    
    if (path.empty())
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "path is empty");
    
    @autoreleasepool {
        auto fileManager = [NSFileManager defaultManager];
        auto pathString = [NSString stringWithUTF8String:path.c_str()];
        
        BOOL isDirectory = NO;
        
        if ([fileManager fileExistsAtPath:pathString isDirectory:&isDirectory] && isDirectory)
            return [fileManager removeItemAtPath:pathString error:nil];
        return true;
    }
}

void FileSystem::findFiles(const std::string &path, const std::string &extension, const std::function<bool (const std::string &)> &onPathFound) {
    if (path.empty())
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "path is empty");
    
    @autoreleasepool {
        auto fileManager = [NSFileManager defaultManager];
        auto pathString = [NSString stringWithUTF8String:path.c_str()];
        auto extensionString = [NSString stringWithUTF8String:extension.c_str()];
        auto pathURL = [NSURL fileURLWithPath:pathString];
        auto options = NSDirectoryEnumerationSkipsSubdirectoryDescendants | NSDirectoryEnumerationSkipsHiddenFiles;
        
        auto pathStringWithSlash = pathString;
        if ([pathString characterAtIndex:[pathString length] - 1] != '/')
            pathStringWithSlash = [pathString stringByAppendingString:@"/"];
        
        auto enumerator = [fileManager enumeratorAtURL:pathURL includingPropertiesForKeys:nil options:options errorHandler:nil];
        
        for (NSURL *url in enumerator) {
            if (!extension.empty() && ![[url pathExtension] isEqualToString:extensionString])
                continue;
            NSString *newPath = [url path];
            newPath = [newPath stringByReplacingOccurrencesOfString:pathStringWithSlash withString:@""];
            bool canContinue = onPathFound([newPath UTF8String]);
            if (!canContinue)
                break;
        }
    }
}

bool FileSystem::openFileWithDefaultApplication(const std::string &path) {
    if (path.empty())
       throw Id::RuntimeErrorException(__FILE__, __LINE__, "path is empty");
#ifdef ID_OSX
    @autoreleasepool {
        auto workspace = [NSWorkspace sharedWorkspace];
        auto pathString = [NSString stringWithUTF8String:path.c_str()];
        return [workspace openFile:pathString];
    }
#else
    // We cannot open file urls on iOS, and there seems to be no default data URI handler either
    return false;
#endif
}
    
bool FileSystem::showDirectoryInFileBrowser(const std::string &path) {
    if (path.empty())
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "path is empty");
#ifdef ID_OSX
    @autoreleasepool {
        auto workspace = [NSWorkspace sharedWorkspace];
        auto pathString = [NSString stringWithUTF8String:path.c_str()];
        return [workspace selectFile:nil inFileViewerRootedAtPath:pathString];
    }
#else
    // iOS does not have a file browser
    return false;
#endif
}

bool FileSystem::openURLWithDefaultApplication(const std::string &url) {
    if (url.empty())
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "url is empty");
    @autoreleasepool {
        auto urlString = [NSString stringWithUTF8String:url.c_str()];
        auto urlURL = [NSURL URLWithString:urlString];
        
#ifdef ID_OSX
        auto workspace = [NSWorkspace sharedWorkspace];
        return [workspace openURL:urlURL];
#else
        if ([[urlURL scheme] isEqualToString:@"file"])
            throw Id::RuntimeErrorException(__FILE__, __LINE__, "url cannot use the file scheme");
        auto application = [UIApplication sharedApplication];
        return [application openURL:urlURL];
#endif
    }
}

} // namespace OSX
} // namespace Ego
