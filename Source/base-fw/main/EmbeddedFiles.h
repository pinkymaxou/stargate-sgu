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
    EF_EFILE_CONFIG_JSON = 0,    /*!< @brief File: config.json (size: 18 KB) */
    EF_EFILE_FAVICON_ICO = 1,    /*!< @brief File: favicon.ico (size: 1 KB) */
    EF_EFILE_INDEX_HTML = 2,    /*!< @brief File: index.html (size: 4 KB) */
    EF_EFILE_CSS_CONTENT_CSS = 3,    /*!< @brief File: css/content.css (size: 2 KB) */
    EF_EFILE_FONT_ANCIENT_VIRTUAL_TTF_WOFF = 4,    /*!< @brief File: font/ancient_virtual.ttf.woff (size: 11 KB) */
    EF_EFILE_FONT_EUROSTILE_EXTENDED2_BOLD_WOFF = 5,    /*!< @brief File: font/eurostile_extended2_bold.woff (size: 14 KB) */
    EF_EFILE_FONT_STARGATEUNIVERSE_WOFF = 6,    /*!< @brief File: font/stargateuniverse.woff (size: 6 KB) */
    EF_EFILE_FONT_COPYRIGHTS_ANCIENT_VIRTUAL_COPYRIGHT_TXT = 7,    /*!< @brief File: font/copyrights/ancient_virtual_copyright.txt (size: 46  B) */
    EF_EFILE_FONT_COPYRIGHTS_EUROSTILE_EXTENDED2_BOLD_COPYRIGHT_HTML = 8,    /*!< @brief File: font/copyrights/eurostile_extended2_bold_copyright.html (size: 5 KB) */
    EF_EFILE_FONT_COPYRIGHTS_STARGATEUNIVERSE_LISEZ_MOI_TXT = 9,    /*!< @brief File: font/copyrights/stargateuniverse-lisez-moi.txt (size: 2 KB) */
    EF_EFILE_FONT_COPYRIGHTS_STARGATEUNIVERSE_README_TXT = 10,    /*!< @brief File: font/copyrights/stargateuniverse-readme.txt (size: 2 KB) */
    EF_EFILE_JS_APP_JS = 11,    /*!< @brief File: js/app.js (size: 5 KB) */
    EF_EFILE_JS_VUE_MIN_JS = 12,    /*!< @brief File: js/vue.min.js (size: 92 KB) */
    EF_EFILE_COUNT = 13
} EF_EFILE;

/*! @brief Check if compressed flag is active */
#define EF_ISFILECOMPRESSED(x) ((x & EF_EFLAGS_GZip) == EF_EFLAGS_GZip)

extern const EF_SFile EF_g_sFiles[EF_EFILE_COUNT];
extern const uint8_t EF_g_u8Blobs[];

#endif
