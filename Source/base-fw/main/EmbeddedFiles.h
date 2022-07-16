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
  EF_EFILE_FONT_COPYRIGHTS_EUROSTILE_EXTENDED2_BOLD_COPYRIGHT_HTML = 0,    /*!< @brief File: font/copyrights/eurostile_extended2_bold_copyright.html (size: 4.6 KB) */
  EF_EFILE_INDEX_HTML = 1,    /*!< @brief File: index.html (size: 3.6 KB) */
  EF_EFILE_CSS_CONTENT_CSS = 2,    /*!< @brief File: css/content.css (size: 2.4 KB) */
  EF_EFILE_JS_APP_JS = 3,    /*!< @brief File: js/app.js (size: 4.4 KB) */
  EF_EFILE_JS_VUE_MIN_JS = 4,    /*!< @brief File: js/vue.min.js (size: 92.0 KB) */
  EF_EFILE_CONFIG_JSON = 5,    /*!< @brief File: config.json (size: 18.2 KB) */
  EF_EFILE_FONT_COPYRIGHTS_ANCIENT_VIRTUAL_COPYRIGHT_TXT = 6,    /*!< @brief File: font/copyrights/ancient_virtual_copyright.txt (size: 46 B) */
  EF_EFILE_FONT_COPYRIGHTS_STARGATEUNIVERSE_LISEZ_MOI_TXT = 7,    /*!< @brief File: font/copyrights/stargateuniverse-lisez-moi.txt (size: 1.9 KB) */
  EF_EFILE_FONT_COPYRIGHTS_STARGATEUNIVERSE_README_TXT = 8,    /*!< @brief File: font/copyrights/stargateuniverse-readme.txt (size: 1.8 KB) */
  EF_EFILE_FAVICON_ICO = 9,    /*!< @brief File: favicon.ico (size: 1.1 KB) */
  EF_EFILE_FONT_ANCIENT_VIRTUAL_TTF_WOFF = 10,    /*!< @brief File: font/ancient_virtual.ttf.woff (size: 11.1 KB) */
  EF_EFILE_FONT_EUROSTILE_EXTENDED2_BOLD_WOFF = 11,    /*!< @brief File: font/eurostile_extended2_bold.woff (size: 14.3 KB) */
  EF_EFILE_IMG_STARGATE_PNG = 12,    /*!< @brief File: img/stargate.png (size: 378.7 KB) */
  EF_EFILE_FONT_STARGATEUNIVERSE_WOFF = 13,    /*!< @brief File: font/stargateuniverse.woff (size: 5.7 KB) */
  EF_EFILE_COUNT = 14
} EF_EFILE;

/*! @brief Check if compressed flag is active */
#define EF_ISFILECOMPRESSED(x) ((x & EF_EFLAGS_GZip) == EF_EFLAGS_GZip)

extern const EF_SFile EF_g_sFiles[EF_EFILE_COUNT];
extern const uint8_t EF_g_u8Blobs[];

#endif
