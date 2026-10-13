#include "reimpl/configuration.h"

#include <psp2/apputil.h>
#include <psp2/system_param.h>
#include <malloc.h>
#include <string.h>

struct AConfiguration {
    int unused;
};

AConfiguration * AConfiguration_new(void) {
    AConfiguration *config = (AConfiguration *) malloc(sizeof(AConfiguration));
    memset(config, 0, sizeof(AConfiguration));
    return config;
}

void AConfiguration_delete(AConfiguration *config) {
    free(config);
}

void AConfiguration_fromAssetManager(AConfiguration *out, AAssetManager *am) {
    (void)out;
    (void)am;
}

void AConfiguration_getCountry(AConfiguration *config, char *outCountry) {
    (void)config;
    int lang = SCE_SYSTEM_PARAM_LANG_ENGLISH_US;
    sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG, &lang);

    const char *country = "US";
    switch (lang) {
        case SCE_SYSTEM_PARAM_LANG_FRENCH:
            country = "FR"; break;
        case SCE_SYSTEM_PARAM_LANG_SPANISH:
            country = "ES"; break;
        case SCE_SYSTEM_PARAM_LANG_GERMAN:
            country = "DE"; break;
        case SCE_SYSTEM_PARAM_LANG_ITALIAN:
            country = "IT"; break;
        case SCE_SYSTEM_PARAM_LANG_PORTUGUESE_PT:
            country = "PT"; break;
        case SCE_SYSTEM_PARAM_LANG_PORTUGUESE_BR:
            country = "BR"; break;
        case SCE_SYSTEM_PARAM_LANG_RUSSIAN:
            country = "RU"; break;
        case SCE_SYSTEM_PARAM_LANG_KOREAN:
            country = "KR"; break;
        case SCE_SYSTEM_PARAM_LANG_CHINESE_T:
            country = "TW"; break;
        case SCE_SYSTEM_PARAM_LANG_CHINESE_S:
            country = "CN"; break;
        case SCE_SYSTEM_PARAM_LANG_FINNISH:
            country = "FI"; break;
        case SCE_SYSTEM_PARAM_LANG_JAPANESE:
            country = "JP"; break;
        case SCE_SYSTEM_PARAM_LANG_TURKISH:
            country = "TR"; break;
        case SCE_SYSTEM_PARAM_LANG_ENGLISH_GB:
            country = "GB"; break;
        default:
            country = "US"; break;
    }

    outCountry[0] = country[0];
    outCountry[1] = country[1];
    outCountry[2] = '\0';
}

void AConfiguration_getLanguage(AConfiguration *config, char *outLanguage) {
    (void)config;
    int lang = SCE_SYSTEM_PARAM_LANG_ENGLISH_US;
    sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG, &lang);

    const char *code = "en";
    switch (lang) {
        case SCE_SYSTEM_PARAM_LANG_FRENCH:
            code = "fr"; break;
        case SCE_SYSTEM_PARAM_LANG_SPANISH:
            code = "es"; break;
        case SCE_SYSTEM_PARAM_LANG_GERMAN:
            code = "de"; break;
        case SCE_SYSTEM_PARAM_LANG_ITALIAN:
            code = "it"; break;
        case SCE_SYSTEM_PARAM_LANG_PORTUGUESE_PT:
        case SCE_SYSTEM_PARAM_LANG_PORTUGUESE_BR:
            code = "pt"; break;
        case SCE_SYSTEM_PARAM_LANG_RUSSIAN:
            code = "ru"; break;
        case SCE_SYSTEM_PARAM_LANG_KOREAN:
            code = "ko"; break;
        case SCE_SYSTEM_PARAM_LANG_CHINESE_T:
        case SCE_SYSTEM_PARAM_LANG_CHINESE_S:
            code = "zh"; break;
        case SCE_SYSTEM_PARAM_LANG_FINNISH:
            code = "fi"; break;
        case SCE_SYSTEM_PARAM_LANG_JAPANESE:
            code = "ja"; break;
        case SCE_SYSTEM_PARAM_LANG_TURKISH:
            code = "tr"; break;
        default:
            code = "en"; break;
    }

    outLanguage[0] = code[0];
    outLanguage[1] = code[1];
    outLanguage[2] = '\0';
}
