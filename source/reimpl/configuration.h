/**
 * @file  configuration.h
 * @brief Minimal reimplementation of android/configuration.h. AConfiguration is
 *        opaque in the real NDK too, so the engine never looks at its fields
 *        directly -- only our own AConfiguration_* functions do.
 *
 * Confirmed needed via `objdump -T` on libbadland2.so (armeabi-v7a):
 * AConfiguration_new/delete/fromAssetManager/getCountry/getLanguage are all
 * UND imports of the .so (android_native_app_glue reads locale at startup).
 */

#ifndef ANDROID_CONFIGURATION_H
#define ANDROID_CONFIGURATION_H

#include "reimpl/asset_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

struct AConfiguration;
typedef struct AConfiguration AConfiguration;

AConfiguration * AConfiguration_new(void);
void AConfiguration_delete(AConfiguration *config);
void AConfiguration_fromAssetManager(AConfiguration *out, AAssetManager *am);
void AConfiguration_getCountry(AConfiguration *config, char *outCountry);
void AConfiguration_getLanguage(AConfiguration *config, char *outLanguage);

#ifdef __cplusplus
};
#endif

#endif // ANDROID_CONFIGURATION_H
