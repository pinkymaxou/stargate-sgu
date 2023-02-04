#ifndef _EMBEDDEDFILES_H_
#define _EMBEDDEDFILES_H_

#include <stdint.h>

typedef enum
{
    EF_EFLAGS_None = 0,
    EF_EFLAGS_GZip = 1,
} EF_EFLAGS;

typedef struct
{
    const char* strFilename;
    uint32_t u32Length;
    EF_EFLAGS eFlags;
    const uint8_t* pu8StartAddr;
} EF_SFile;

typedef enum
{
    EF_EFILE_ABOUT_OTA_HTML = 0,    /*!< @brief File: about-ota.html (size: 1 KB) */
    EF_EFILE_CONFIG_JSON = 1,    /*!< @brief File: config.json (size: 18 KB) */
    EF_EFILE_FAVICON_ICO = 2,    /*!< @brief File: favicon.ico (size: 1 KB) */
    EF_EFILE_INDEX_HTML = 3,    /*!< @brief File: index.html (size: 4 KB) */
    EF_EFILE_SETTINGS_HTML = 4,    /*!< @brief File: settings.html (size: 1 KB) */
    EF_EFILE_CSS_CONTENT_CSS = 5,    /*!< @brief File: css/content.css (size: 3 KB) */
    EF_EFILE_FONT_ANCIENT_VIRTUAL_TTF_WOFF = 6,    /*!< @brief File: font/ancient_virtual.ttf.woff (size: 11 KB) */
    EF_EFILE_FONT_EUROSTILE_EXTENDED2_BOLD_WOFF = 7,    /*!< @brief File: font/eurostile_extended2_bold.woff (size: 14 KB) */
    EF_EFILE_FONT_STARGATEUNIVERSE_WOFF = 8,    /*!< @brief File: font/stargateuniverse.woff (size: 6 KB) */
    EF_EFILE_FONT_COPYRIGHTS_ANCIENT_VIRTUAL_COPYRIGHT_TXT = 9,    /*!< @brief File: font/copyrights/ancient_virtual_copyright.txt (size: 46  B) */
    EF_EFILE_FONT_COPYRIGHTS_EUROSTILE_EXTENDED2_BOLD_COPYRIGHT_HTML = 10,    /*!< @brief File: font/copyrights/eurostile_extended2_bold_copyright.html (size: 5 KB) */
    EF_EFILE_FONT_COPYRIGHTS_STARGATEUNIVERSE_LISEZ_MOI_TXT = 11,    /*!< @brief File: font/copyrights/stargateuniverse-lisez-moi.txt (size: 2 KB) */
    EF_EFILE_FONT_COPYRIGHTS_STARGATEUNIVERSE_README_TXT = 12,    /*!< @brief File: font/copyrights/stargateuniverse-readme.txt (size: 2 KB) */
    EF_EFILE_IMG_STARGATE_SGU_PNG = 13,    /*!< @brief File: img/stargate-sgu.png (size: 396 KB) */
    EF_EFILE_JS_ABOUT_OTA_JS = 14,    /*!< @brief File: js/about-ota.js (size: 1 KB) */
    EF_EFILE_JS_APP_JS = 15,    /*!< @brief File: js/app.js (size: 4 KB) */
    EF_EFILE_JS_SETTINGS_JS = 16,    /*!< @brief File: js/settings.js (size: 311  B) */
    EF_EFILE_JS_VUE_MIN_JS = 17,    /*!< @brief File: js/vue.min.js (size: 92 KB) */
    EF_EFILE_COUNT = 18
} EF_EFILE;

/*! @brief Check if compressed flag is active */
#define EF_ISFILECOMPRESSED(x) ((x & EF_EFLAGS_GZip) == EF_EFLAGS_GZip)

extern const EF_SFile EF_g_sFiles[EF_EFILE_COUNT];
extern const uint8_t EF_g_u8Blobs[];

#endif
