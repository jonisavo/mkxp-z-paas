//
//  config.cpp
//  Player
//
//  Created by ゾロアーク on 11/21/20.
//

#include "config.h"

#ifdef __APPLE__
#include "system.h"
#endif

#include <SDL_filesystem.h>
#include <assert.h>

#include <stdint.h>
#include <vector>

#include "filesystem/filesystem.h"
#include "util/exception.h"
#include "util/debugwriter.h"
#include "util/sdl-util.h"
#include "util/util.h"

#include "util/iniconfig.h"
#include "util/encoding.h"

std::string prefPath(const char *org, const char *app) {
    char *path = SDL_GetPrefPath(org, app);
    if (!path)
        return std::string("");
    std::string ret(path);
    SDL_free(path);
    return ret;
}

void fillStringVec(json &item, std::vector<std::string> &vector) {
    if (!item.is_array()) {
        if (item.is_string()) {
            vector.emplace_back(item.get<std::string>());
        }
        return;
    }

    for (const auto &element : item) {
        if (!element.is_string())
            continue;

        vector.emplace_back(element.get<std::string>());
    }
}

bool copyObject(json &dest, json &src, const char *objectName = "") {
    assert(dest.is_object());
    if (src.is_null())
        return false;
    
    if (!src.is_object())
        return false;
    
    for (auto& el : src.items()) {
        const auto& key = el.key();
        auto& value = el.value();

        // Specifically processs this object later.
        if (value.is_object() && dest[key].is_object())
            continue;
        
        if ((value.is_array() && dest[key].is_array())    ||
            (value.is_number() && dest[key].is_number())  ||
            (value.is_string() && dest[key].is_string())  ||
            (value.is_boolean() && dest[key].is_boolean()) ||
            (dest[key].is_null()))
        {
            dest[key] = value;
        }
        else {
            Debug() << "Invalid variable in configuration:" << objectName << key;
        }
    }
    return true;
}

bool getEnvironmentBool(const char *env, bool defaultValue) {
    const char *e = SDL_getenv(env);
    if (!e)
        return defaultValue;
    
    if (!strcmp(e, "0"))
        return false;
    else if (!strcmp(e, "1"))
        return true;
    
    return defaultValue;
}

json readConfFile(const char *path) {
    json ret(0);

    if (!mkxp_fs::fileExists(path)) {
        return json::object();
    }
    
    try {
        std::string cfg = mkxp_fs::contentsOfFileAsString(path);
        ret = json::parse(Encoding::convertString(cfg), nullptr, true, true, true);
    }
    catch (const std::exception &e) {
        Debug() << "Failed to parse" << path << ":" << e.what();
    }
    catch (const Exception &e) {
        Debug() << "Failed to parse" << path << ":" << "Unknown encoding";
    }
    
    if (!ret.is_object())
        ret = json::object({});
    
    return ret;
}

#define CONF_FILE "mkxp.json"

Config::Config() {}

