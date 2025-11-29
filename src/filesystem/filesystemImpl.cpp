//
//  filesystemImpl.cpp
//  Player
//
//  Created by ゾロアーク on 11/21/20.
//

#include <SDL_filesystem.h>

#include "filesystemImpl.h"
#include "util/exception.h"
#include "util/debugwriter.h"

#ifdef MKXPZ_EXP_FS
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include "ghc/filesystem.hpp"
namespace fs = ghc::filesystem;
#endif

#include <fstream>

#if defined(__WIN32__)
#include <windows.h>
#include <shobjidl.h>
#include <SDL_syswm.h>
#elif defined(__linux__)
#include <gtk/gtk.h>
#include <SDL_syswm.h>
#endif

// https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exist-using-standard-c-c11-c
bool filesystemImpl::fileExists(const char *path) {
    fs::path stdPath(path);
    return (fs::exists(stdPath) && !fs::is_directory(stdPath));
}


// https://stackoverflow.com/questions/2912520/read-file-contents-into-a-string-in-c
std::string filesystemImpl::contentsOfFileAsString(const char *path) {
    std::string ret;
    try {
        std::ifstream ifs(path);
        ret = std::string ( (std::istreambuf_iterator<char>(ifs) ),
                       (std::istreambuf_iterator<char>()    ) );
    } catch (...) {
        throw Exception(Exception::NoFileError, "Failed to read file at %s", path);
    }

    return ret;
}

// chdir and getcwd do not support unicode on Windows
bool filesystemImpl::setCurrentDirectory(const char *path) {
    fs::path stdPath(path);
    fs::current_path(stdPath);
    bool ret;

    try {
        ret = fs::equivalent(fs::current_path(), stdPath);
    } catch (...) {
        Debug() << "Failed to check current path." << path;
        ret = false;
    }
    return ret;
}

std::string filesystemImpl::getCurrentDirectory() {
    std::string ret;
    try {
        ret = std::string(fs::current_path().string());
    } catch (...) {
        throw Exception(Exception::MKXPError, "Failed to retrieve current path");
    }
    return ret;
}


std::string filesystemImpl::normalizePath(const char *path, bool preferred, bool absolute) {
    fs::path stdPath(path);
    
    if (!stdPath.is_absolute() && absolute)
        stdPath = fs::current_path() / stdPath;

    stdPath = stdPath.lexically_normal();
    std::string ret(stdPath);
    for (size_t i = 0; i < ret.length(); i++) {
        char sep;
        char sep_alt;
#ifdef __WIN32__
        if (preferred) {
            sep = '\\';
            sep_alt = '/';
        }
        else
#endif
        {
            sep = '/';
            sep_alt = '\\';
        }
        
        if (ret[i] == sep_alt)
            ret[i] = sep;
    }
    return ret;
}

std::string filesystemImpl::getDefaultGameRoot() {
    char *p = SDL_GetBasePath();
    std::string ret(p);
    SDL_free(p);
    return ret;
}

#if defined(__WIN32__) || defined(__linux__)
std::string filesystemImpl::selectPath(SDL_Window *win, const char *msg, const char *prompt) {
#if defined(__WIN32__)
    // Use IFileDialog for modern Windows folder selection
    std::string result;

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        return result;
    }

    IFileOpenDialog *pFileDialog = nullptr;
    hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
                          IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileDialog));

    if (SUCCEEDED(hr)) {
        // Set options for folder picking
        DWORD dwOptions;
        hr = pFileDialog->GetOptions(&dwOptions);
        if (SUCCEEDED(hr)) {
            hr = pFileDialog->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
        }

        // Set the title/prompt
        if (msg) {
            std::wstring wmsg(msg, msg + strlen(msg));
            pFileDialog->SetTitle(wmsg.c_str());
        }

        if (prompt) {
            std::wstring wprompt(prompt, prompt + strlen(prompt));
            pFileDialog->SetOkButtonLabel(wprompt.c_str());
        }

        // Get the native window handle
        HWND hwnd = nullptr;
        SDL_SysWMinfo windowinfo{};
        SDL_VERSION(&windowinfo.version);
        if (SDL_GetWindowWMInfo(win, &windowinfo)) {
            hwnd = windowinfo.info.win.window;
        }

        // Show the dialog
        hr = pFileDialog->Show(hwnd);

        if (SUCCEEDED(hr)) {
            IShellItem *pItem = nullptr;
            hr = pFileDialog->GetResult(&pItem);
            if (SUCCEEDED(hr)) {
                PWSTR pszFilePath = nullptr;
                hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                if (SUCCEEDED(hr)) {
                    // Convert wide string to UTF-8
                    int size_needed = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, NULL, 0, NULL, NULL);
                    if (size_needed > 0) {
                        std::string utf8str(size_needed - 1, 0);
                        WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, &utf8str[0], size_needed, NULL, NULL);
                        result = utf8str;
                    }
                    CoTaskMemFree(pszFilePath);
                }
                pItem->Release();
            }
        }
        pFileDialog->Release();
    }

    CoUninitialize();
    return result;

#elif defined(__linux__)
    // Use GTK file chooser for Linux
    std::string result;

    // Initialize GTK if not already done
    if (!gtk_init_check(nullptr, nullptr)) {
        return result;
    }

    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        msg ? msg : "Select Folder",
        nullptr,
        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
        "_Cancel", GTK_RESPONSE_CANCEL,
        prompt ? prompt : "_Select", GTK_RESPONSE_ACCEPT,
        nullptr
    );

    // Make the dialog modal relative to the SDL window if possible
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_keep_above(GTK_WINDOW(dialog), TRUE);

    gint response = gtk_dialog_run(GTK_DIALOG(dialog));

    if (response == GTK_RESPONSE_ACCEPT) {
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
        char *filename = gtk_file_chooser_get_filename(chooser);
        if (filename) {
            result = std::string(filename);
            g_free(filename);
        }
    }

    gtk_widget_destroy(dialog);

    // Process pending GTK events to ensure dialog is fully closed
    while (gtk_events_pending()) {
        gtk_main_iteration();
    }

    return result;
#endif
}
#endif