void Config::read(const int argc, char *argv[]) {
    auto optsJ = json::object({
        {"rgssVersion", 0},
        {"debugMode", false},
        {"displayFPS", false},
        {"printFPS", false},
        {"winResizable", true},
        {"fullscreen", false},
        {"fixedAspectRatio", true},
        {"smoothScaling", 0},
        {"smoothScalingDown", 0},
        {"bitmapSmoothScaling", 0},
        {"bitmapSmoothScalingDown", 0},
        {"smoothScalingMipmaps", false},
        {"bicubicSharpness", 100},
#ifdef MKXPZ_SSL
        {"xbrzScalingFactor", 1.},
#endif
        {"enableHires", false},
        {"textureScalingFactor", 1.},
        {"framebufferScalingFactor", 1.},
        {"atlasScalingFactor", 1.},
        {"vsync", false},
        {"defScreenW", 0},
        {"defScreenH", 0},
        {"windowTitle", ""},
        {"fixedFramerate", 0},
        {"frameSkip", false},
        {"syncToRefreshrate", false},
        {"solidFonts", json::array({})},
#if defined(__APPLE__) && defined(__aarch64__)
        {"preferMetalRenderer", true},
#else
        {"preferMetalRenderer", false},
#endif
        {"subImageFix", false},
#ifdef __WIN32__
        {"enableBlitting", false},
#else
        {"enableBlitting", true},
#endif
        {"integerScalingActive", false},
        {"integerScalingLastMile", true},
        {"maxTextureSize", 0},
        {"gameFolder", ""},
        {"anyAltToggleFS", false},
        {"enableReset", true},
        {"enableSettings", true},
        {"allowSymlinks", true},
        {"dataPathOrg", ""},
        {"dataPathApp", ""},
        {"iconPath", ""},
        {"execName", "Game"},
        {"midiSoundFont", ""},
        {"midiChorus", false},
        {"midiReverb", false},
        {"SESourceCount", 6},
        {"BGMTrackCount", 1},
        {"customScript", ""},
        {"pathCache", true},
        {"useScriptNames", true},
        {"preloadScript", json::array({})},
        {"postloadScript", json::array({})},
        {"RTP", json::array({})},
        {"patches", json::array({})},
        {"fontSub", json::array({})},
        {"fontScale", 0.0f},
        {"fontKerning", true},
        {"fontHinting", 3}, // TTF_HINTING_NONE
        {"fontHeightReporting", 0},
        {"fontOutlineCrop", true},
        {"rubyLoadpath", json::array({})},
        {"JITEnable", false},
        {"JITVerboseLevel", 0},
        {"JITMaxCache", 100},
        {"JITMinCalls", 10000},
        {"YJITEnable", false},
        {"dumpAtlas", false},
        {"bindingNames", json::object({
            {"a", "A"},
            {"b", "B"},
            {"c", "C"},
            {"x", "X"},
            {"y", "Y"},
            {"z", "Z"},
            {"l", "L"},
            {"r", "R"}
        })}
    });

#define GUARD(exp) \
try { exp } catch (...) {}
    
    editor.debug = false;
    editor.battleTest = false;
    
    if (argc > 1) {
        if (!strcmp(argv[1], "debug") || !strcmp(argv[1], "test"))
            editor.debug = true;
        else if (!strcmp(argv[1], "btest"))
            editor.battleTest = true;
        
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "debug"))
                launchArgs.push_back(argv[i]);
        }
    }
    
    json baseConf = readConfFile(CONF_FILE);
    copyObject(optsJ, baseConf);
    copyObject(optsJ["bindingNames"], baseConf["bindingNames"], "bindingNames .");
    
#define SET_OPT_CUSTOMKEY(var, key, type) GUARD(var = optsJ[#key].get<type>();)
#define SET_OPT(var, type) SET_OPT_CUSTOMKEY(var, var, type)
#define SET_STRINGOPT(var, key) GUARD(var = optsJ[#key].get<std::string>();)
    
    SET_STRINGOPT(gameFolder, gameFolder);
    SET_STRINGOPT(dataPathOrg, dataPathOrg);
    SET_STRINGOPT(dataPathApp, dataPathApp);
    SET_STRINGOPT(iconPath, iconPath);
    SET_STRINGOPT(execName, execName);
    SET_OPT(allowSymlinks, bool);
    SET_OPT(pathCache, bool);
    SET_OPT_CUSTOMKEY(jit.enabled, JITEnable, bool);
    SET_OPT_CUSTOMKEY(jit.verboseLevel, JITVerboseLevel, int);
    SET_OPT_CUSTOMKEY(jit.maxCache, JITMaxCache, int);
    SET_OPT_CUSTOMKEY(jit.minCalls, JITMinCalls, int);
    SET_OPT_CUSTOMKEY(yjit.enabled, YJITEnable, bool);
    SET_OPT(rgssVersion, int);
    SET_OPT(defScreenW, int);
    SET_OPT(defScreenH, int);
    
    // Take a break real quick and witch to set game folder and read the game's ini
    if (!gameFolder.empty() && !mkxp_fs::setCurrentDirectory(gameFolder.c_str())) {
        throw Exception(Exception::MKXPError, "Unable to switch into gameFolder %s", gameFolder.c_str());
    }
    
    readGameINI();
    
    // Now check for an extra mkxp.conf in the user's save directory and merge anything else from that
    userConfPath = mkxp_fs::normalizePath(std::string(customDataPath + "/" CONF_FILE).c_str(), 0, 1);
    json userConf = readConfFile(userConfPath.c_str());
    copyObject(optsJ, userConf);
    
    // now RESUME
    
    SET_OPT(debugMode, bool);
    SET_OPT(displayFPS, bool);
    SET_OPT(printFPS, bool);
    SET_OPT(fullscreen, bool);
    SET_OPT(fixedAspectRatio, bool);
    SET_OPT(smoothScaling, int);
    SET_OPT(smoothScalingDown, int);
    SET_OPT(bitmapSmoothScaling, int);
    SET_OPT(bitmapSmoothScalingDown, int);
    SET_OPT(smoothScalingMipmaps, bool);
    SET_OPT(bicubicSharpness, int);
#ifdef MKXPZ_SSL
    SET_OPT(xbrzScalingFactor, int);
#endif
    SET_OPT(enableHires, bool);
    SET_OPT(textureScalingFactor, double);
    SET_OPT(framebufferScalingFactor, double);
    SET_OPT(atlasScalingFactor, double);
    SET_OPT(winResizable, bool);
    SET_OPT(vsync, bool);
    SET_STRINGOPT(windowTitle, windowTitle);
    SET_OPT(fixedFramerate, int);
    SET_OPT(frameSkip, bool);
    SET_OPT(syncToRefreshrate, bool);
    fillStringVec(optsJ["solidFonts"], solidFonts);
    for (std::string & solidFont : solidFonts)
        std::transform(solidFont.begin(), solidFont.end(), solidFont.begin(),
            [](unsigned char c) { return std::tolower(c); });
#ifdef __APPLE__
    SET_OPT(preferMetalRenderer, bool);
#endif
    SET_OPT(subImageFix, bool);
    SET_OPT(enableBlitting, bool);
    SET_OPT_CUSTOMKEY(integerScaling.active, integerScalingActive, bool);
    SET_OPT_CUSTOMKEY(integerScaling.lastMileScaling, integerScalingLastMile, bool);
    SET_OPT(maxTextureSize, int);
    SET_OPT(anyAltToggleFS, bool);
    SET_OPT(enableReset, bool);
    SET_OPT(enableSettings, bool);
    SET_STRINGOPT(midi.soundFont, midiSoundFont);
    SET_OPT_CUSTOMKEY(midi.chorus, midiChorus, bool);
    SET_OPT_CUSTOMKEY(midi.reverb, midiReverb, bool);
    SET_OPT_CUSTOMKEY(SE.sourceCount, SESourceCount, int);
    SET_OPT_CUSTOMKEY(BGM.trackCount, BGMTrackCount, int);
    SET_STRINGOPT(customScript, customScript);
    SET_OPT(useScriptNames, bool);
    SET_OPT(dumpAtlas, bool);
    
    fillStringVec(optsJ["preloadScript"], preloadScripts);
    fillStringVec(optsJ["postloadScript"], postloadScripts);
    fillStringVec(optsJ["RTP"], rtps);
    fillStringVec(optsJ["patches"], patches);
    fillStringVec(optsJ["fontSub"], fontSubs);
    for (std::string & fontSub : fontSubs)
        std::transform(fontSub.begin(), fontSub.end(), fontSub.begin(),
            [](unsigned char c) { return std::tolower(c); });
    SET_OPT(fontScale, double);
    SET_OPT(fontKerning, bool);
    SET_OPT(fontHinting, int);
    SET_OPT(fontHeightReporting, int);
    SET_OPT(fontOutlineCrop, bool);
    fillStringVec(optsJ["rubyLoadpath"], rubyLoadpaths);
    
    auto &bnames = optsJ["bindingNames"];
    
#define BINDING_NAME(btn) kbActionNames.btn = bnames[#btn].get<std::string>()
    BINDING_NAME(a);
    BINDING_NAME(b);
    BINDING_NAME(c);
    BINDING_NAME(x);
    BINDING_NAME(y);
    BINDING_NAME(z);
    BINDING_NAME(l);
    BINDING_NAME(r);
    
    rgssVersion = clamp(rgssVersion, 0, 3);
    SE.sourceCount = clamp(SE.sourceCount, 1, 64);
    BGM.trackCount = clamp(BGM.trackCount, 1, 16);
    
    // Determine whether to open a console window on... Windows
    winConsole = getEnvironmentBool("MKXPZ_WINDOWS_CONSOLE", editor.debug);
    
#ifdef __APPLE__
    // Determine whether to use the Metal renderer on macOS
    // Environment variable takes priority over the json setting
    preferMetalRenderer = isMetalSupported() && getEnvironmentBool("MKXPZ_MACOS_METAL", preferMetalRenderer);
#endif
    
    // Determine whether to allow manual selection of a game folder on startup
    // Only works on macOS atm, mainly used to test games located outside of the bundle.
    // The config is re-read after the window is already created, so some entries
    // may not take effect
    manualFolderSelect = getEnvironmentBool("MKXPZ_FOLDER_SELECT", false);
    
    raw = optsJ;
}

static void setupScreenSize(Config &conf) {
    if (conf.defScreenW <= 0)
        conf.defScreenW = (conf.rgssVersion == 1 ? 640 : 544);
    
    if (conf.defScreenH <= 0)
        conf.defScreenH = (conf.rgssVersion == 1 ? 480 : 416);
}

bool Config::fontIsSolid(const char *fontName) const {
    for (const std::string& solidfont : solidFonts)
        if (!strcmp(solidfont.c_str(), fontName)) return true;
    
    return false;
}

void Config::readGameINI() {
    if (!customScript.empty()) {
        game.title = customScript.c_str();
        
        if (rgssVersion == 0)
            rgssVersion = 1;
        
        setupScreenSize(*this);
        
        return;
    }
    
    std::string iniFileName(execName + ".ini");
    SDLRWStream iniFile(iniFileName.c_str(), "r");
    
    bool convSuccess = false;
    if (iniFile)
    {
        INIConfiguration ic;
        if (ic.load(iniFile.stream()))
        {
            GUARD(game.title = ic.getStringProperty("Game", "Title"););
            GUARD(game.scripts = ic.getStringProperty("Game", "Scripts"););
            
            strReplace(game.scripts, '\\', '/');
            
            if (game.title.empty()) {
                Debug() << iniFileName + ": Could not find Game.Title";
            }
            
            if (game.scripts.empty())
                Debug() << iniFileName + ": Could not find Game.Scripts";
        }
    }
    else
        Debug() << "Could not read" << iniFileName;
    
    try {
        game.title = Encoding::convertString(game.title);
        convSuccess = true;
    }
    catch (const Exception &e) {
        Debug() << iniFileName + ": Could not determine encoding of Game.Title";
    }
    
    if (game.title.empty() || !convSuccess)
        game.title = "mkxp-z";
    
    if (dataPathOrg.empty())
        dataPathOrg = ".";
    
    if (dataPathApp.empty())
        dataPathApp = game.title;
    
    customDataPath = mkxp_fs::normalizePath(prefPath(dataPathOrg.c_str(), dataPathApp.c_str()).c_str(), 0, 1);
    
    if (rgssVersion == 0) {
        /* Try to guess RGSS version based on Data/Scripts extension */
        rgssVersion = 1;
        
        if (!game.scripts.empty()) {
            const char *p = &game.scripts[game.scripts.size()];
            const char *head = &game.scripts[0];
            
            while (--p != head)
                if (*p == '.')
                    break;
            
            if (!strcmp(p, ".rvdata"))
                rgssVersion = 2;
            else if (!strcmp(p, ".rvdata2"))
                rgssVersion = 3;
        }
    }
    
    setupScreenSize(*this);
}
